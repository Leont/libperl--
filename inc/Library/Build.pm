package Library::Build;

# This code needs some serious refactoring, but it works…

use 5.008;
use strict;
use warnings;

our $VERSION = 0.002;

use Carp qw/croak/;
use Config;

use Exporter 5.57 qw/import/;
our @EXPORT_OK = qw/dispatch/;

use ExtUtils::Embed qw/ldopts/;
use ExtUtils::Install qw/install/;

use Library::Build::Util;

my @testcleanfiles = glob 't/*.[ot]';
my @cleanfiles = (qw{/examples/combined source/ppport.h source/evaluate.C perl++/headers/config.h perl++/headers/extend.h blib _build}, @testcleanfiles);

sub build_perl {
	my $builder = shift;

	$builder->create_by_system(sub { 
		(my $oldname = $_ ) =~ s{ perl\+\+ / headers / (\w+) \.h  }{perl++/source/$1.pre}x;
		"$Config{cpp} $Config{ccflags} -I$Config{archlibexp}/CORE $oldname > $_";
	}, qw{perl++/headers/config.h perl++/headers/extend.h});

	$builder->create_by_system( sub { "$^X -T $_.PL > $_" }, 'perl++/source/evaluate.C');

	$builder->create_dir(qw{blib/arch _build});
	$builder->build_library('perl++' => {
		input_dir     => 'perl++/source',
		linker_append => ldopts,
		include_dirs  => [ qw(perl++/headers source) ],
		'C++'         => 1,
	});

	$builder->copy_files('lib', 'blib/lib');
	$builder->copy_files('perl++/headers', 'blib/headers/perl++');

	return;
}

sub build_tap {
	my $builder = shift;
	$builder->create_dir('_build');
	$builder->build_library('tap++' => {
		input_dir    => 'tap++/source',
		include_dirs => [ qw(tap++/headers) ],
		'C++'        => 1,
	});
	$builder->copy_files('tap++/headers', 'blib/headers/tap++');
	return;
}

my %examples = (
	executables => [ qw/combined game/ ],
	libraries   => [ qw/Extend/ ]
);

sub build_examples {
	my $builder = shift;
	for my $example_name (@{$examples{executables}}) {
		$builder->build_executable("examples/$example_name.C", 'blib/example_name',
			include_dirs         => [ 'perl++/headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib/arch' ],
			'C++'                => 1,
		);
	}
	for my $example_name (@{$examples{libraries}}) {
		$builder->build_library($example_name, {
			input                =>  [ "$example_name.C" ],
			input_dir            => 'examples',
			include_dirs         => [ 'perl++/headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib/arch' ],
			libfile              => "blib/$example_name.$Config{dlext}",
			'C++'                => 1,
		});
	}
	return;
}

sub build_tests {
	my ($builder, %test_executable_for) = @_;
	for my $test_source (sort keys %test_executable_for) {
		$builder->build_executable($test_source, $test_executable_for{$test_source},
			include_dirs         => [ qw(perl++/headers tap++/headers) ],
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

sub dispatch {
	my ($arguments, $cached) = @_;

	my $builder = Library::Build::Util->new(argv => $arguments, cached => $cached);

	my @test_goals = $builder->{test_files} ? split / /, $builder->{test_files} : glob 't/*.C';
	my %test_map = map { ( $_ => name_for_test($_) ) } @test_goals;

	my %action_map = (
		build     => sub { build_perl($builder) },
		build_tap => sub { build_tap($builder) },
		test      => sub {
			build_perl($builder);
			build_tap($builder);
			build_tests($builder, %test_map);

			$builder->run_tests(sort values %test_map)
		},
		testbuild => sub {
			build_perl($builder);
			build_tap($builder);
			build_tests($builder, %test_map);
		},
		examples  => sub {
			build_perl($builder);
			build_examples($builder);
		},
		install   => sub {
			build_perl($builder);
			build_tap($builder);

			my $incdir = $builder->{incdir}  || $Config{usrinc};
			install([
				from_to => {
					'blib/arch'    => $builder->{libdir}  || (split ' ', $Config{libpth})[0],
					'blib/headers' => "$incdir/perl++",
					'blib/lib'     => $builder->{moddir} || $Config{installsitelib},
				},
				verbose => $builder->{quiet} <= 0,
				dry_run => $builder->{dry_run},
			]);
		},
		clean     => sub {
			$builder->remove_tree(@cleanfiles);
		},
		realclean => sub {
			$builder->remove_tree(@cleanfiles, 'Build');
		},
		testclean => sub {
			$builder->remove_tree(@testcleanfiles);
		},
		help      => sub {
			print "No help available yet\n";
		},
	);

	my $action = $action_map{ $builder->{action} } or croak "No such action defined";
	return $action->();
}

1;
