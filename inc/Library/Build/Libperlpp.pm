package Library::Build::Libperlpp;

# This code needs some serious refactoring, but it works…

use 5.008;
use strict;
use warnings;

our $VERSION = 0.003;

use Exporter 5.57 qw/import/;
our @EXPORT_OK = qw/create_builder/;

use Carp qw/croak/;
use Config;

use ExtUtils::Embed qw/ldopts/;

use Library::Build;

my %cpp_files = (
	'perl++/source/config.pre' => 'perl++/headers/config.h',
	'perl++/source/extend.pre' => 'perl++/headers/extend.h',
);

my %examples = (
	executables => [ qw/combined game/ ],
	libraries   => [ qw/Extend/ ]
);

sub test_map {
	my $self = shift;
	if ($self->{test_files}) {
		return map { (my $source = $_) =~ s/ .t (?:$Config{_exe})? \z /.C/xms; -e $source ? ($source => $_) : () } split / /, $self->{test_files};
	}
	else {
		return map { (my $test = $_) =~ s/ .C \z /.t$Config{_exe}/xms; ( $_ => $test ) } glob 't/*.C';
	}
}

my %action_map = (
	source    => sub {
		my $builder = shift;
		while (my ($input, $output) = each %cpp_files) {
			$builder->process_cpp($input, $output);
		}
		
		$builder->process_perl('perl++/source/evaluate.C.PL', 'perl++/source/evaluate.C');
	},
	'perl++'  => sub {
		my $builder = shift;
		$builder->dispatch('source');
		$builder->dispatch('dirs');

		$builder->copy_files('perl++/headers', 'blib/headers/perl++');

		$builder->build_library(
			name          => 'perl++',
			input_dir     => 'perl++/source',
			linker_append => ldopts,
			include_dirs  => [ qw(blib/headers source) ],
			'C++'         => 1,
		);
	},
	tap       => sub {
		my $builder = shift;
		$builder->dispatch('dirs');

		$builder->copy_files('tap++/headers', 'blib/headers/tap++');
		$builder->build_library(
			name         => 'tap++',
			input_dir    => 'tap++/source',
			include_dirs => [ qw(blib/headers) ],
			'C++'        => 1,
		);
	},
	build     => sub { 
		my $builder = shift;
		$builder->dispatch('perl++');
		$builder->dispatch('lib');
		$builder->dispatch('tap');
	},
	examples  => sub {
		my $builder = shift;
		$builder->dispatch('build');

		for my $example_name (@{$examples{executables}}) {
			$builder->build_executable("examples/$example_name.C", "examples/$example_name",
				include_dirs         => [ 'blib/headers' ],
				libs                 => [ 'perl++' ],
				libdirs              => [ 'blib/arch' ],
				'C++'                => 1,
			);
		}
		for my $example_name (@{$examples{libraries}}) {
			$builder->build_library(
				name                 => $example_name,
				input_files          => [ "$example_name.C" ],
				input_dir            => 'examples',
				include_dirs         => [ 'blib/headers' ],
				libs                 => [ 'perl++' ],
				libdirs              => [ 'blib/arch' ],
				libfile              => "examples/$example_name\.$Config{dlext}",
				'C++'                => 1,
			);
		}
	},
	testbuild => sub {
		my $builder = shift;
		$builder->dispatch('build');

		my %test_executable_for = test_map($builder);
		for my $test_source (sort keys %test_executable_for) {
			$builder->build_executable($test_source, $test_executable_for{$test_source},
				include_dirs         => [ qw(blib/headers) ],
				libs                 => [ qw/perl++ tap++/ ],
				libdirs              => [ 'blib/arch' ],
				'C++'                => 1,
			);
		}
	},
);

sub create_builder {
	my ($arguments, $cached, $version) = @_;

	my $builder = Library::Build->new($arguments, $cached, 'libperl++', $version);
	$builder->register_actions(%action_map);
	$builder->register_dirty(
		test     => [ glob 't/*0-*.t' ],
		source   => [ qw{source/ppport.h source/evaluate.C perl++/headers/config.h perl++/headers/extend.h} ],
		examples => [ qw{examples/combined examples/game examples/Extend.so} ],
	);
	return $builder;
}

1;
