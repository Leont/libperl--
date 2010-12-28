package Library::Build::Compile;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp 'croak';
use Config;
use File::Basename qw/basename dirname/;
use File::Spec::Functions qw/catfile catdir splitdir/;

my $compiler = $Config{cc} eq 'cl' ? 'msvc' : 'gcc';

sub compiler_flags {
	return ($compiler eq 'gcc') ? [ qw/--std=gnu++0x -ggdb3 -DDEBUG -Wall -Wshadow -Wnon-virtual-dtor -Wsign-promo -Wextra -Winvalid-pch/ ] :
	($compiler eq 'msvc') ? [ qw{/TP /EHsc /Wall} ] :
	[];
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
		opendir my ($dh), $input_dir or croak "Can't open input directory '$input_dir': $!";
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

my %compile_methods = (
	cbuilder => sub {
		my $self = shift;
		require ExtUtils::CBuilder;
		return $self->{builder} ||= ExtUtils::CBuilder->new(quiet => $self->stash('verbose') < 0)
	},
	create_by_system => sub {
		my ($self, $exec, $input, $output) = @_;
		if (not -e $output or -M $input < -M $output) {
			my_system($self, $exec, $input, $output);
		}
		return;
	},
	build_objects => sub {
		my ($self, %args) = @_;

		my $input_dir  = $args{input_dir} || '.';
		my $tempdir    = $args{temp_dir}  || '_build';
		my @raw_files  = get_input_files(@args{qw/input_files input_dir/});
		my %object_for = map { (catfile($input_dir, $_) => catfile($tempdir, $self->cbuilder->object_file($_))) } @raw_files;
		my @input_dirs = ( (defined $self->stash('include_dir') ? @{ $self->stash('include_dir') } : ()), (defined $args{include_dirs} ? @{ $args{include_dirs} } : ()));

		for my $source_file (sort keys %object_for) {
			my $object_file = $object_for{$source_file};
			next if -e $object_file and -M $source_file > -M $object_file;
			$self->create_dir(dirname($object_file));
			$self->cbuilder->compile(
				source               => $source_file,
				object_file          => $object_file,
				'C++'                => $args{'C++'},
				include_dirs         => \@input_dirs,
				extra_compiler_flags => $args{cc_flags} || compiler_flags(),
			);
		}
		return values %object_for;
	},
	build_library => sub {
		my ($self, %args) = @_;

		my @objects      = $self->build_objects(%args);

		my $output_dir   = $args{output_dir} || 'blib';
		my $library_file = $args{libfile} || catfile($output_dir, 'so', 'lib' . $self->cbuilder->lib_file($args{name}));
		my $linker_flags = linker_flags($args{libs}, $args{libdirs}, append => $args{linker_append}, 'C++' => $args{'C++'});
		$self->create_dir(dirname($library_file));
		$self->cbuilder->link(
			lib_file           => $library_file,
			objects            => \@objects,
			extra_linker_flags => $linker_flags,
			module_name        => $args{name},
		) if not -e $library_file or grep { (-M $_ < -M $library_file ) } @objects;
		return;
	},
	build_executable => sub {
		my ($self, %args) = @_;

		my @objects      = $self->build_objects(%args);
		my $linker_flags = linker_flags($args{libs}, $args{libdirs}, append => $args{linker_append}, 'C++' => $args{'C++'});
		$self->create_dir(dirname($args{output}));
		$self->cbuilder->link_executable(
			objects            => \@objects,
			exe_file           => $args{output},
			extra_linker_flags => $linker_flags,
			'C++'              => $args{'C++'},
		) if not -e $args{output} or grep { (-M $_ < -M $args{output}) } @objects;
		return;
	},
	process_cpp => sub {
		my ($self, $input, $output) = @_;
		$self->create_by_system([ $Config{cpp}, split(/ /, $Config{ccflags}), '-I' . catdir($Config{archlibexp}, 'CORE') ], $input, $output);
		return;
	},
	process_perl => sub {
		my ($self, $input, $output) = @_;
		$self->create_by_system([ $^X, '-T' ], $input, $output);
		return;
	},
);

sub mixin {
	my $builder = shift;

	$builder->inject_roles({ build => \%compile_methods });
	$builder->register_dirty(binary  => [qw/_build/]);
	$builder->register_argument(include_dir => 2);
	$builder->register_paths(
		'so'      => (split ' ', $Config{libpth})[0],
		'headers' => $Config{usrinc},
	);
	return;
}

1;
