package Library::Build::Base;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Archive::Tar;
use Carp 'croak';
use Config;
use ExtUtils::Install qw/install/;
use ExtUtils::Manifest qw/maniread manicheck mkmanifest/;
use File::Basename qw/basename dirname/;
use File::Copy qw/copy/;
use File::Find qw/find/;
use File::Path qw/mkpath rmtree/;
use File::Spec::Functions qw/catfile catdir splitdir/;
use List::MoreUtils qw/any uniq/;
use Pod::Man;
use POSIX qw/strftime/;
use TAP::Harness;

my $NONREADABLE = ~oct 22;
my $SECURE      = oct 744;
my $compiler    = $Config{cc} eq 'cl' ? 'msvc' : 'gcc';

sub _compiler_flags {
	return ($compiler eq 'gcc') ? [ qw/--std=gnu++0x -ggdb3 -DDEBUG -Wall -Wshadow -Wnon-virtual-dtor -Wsign-promo -Wextra -Winvalid-pch/ ] : 
	($compiler eq 'msvc') ? [ qw{/TP /EHsc /Wall} ] : 
	[];
}

sub _get_input_files {
	my ($input_files, $input_dir) = @_;
	if ($input_files) {
		if (ref $input_files) {
			return @{$input_files};
		}
		else {
			return $input_files;
		}
	}
	elsif ($input_dir) {
		opendir my ($dh), $input_dir or croak "Can't open input directory '$input_dir': $!";
		my @ret = grep { /^ .+ \. C $/xsm } readdir $dh;
		closedir $dh;
		return @ret;
	}
	croak 'Can\'t establish source files';
}

sub _linker_flags {
	my ($libs, $libdirs, %options) = @_;
	my @elements;
	if ($compiler eq 'gcc') {
		push @elements, map { "-l$_" } @{$libs};
		push @elements, map { "-L$_" } @{$libdirs};
		if ($options{'C++'}) {
			push @elements, '-lstdc++';
		}
	}
	elsif ($compiler eq 'msvc') {
		push @elements, map { "$_.dll" } @{$libs};
		push @elements, map { qq{-libpath:"$_"} } @{$libdirs};
		if ($options{'C++'}) {
			push @elements, 'msvcprt.lib';
		}
	}
	push @elements, $options{append} if defined $options{append};
	return join ' ', @elements;
}

# XXX

sub cbuilder {
	my $self = shift;
	require ExtUtils::CBuilder;
	return $self->{builder} ||= ExtUtils::CBuilder->new(quiet => $self->stash('verbose') < 0)
}

my $my_system = $^O eq 'MSWin32'
? sub {
	my ($self, $exec, $input, $output) = @_;
	my $call = join ' ', @{$exec}, $input, '>', $output;
	print "$call\n" if $self->stash('verbose') >= 0;
	system $call and croak "Couldn't call system(): $!";
	return;
}
: sub {
	my ($self, $exec, $input, $output) = @_;
	my @call = (@{$exec}, $input);
	print "@call > $output\n" if $self->stash('verbose') >= 0;
	my $pid = fork;
	if ($pid) {
		waitpid $pid, 0;
	}
	else {
		open STDOUT, '>', $output or croak "Can't write to file '$output': $!";
		exec @call or croak "Couldn't exec: $!";
	}
	return;
};

sub create_by_system {
	my ($self, $exec, $input, $output) = @_;
	if (not -e $output or -M $input < -M $output) {
		$my_system->($self, $exec, $input, $output);
	}
	return;
}

sub process_cpp {
	my ($self, $input, $output) = @_;
	$self->create_by_system([ $Config{cpp}, split(/ /, $Config{ccflags}), '-I' . catdir($Config{archlibexp}, 'CORE') ], $input, $output);
	return;
}

sub process_perl {
	my ($self, $input, $output) = @_;
	$self->create_by_system([ $^X, '-T' ], $input, $output);
	return;
}

sub create_dir {
	my ($self, @dirs) = @_;
	mkpath(\@dirs, $self->stash('verbose') >= 0, $SECURE);
	return;
}

sub copy_files {
	my ($self, $source, $destination) = @_;
	if (-d $source) {
		$self->create_dir($destination);
		opendir my $dh, $source or croak "Can't open dir $source: $!";
		for my $filename (readdir $dh) {
			next if $filename =~ / \A \. /xms;
			$self->copy_files(catfile($source, $filename), catfile($destination, $filename));
		}
		closedir $dh;
	}
	elsif (-f $source) {
		$self->create_dir(dirname($destination));
		if (not -e $destination or -M $source < -M $destination) {
			print "cp $source $destination\n" if $self->stash('verbose') >= 0;
			copy($source, $destination) or croak "Could not copy '$source' to '$destination': $!";
		}
	}
	return;
}

sub build_objects {
	my ($self, %args) = @_;

	my $input_dir  = $args{input_dir} || '.';
	my $tempdir    = $args{temp_dir}  || '_build';
	my @raw_files  = _get_input_files(@args{qw/input_files input_dir/});
	my %object_for = map { (catfile($input_dir, $_) => catfile($tempdir, $self->cbuilder->object_file($_))) } @raw_files;
	my @input_dirs = ( (defined $self->stash('include_dirs') ? split(/ : /x, $self->stash('include_dirs')) : ()), (defined $args{include_dirs} ? @{ $args{include_dirs} } : ()));

	for my $source_file (sort keys %object_for) {
		my $object_file = $object_for{$source_file};
		next if -e $object_file and -M $source_file > -M $object_file;
		$self->create_dir(dirname($object_file));
		$self->cbuilder->compile(
			source               => $source_file,
			object_file          => $object_file,
			'C++'                => $args{'C++'},
			include_dirs         => \@input_dirs,
			extra_compiler_flags => $args{cc_flags} || _compiler_flags(),
		);
	}
	return values %object_for;
}

sub build_library {
	my ($self, %args) = @_;

	my @objects      = $self->build_objects(%args);

	my $output_dir   = $args{output_dir} || 'blib';
	my $library_file = $args{libfile} || catfile($output_dir, 'so', 'lib' . $self->cbuilder->lib_file($args{name}));
	my $linker_flags = _linker_flags($args{libs}, $args{libdirs}, append => $args{linker_append}, 'C++' => $args{'C++'});
	$self->create_dir(dirname($library_file));
	$self->cbuilder->link(
		lib_file           => $library_file,
		objects            => \@objects,
		extra_linker_flags => $linker_flags,
		module_name        => 'libperl++',
	) if not -e $library_file or any { (-M $_ < -M $library_file ) } @objects;
	return;
}

sub build_executable {
	my ($self, %args) = @_;

	my @objects      = $self->build_objects(%args);
	my $linker_flags = _linker_flags($args{libs}, $args{libdirs}, append => $args{linker_append}, 'C++' => $args{'C++'});
	$self->create_dir(dirname($args{output}));
	$self->cbuilder->link_executable(
		objects            => \@objects,
		exe_file           => $args{output},
		extra_linker_flags => $linker_flags,
		'C++'              => $args{'C++'},
	) if not -e $args{output} or any { (-M $_ < -M $args{output}) } @objects;
	return;
}

sub pod2man {
	my ($self, $source, $dest) = @_;
	return if -e $dest and -M $source > -M $dest;
	$self->create_dir(dirname($dest));
	print "pod2man $source $dest\n" if $self->stash('verbose') >= 0;
	$self->{pod_parser} ||= Pod::Man->new;
	$self->{pod_parser}->parse_from_file($source, $dest);
	return;
}

sub run_tests {
	my ($self, @test_goals) = @_;
	my $library_var = $self->stash('library_var') || $Config{ldlibpthname};
	printf "Report %s\n", strftime('%y%m%d-%H:%M', localtime) if $self->stash('verbose') > -2;
	my $harness = TAP::Harness->new({
		verbosity => $self->stash('verbose'),
		exec => sub {
			my (undef, $file) = @_;
			return -B $file ? [ $file ] : [ $^X, '-T', '-I' . catdir(qw/blib lib/), $file ];
		},
		merge => 1,
		color => -t STDOUT,
	});

	my $ld_dir = catdir(qw/blib so/);
	local $ENV{$library_var} = $ld_dir if -d $ld_dir;
	return $harness->runtests(@test_goals);
}

sub remove_tree {
	my ($self, @files) = @_;
	rmtree(\@files, $self->stash('verbose') >= 0, 0);
	return;
}

sub find_files {
	my ($self, $dir, $regexp) = @_;
	my @ret;
	find({
		wanted   => sub {
			push @ret, $_ if $_ =~ $regexp; 
		},
		no_chdir => 1,
	} , $dir) if -d $dir;
	@ret = sort @ret;
	return @ret;
}

sub prompt {
	my ($self, $question, $default) = @_;
	return $default if $ENV{PERL_MM_USE_DEFAULT} or not -t STDIN and eof STDIN;
	print "$question [$default] ";
	my $answer = <STDIN>;
	return $default if not defined $answer;
	chomp $answer;
	return $answer;
}

sub yes_no {
	my ($self, $question, $default) = @_;
	while (1) {
		my $answer = $self->prompt($question, $default ? 'y' : 'n');
		return 1 if $answer =~ / \A y ( es )? \z /xmsi;
		return 0 if $answer =~ / \A n ( o  )? \z /xmsi;
		print "Please answer 'y' or 'n'.\n";
	}
}

sub remove_dirty_files {
	my ($self, @categories) = @_;
	my @keys = map { $_ eq 'all' ? keys %{ $self->{dirty_files} } : $_ } @categories;
	my @files = map { @{ $self->{dirty_files}{$_} } } uniq sort @keys;
	$self->remove_tree(@files);
	return;
}

sub register_dirty {
	my ($self, %files_map) = @_;
	while (my ($category, $files) = each %files_map) {
		push @{ $self->{dirty_files}{$category} }, @{$files};
	}
	return;
}

sub register_paths {
	my ($self, %paths) = @_;
	while (my ($source, $destination) = each %paths) {
		$self->{install_paths}{"blib/$source"} = $destination;
	}
	return;
}

sub arg {
	my ($self, $argname) = @_;
	return $self->{args}{$argname};
}

my %default_actions = (
	lib       => sub {
		my $builder = shift;
		for my $ext (qw/pm pod/) {
			for my $file ($builder->find_files('lib', qr/ \. $ext \z /xms)) {
				$builder->copy_files($file, catfile('blib', $file));
				my @directories = splitdir(dirname($file));
				shift @directories;
				my $base = basename($file, $ext);
				$builder->pod2man($file, catfile('blib', 'libdoc', join('::', @directories, $base) . '3'));
			}
		}
	},
	build     => sub {
		my $builder = shift;
		$builder->dispatch('lib');
	},
	testbuild => sub {
		my $builder = shift;
		$builder->dispatch('build');
	},
	test      => sub {
		my $builder = shift;

		$builder->dispatch('testbuild');
		my @tests = defined $builder->stash('test_file') ? @{ $builder->stash('test_file') } : $builder->find_files('t', qr/ \. t (?:$Config{_exe})? \z /xms);
		$builder->run_tests(@tests);
	},
	install   => sub {
		my $builder = shift;
		$builder->dispatch('build');

		install([
			from_to => $builder->{install_paths},
			verbose => $builder->stash('verbose') >= 0,
			dry_run => $builder->stash('dry_run'),
		]);
	},
	dist      => sub {
		my $builder = shift;
		$builder->dispatch('build');
		my $manifest = maniread() or croak 'No MANIFEST found';
		my @files = keys %{$manifest};
		my $arch = Archive::Tar->new;
		$arch->add_files(@files);
		$_->mode($_->mode & $NONREADABLE) for $arch->get_files;
		my $release_name = $builder->name . '-' . $builder->version;
		print "tar xjf $release_name.tar.gz @files\n" if $builder->stash('verbose') >= 0;
		$arch->write("$release_name.tar.gz", COMPRESS_GZIP, $release_name);
	},
	manifest  => sub {
		mkmanifest();
	},
	distcheck => sub {
		my @missing = manicheck();
		croak "Missing files @missing" if @missing;
	},
	help      => sub {
		my $builder = shift;
		print "Available commands:\n";
		for my $command (sort keys %{$builder->{action_map}}) {
			print "$command\n";
		}
	},
	clean     => sub {
		my $builder = shift;
		$builder->remove_dirty_files($builder->stash('what') || 'all');
	},
	realclean => sub {
		my $builder = shift;
		$builder->dispatch('clean');
		$builder->remove_tree('Build');
	},
	testclean => sub {
		my $builder = shift;
		$builder->remove_dirty_files('test');
	},
);

sub mixin {
	my $builder = shift;
	while (my ($name, $glob) = each %Library::Build::Base::) {
		next if $name =~ / \A _ /x;
		next if $name eq 'mixin';
		next if not defined *$glob{CODE};
		next if *$glob{PACKAGE} ne 'Library::Build::Base';
		$builder->inject_method($name, *$glob{CODE});
	}
	$builder->stash(verbose => 0);
	$builder->register_actions(%default_actions);
	$builder->register_dirty(
		binary => [ qw/blib _build/ ],
		meta   => [ 'MYMETA.yml' ],
		test   => [ '_build/t' ],
		tarball  => [ $builder->name . '' . $builder->version . '.tar.gz' ],
	);
	$builder->register_argument($_, 0) for qw/verbose dry_run/;
	$builder->register_argument($_, 1) for qw/library_var what/;
	$builder->register_argument($_, 2) for qw/include_dir test_file/;
	$builder->register_paths(
		'so'      => $builder->arg('libdir') || (split ' ', $Config{libpth})[0],
		'headers' => $builder->arg('incdir') || $Config{usrinc},
		'lib'     => $builder->arg('moddir') || $Config{installsitelib},
	);
	return;
}

1;
