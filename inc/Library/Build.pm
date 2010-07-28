package Library::Build;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = 0.003;

use Archive::Tar;
use autodie;
use Carp 'croak';
use Config;
use ExtUtils::CBuilder;
use ExtUtils::Install qw/install/;
use File::Basename qw/dirname/;
use File::Copy qw/copy/;
use File::Path qw/mkpath rmtree/;
use File::Spec::Functions qw/catfile catdir/;
use List::MoreUtils qw/any first_index uniq/;
use POSIX qw/strftime/;
use Readonly;
use TAP::Harness;

Readonly my $compiler => $Config{cc} eq 'cl' ? 'msvc' : 'gcc';
Readonly my $NOTFOUND => -1;
Readonly my $SECURE   => oct 744;

sub compiler_flags {
	if ($compiler eq 'gcc') {
		return qw/--std=gnu++0x -ggdb3 -DDEBUG -Wall -Wshadow -Wnon-virtual-dtor -Wsign-promo -Wextra -Winvalid-pch/;
	}
	elsif ($compiler eq 'msvc') {
		return qw{/TP /EHsc /Wall};
	}
}

sub parse_line {
	my ($action, $line) = @_;

	for ($line) {
		next if / ^ \# /xms;
		s{ \n \s+ }{ }x;
		next if / ^ \s* $ /xms;


		if (my ($name, $args) = m/ \A \s* (\* | \w+ ) \s+ (.*?) \s* \z /xms) {
			my @args = split /\s+/, $args;
			return @args if $name eq $action or $name eq '*';
		}
		else {
			croak "Can't parse line '$_'";
		}
	}
	return;
}

sub read_config {
	my $action = shift;

	my @ret;
	
	my @files = (
		($ENV{MODULEBUILDRC} ? $ENV{MODULEBUILDRC}                        : ()), 
		($ENV{HOME} ?          catfile($ENV{HOME},'.modulebuildrc')       : ()), 
		($ENV{USERPROFILE} ?   catfile($ENV{USERPROFILE},'.modulebuldrc') : ()),
	);
	FILE:
	for my $file (@files) {
		next FILE if not -e $file;

		open my $fh, '<', $file or croak "Couldn't open configuration file '$file': $!";
		my @lines = split / \n (?! \s) /xms, do { local $/ = undef, <$fh> };
		close $fh;
		for my $line (@lines) {
			push @ret, parse_line($action, $line);
		}
	}
	return @ret;
}

sub parse_action {
	my $meta_arguments = shift;
	for my $meta_argument (map { $meta_arguments->{$_} } qw/argv envs/) {
		my $position = first_index { not m/ ^ -- /xms and not m/=/xms } @{$meta_argument};
		return splice @{$meta_argument}, $position, 1 if $position != $NOTFOUND;
	}
	return;
}

sub parse_option {
	my ($options, $argument) = @_;
	$argument =~ s/ ^ -- //xms;
	if ($argument =~ / \A (\w+) = (.*) \z /xms) {
		$options->{$1} = $2;
	}
	else {
		$options->{$argument} = 1;
	}
	return;
}

sub parse_options {
	my @args = @_;
	my (%meta_arguments, $name, $version);
	(@meta_arguments{qw/argv cached/}, $name, $version) = @args;
	@{ $meta_arguments{envs} } = split / /, $ENV{PERL_MB_OPT} if $ENV{PERL_MB_OPT};

	my %options = (
		quiet   => 0,
		name    => $name,
		version => $version,
	);

	$options{action} = parse_action(\%meta_arguments) || 'build';

	@{ $meta_arguments{config} } = read_config($options{action});

	for my $argument_list (map { $meta_arguments{$_} } qw/config cached envs argv/) {
		for my $argument (@{$argument_list}) {
			parse_option(\%options, $argument);
		}
	}
	$options{quiet} = -$options{verbose} if not $options{quiet} and $options{verbose};
	return %options;
}

sub get_input_files {
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
		opendir my ($dh), $input_dir;
		my @ret = grep { /^ .+ \. C $/xsm } readdir $dh;
		closedir $dh;
		return @ret;
	}
	croak 'Can\'t establish source files';
}

sub linker_flags {
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

*my_system = $^O eq 'MSWin32'
	? sub {
		my ($self, $exec, $input, $output) = @_;
		my $call = join ' ', @{$exec}, $input, '>', $output;
		print "$call\n" if $self->quiet <= 0;
		system $call and croak "Couldn't system(): $!";
		return;
	}
	: sub {
		my ($self, $exec, $input, $output) = @_;
		my @call = (@{$exec}, $input);
		print "@call > $output\n" if $self->quiet <= 0;
		my $pid = fork;
		if ($pid) {
			waitpid $pid, 0;
		}
		else {
			open STDOUT, '>', $output;
			exec @call or croak "Couldn't exec: $!";
		}
		return;
	};

BEGIN {
	local $@;
	*subname = eval { require Sub::Name } ? \&Sub::Name::subname : sub {};
	eval { require namespace::clean } and namespace::clean->import;
}

for my $accessor_name (qw/name version quiet test_files action/) {
	my $sub = sub {
		my $self = shift;
		return $self->{$accessor_name};
	};
	subname $accessor_name, $sub;
	no strict 'refs';
	*{$accessor_name} = $sub;
}

my %default_actions = (
	lib       => sub {
		my $builder = shift;
		$builder->copy_files('lib', catdir(qw/blib lib/));
	},
	build     => sub {
		my $builder = shift;
		$builder->dispatch('lib');
	},
	testbuild => sub {
	},
	test      => sub {
		my $builder = shift;

		$builder->dispatch('testbuild');
		$builder->run_tests($builder->tests)
	},
	install   => sub {
		my $builder = shift;
		$builder->dispatch('build');

		install([
			from_to => {
				'blib/so'      => $builder->{libdir} || (split ' ', $Config{libpth})[0],
				'blib/headers' => $builder->{incdir} || $Config{usrinc},
				'blib/lib'     => $builder->{moddir} || $Config{installsitelib},
			},
			verbose => $builder->quiet <= 0,
			dry_run => $builder->{dry_run},
		]);
	},
	dist      => sub {
		my $builder = shift;
		$builder->dispatch('build');
		my $arch = Archive::Tar->new;
		open my $file, '<', 'MANIFEST'; 
		my @files = map { chomp; $_ } <$file>;
		close $file;
		$arch->add_files(@files);
		my $release_name = $builder->name . '-' . $builder->version;
		print "tar xjf $release_name.tar.bz2 @files\n" if $builder->quiet <= 0;
		$arch->write("$release_name.tar.bz2", COMPRESS_BZIP, $release_name);
	},
	help      => sub {
		my $builder = shift;
		print "No help available\n";
	},
	clean     => sub {
		my $builder = shift;
		$builder->remove_tree($builder->get_dirty_files('all'));
	},
	realclean => sub {
		my $builder = shift;
		$builder->dispatch('clean');
		$builder->remove_tree('Build');
	},
	testclean => sub {
		my $builder = shift;
		$builder->remove_tree($builder->get_dirty_files('test'));
	},
);

sub new {
	my ($class, @meta) = @_;
	my %self = parse_options(@meta);
	my $self = bless \%self, $class;
	$self->register_actions(%default_actions);
	$self->register_dirty(
		binary => [ qw/blib _build/ ],
		meta   => [ 'MYMETA.yml' ],
		test   => [ '_build/t' ],
		tarball  => [ "$self{name}-$self{version}.tar.bz2" ],
	);
	return $self;
}

sub builder {
	my $self = shift;
	return $self->{builder} ||= ExtUtils::CBuilder->new(quiet => $self->quiet)
}

sub include_dirs {
	my ($self, $extra) = @_;
	return [ (defined $self->{include_dirs} ? split(/:/, $self->{include_dirs}) : ()), (defined $extra ? @{$extra} : ()) ];
}

sub create_by_system {
	my ($self, $exec, $input, $output) = @_;
	if (not -e $output or -M $input < -M $output) {
		my_system($self, $exec, $input, $output);
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
	mkpath(\@dirs, $self->quiet <= 0, $SECURE);
	return;
}

sub copy_files {
	my ($self, $source, $destination) = @_;
	if (-d $source) {
		$self->create_dir($destination);
		opendir my $dh, $source or croak "Can't open dir $source: $!";
		for my $filename (readdir $dh) {
			next if $filename =~ / \A \. /xms;
			$self->copy_files("$source/$filename", "$destination/$filename");
		}
	}
	elsif (-f $source) {
		$self->create_dir(dirname($destination));
		if (not -e $destination or -M $source < -M $destination) {
			print "cp $source $destination\n" if $self->quiet <= 0;
			copy($source, $destination) or croak "Could not copy '$source' to '$destination': $!";
		}
	}
	return;
}

sub build_objects {
	my ($self, %args) = @_;

	my $input_dir  = $args{input_dir} || '.';
	my $tempdir    = $args{temp_dir}  || '_build';
	my @raw_files  = get_input_files(@args{qw/input_files input_dir/});
	my %object_for = map { (catfile($input_dir, $_) => catfile($tempdir, $self->builder->object_file($_))) } @raw_files;

	for my $source_file (sort keys %object_for) {
		my $object_file = $object_for{$source_file};
		next if -e $object_file and -M $source_file > -M $object_file;
		$self->create_dir(dirname($object_file));
		$self->builder->compile(
			source               => $source_file,
			object_file          => $object_file,
			'C++'                => $args{'C++'},
			include_dirs         => $self->include_dirs($args{include_dirs}),
			extra_compiler_flags => $args{cc_flags} || [ compiler_flags ],
		);
	}
	return values %object_for;
}

sub build_library {
	my ($self, %library) = @_;

	my @objects      = $self->build_objects(%library);

	my $output_dir   = $library{output_dir} || 'blib';
	my $library_file = $library{libfile} || catfile($output_dir, 'so', 'lib' . $self->builder->lib_file($library{name}));
	my $linker_flags = linker_flags($library{libs}, $library{libdirs}, append => $library{linker_append}, 'C++' => $library{'C++'});
	$self->create_dir(dirname($library_file));
	$self->builder->link(
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
	my $linker_flags = linker_flags($args{libs}, $args{libdirs}, append => $args{linker_append}, 'C++' => $args{'C++'});
	$self->create_dir(dirname($args{output}));
	$self->builder->link_executable(
		objects            => \@objects,
		exe_file           => $args{output},
		extra_linker_flags => $linker_flags,
		'C++'              => $args{'C++'},
	) if not -e $args{output} or any { (-M $_ < -M $args{output}) } @objects;
	return;
}

sub run_tests {
	my ($self, @test_goals) = @_;
	my $library_var = $self->{library_var} || $Config{ldlibpthname};
	printf "Report %s\n", strftime('%y%m%d-%H:%M', localtime) if $self->quiet < 2;
	my $harness = TAP::Harness->new({ 
		verbosity => -$self->quiet,
		exec => sub {
			my (undef, $file) = @_;
			return -B $file ? [ $file ] : [ $^X, '-T', $file ];
		},
		merge => 1,
		color => -t STDOUT,
	});

	local $ENV{$library_var} = catdir(qw/blib so/);
	return $harness->runtests(@test_goals);
}

sub remove_tree {
	my ($self, @files) = @_;
	rmtree(\@files, $self->quiet <= 0, 0);
	return;
}

sub tests {
	my $self = shift;
	return defined $self->test_files ? split / /, $self->test_files : glob catfile('t', '*.t');
}

sub get_dirty_files {
	my ($self, @categories) = @_;
	my @keys = map { $_ eq 'all' ? keys %{ $self->{dirty_files} } : $_ } @categories;
	return map { @{ $self->{dirty_files}{$_} } } uniq sort @keys;
}

sub register_dirty {
	my ($self, %files_map) = @_;
	while (my ($category, $files) = each %files_map) {
		push @{ $self->{dirty_files}{$category} }, @{$files};
	}
	return;
}

sub register_actions {
	my ($self, %action_map) = @_;
	while (my ($name, $sub) = each %action_map) {
		unshift @{ $self->{action_map}{$name} }, $sub;
	}
	return;
}

sub dispatch {
	my $self        = shift;
	my $action_name = shift || croak 'No action defined';
	my $action_ref  = $self->{action_map}{$action_name} or croak "No action '$action_name' defined";
	$self->dispatch_next($action_ref);
	return;
}

sub dispatch_next {
	my ($self, $action_ref) = @_;
	if (@{$action_ref}) {
		my ($action_sub, @action_rest) = @{$action_ref};
		$action_sub->($self, \@action_rest);
	}
	return;
}

sub dispatch_default {
	my $self = shift;
	$self->dispatch($self->action);
	return;
}

1;
