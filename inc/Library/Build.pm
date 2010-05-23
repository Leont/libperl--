package Library::Build;

use 5.008;
use strict;
use warnings;

use Library::Build::Util qw/:all/;
use Config;
use ExtUtils::Embed qw/ldopts/;

use Exporter 5.57 qw/import/;

our @EXPORT = qw/dispatch/;

my @testcleanfiles = glob 't/*.[ot]';
my @cleanfiles = (qw{/examples/combined source/ppport.h source/evaluate.C headers/config.h blib}, @testcleanfiles);

my %libraries = (
	'perl++' => {
		input => [ qw/array.C call.C evaluate.C exporter.C glob.C hash.C handle.C helpers.C interpreter.C primitives.C reference.C regex.C scalar.C/ ],
		input_dir => 'source',
		linker_append => ldopts,
		include_dirs => [ qw/headers source/ ],
		'C++' => 1,
	},
	'tap++' => {
		input => 'tap++.C',
		input_dir => 'source',
		include_dirs => [ qw/headers/ ],
		'C++' => 1,
	},
);

my %options = (
	silent => 0,
);

sub build {
	create_dir(\%options, 'blib');

	create_by_system { 
		(my $oldname = $_ ) =~ s{ headers / (\w+) \.h  }{source/$1.pre}x;
		"$Config{cpp} $Config{ccflags} -I$Config{archlibexp}/CORE $oldname > $_";
	} \%options, qw{headers/config.h headers/extend.h};

	create_by_system { "$^X -T $_.PL > $_" } \%options, 'source/evaluate.C';

	for my $library_name (keys %libraries) {
		build_library($library_name => $libraries{$library_name});
	}
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
			include_dirs         => [ 'headers' ],
			libs                 => [ 'perl++' ],
			libdirs              => [ 'blib' ],
			'C++'                => 1,
		);
	}
	for my $example_name (@{$examples{libraries}}) {
		build_library($example_name, {
			input                =>  [ "$example_name.C" ],
			input_dir            => 'examples',
			include_dirs         => [ 'headers' ],
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
			include_dirs         => [ 'headers' ],
			libs                 => [ qw/perl++ tap++/ ],
			libdirs              => [ 'blib' ],
			'C++'                => 1,
		);
	}
	return;
}


sub dispatch {
	my @arguments = @_;
	my $action_name = shift @arguments|| 'build';



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
		build     => \&build,
		test      => sub {
			build;
			build_tests;

			run_tests(\%options, @test_goals)
		},
		testbuild => sub {
			build;
			build_tests;
		},
		examples  => sub {
			build;
			build_examples;
		},
		install   => sub {
			build;

			die "install not implemented yet\n";
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
