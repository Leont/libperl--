package Library::Build;

# This code needs some serious refactoring, but it works…

use 5.008;
use strict;
use warnings;

our $VERSION = 0.002;

use Library::Build::Util qw/:all/;
use Config;
use ExtUtils::Embed qw/ldopts/;
use ExtUtils::Install qw/install/;

use Exporter 5.57 qw/import/;

our @EXPORT = qw/dispatch/;

my @testcleanfiles = glob 't/*.[ot]';
my @cleanfiles = (qw{/examples/combined source/ppport.h source/evaluate.C perl++/headers/config.h perl++/headers/extend.h blib}, @testcleanfiles);

my %libraries = (
);

my %options = (
	silent => 0,
);

sub build_perl {
	create_dir(\%options, 'blib/lib', 'blib/build');

	create_by_system { 
		(my $oldname = $_ ) =~ s{ perl\+\+ / headers / (\w+) \.h  }{perl++/source/$1.pre}x;
		"$Config{cpp} $Config{ccflags} -I$Config{archlibexp}/CORE $oldname > $_";
	} \%options, qw{perl++/headers/config.h perl++/headers/extend.h};

	create_by_system { "$^X -T $_.PL > $_" } \%options, 'perl++/source/evaluate.C';

	build_library('perl++' => {
		input_dir     => 'perl++/source',
		linker_append => ldopts,
		include_dirs  => [ qw(perl++/headers source) ],
		'C++'         => 1,
	});
	return;
}

sub build_tap {
	create_dir(\%options, 'blib/lib', 'blib/build');
	build_library('tap++' => {
		input_dir    => 'tap++/source',
		include_dirs => [ qw(tap++/headers) ],
		'C++'        => 1,
	});
	return;
}

my %tests = map { (my $foo = $_) =~ s/\.C$/.t/; $_ => $foo } glob 't/*.C';

my %examples = (
	executables => [ qw/combined game/ ],
	libraries   => [ qw/Extend/ ]
);

sub build_examples {
	for my $example_name (@{$examples{executables}}) {
		build_executable("examples/$example_name.C", 'blib/example_name',
			include_dirs         => [ 'perl++/headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib' ],
			'C++'                => 1,
		);
	}
	for my $example_name (@{$examples{libraries}}) {
		build_library($example_name, {
			input                =>  [ "$example_name.C" ],
			input_dir            => 'examples',
			include_dirs         => [ 'perl++/headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib' ],
			libfile              => "blib/$example_name.$Config{dlext}",
			'C++'                => 1,
		});
	}
	return;
}

sub build_tests {
	for my $test_source (sort keys %tests) {
		build_executable($test_source, $tests{$test_source},
			include_dirs         => [ qw(perl++/headers tap++/headers) ],
			libs                 => [ qw/perl++ tap++/ ],
			libdirs              => [ 'blib/lib' ],
			'C++'                => 1,
		);
	}
	return;
}

sub dispatch {
	my ($action_name, @arguments) = @_;

	for my $argument (@arguments) {
		if ($argument =~ / ^ (\w+) = (.*) $ /xms) {
			$options{$1} = $2;
		}
		else {
			$options{$argument} = 1;
		}
	}

	make_silent($options{silent}) if $options{silent};
	my @test_goals = $options{test_files} ? split / /, $options{test_files} : sort values %tests;

	my %action_map = (
		build     => \&build_perl,
		build_tap => \&build_tap,
		test      => sub {
			build_perl;
			build_tap;
			build_tests;

			run_tests(\%options, @test_goals)
		},
		testbuild => sub {
			build_perl;
			build_tap;
			build_tests;
		},
		examples  => sub {
			build_perl;
			build_examples;
		},
		install   => sub {
			build_perl;
			build_tap;

			my $incdir = $options{incdir}  || $Config{usrinc};
			install([
				from_to => {
					'blib/lib'       => $options{libdir}  || (split ' ', $Config{libpth})[0],
					'perl++/headers' => "$incdir/perl++",
					'lib'            => $options{moddir} || $Config{installprivlib},
				},
				verbose => $options{silent} <= 0,
				dry_run => $options{dry_run},
			]);
		},
		clean     => sub {
			remove_tree(\%options, @cleanfiles);
		},
		realclean => sub {
			remove_tree(\%options, @cleanfiles, 'Build');
		},
		testclean => sub {
			remove_tree(\%options, @testcleanfiles);
		},
		help      => sub {
			print "No help available yet\n";
		},
	);

	my $action = $action_map{ $action_name } or die "No such action defined\n";
	return $action->();
}

1;
