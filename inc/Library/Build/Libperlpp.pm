package Library::Build::Libperlpp;

# This code needs some serious refactoring, but it works…

use 5.008;
use strict;
use warnings;

our $VERSION = 0.002;

use Carp qw/croak/;
use Config;

use ExtUtils::Embed qw/ldopts/;
use ExtUtils::Install qw/install/;

use parent 'Library::Build';

my @testcleanfiles = glob 't/*0-*.[ot]';
my @cleanfiles = (qw{/examples/combined source/ppport.h source/evaluate.C perl++/headers/config.h perl++/headers/extend.h blib _build MYMETA.yml}, @testcleanfiles);

sub build_perl {
	my $self = shift;

	my %cpp_files = (
		'perl++/source/config.pre' => 'perl++/headers/config.h',
		'perl++/source/extend.pre' => 'perl++/headers/extend.h',
	);
	while (my ($input, $output) = each %cpp_files) {
		$self->process_cpp($input, $output);
	}
	
	$self->process_perl('perl++/source/evaluate.C.PL', 'perl++/source/evaluate.C');

	$self->create_dir(qw{blib/arch _build});
	$self->copy_files('perl++/headers', 'blib/headers/perl++');

	$self->build_library('perl++' => {
		input_dir     => 'perl++/source',
		linker_append => ldopts,
		include_dirs  => [ qw(blib/headers source) ],
		'C++'         => 1,
	});

	$self->copy_files('lib', 'blib/lib');
	return;
}

sub build_tap {
	my $self = shift;
	$self->create_dir('_build');
	$self->copy_files('tap++/headers', 'blib/headers/tap++');
	$self->build_library('tap++' => {
		input_dir    => 'tap++/source',
		include_dirs => [ qw(blib/headers) ],
		'C++'        => 1,
	});
	return;
}

my %examples = (
	executables => [ qw/combined game/ ],
	libraries   => [ qw/Extend/ ]
);

sub build_examples {
	my $self = shift;
	for my $example_name (@{$examples{executables}}) {
		$self->build_executable("examples/$example_name.C", "examples/$example_name",
			include_dirs         => [ 'blib/headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib/arch' ],
			'C++'                => 1,
		);
	}
	for my $example_name (@{$examples{libraries}}) {
		$self->build_library($example_name, {
			input_files          => [ "$example_name.C" ],
			input_dir            => 'examples',
			include_dirs         => [ 'blib/headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib/arch' ],
			libfile              => "examples/$example_name\.$Config{dlext}",
			'C++'                => 1,
		});
	}
	return;
}

sub build_tests {
	my ($self, %test_executable_for) = @_;
	for my $test_source (sort keys %test_executable_for) {
		$self->build_executable($test_source, $test_executable_for{$test_source},
			include_dirs         => [ qw(blib/headers) ],
			libs                 => [ qw/perl++ tap++/ ],
			libdirs              => [ 'blib/arch' ],
			'C++'                => 1,
		);
	}
	return;
}

sub name_for_test {
	my $name = shift;
	my $ext = ".t$Config{_exe}";
	$name =~ s{ \.C $ }{$ext}x;
	return $name;
}

sub test_map {
	my $self = shift;
	my @test_goals = $self->{test_files} ? split / /, $self->{test_files} : glob 't/*.C';
	return map { ( $_ => name_for_test($_) ) } @test_goals;
}

my %action_map = (
	build     => sub { 
		my $builder = shift;
		$builder->build_perl;
	},
	build_tap => sub {
		my $builder = shift;
		$builder->build_tap
	},
	test      => sub {
		my $builder = shift;
		$builder->build_perl;
		$builder->build_tap;
		my %test_map = $builder->test_map;
		$builder->build_tests(%test_map);

		$builder->run_tests(sort values %test_map)
	},
	testbuild => sub {
		my $builder = shift;
		$builder->build_perl;
		$builder->build_tap;
		my %test_map = $builder->test_map;
		$builder->build_tests(%test_map);
	},
	examples  => sub {
		my $builder = shift;
		$builder->build_perl;
		$builder->build_examples;
	},
	install   => sub {
		my $builder = shift;
		$builder->build_perl;
		$builder->build_tap;

		install([
			from_to => {
				'blib/arch'    => $builder->{libdir} || (split ' ', $Config{libpth})[0],
				'blib/headers' => $builder->{incdir} || $Config{usrinc},
				'blib/lib'     => $builder->{moddir} || $Config{installsitelib},
			},
			verbose => $builder->{quiet} <= 0,
			dry_run => $builder->{dry_run},
		]);
	},
	dist      => sub {
		my $builder = shift;
		require Archive::Tar;
		my $arch = Archive::Tar->new;
		my @files = map { chomp; $_ } do { open my $file, '<', 'MANIFEST'; <$file> };
		$arch->add_files(@files);
		my $version = $builder->{version};
		$arch->write("libperl++-$version.tar.gz", 9, "libperl++-$version");
	},
	clean     => sub {
		my $builder = shift;
		$builder->remove_tree(@cleanfiles);
	},
	realclean => sub {
		my $builder = shift;
		$builder->remove_tree(@cleanfiles, 'Build');
	},
	testclean => sub {
		my $builder = shift;
		$builder->remove_tree(@testcleanfiles);
	},
	help      => sub {
		my $builder = shift;
		print "No help available yet\n";
	},
);

sub new {
	my ($class, $arguments, $cached, $version) = @_;

	my $self = $class->SUPER::new(argv => $arguments, cached => $cached, version => $version);
	$self->register_actions(%action_map);
	return $self;
}

1;
