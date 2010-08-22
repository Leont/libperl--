package Perlpp::Build;

use 5.008;
use strict;
use warnings;

our $VERSION = 0.003;

use Carp qw/croak/;
use Config;
use ExtUtils::Embed qw/ldopts/;
use File::Spec::Functions qw/catfile/;

use Library::Build;

sub portable {
	my @args = @_;
	my @ret = map { catfile(split m{/}, $_) } @args;
	return wantarray ? @ret : $ret[0];
}

my %cpp_files = (
	portable('perl++/source/config.pre') => portable('perl++/headers/config.h'),
	portable('perl++/source/extend.pre') => portable('perl++/headers/extend.h'),
);

my %examples = (
	executables => [ qw/combined game/ ],
	libraries   => [ qw/Extend Extend2/ ]
);

sub test_map {
	my $self = shift;
	if ($self->stash('test_file')) {
		return map { (my $source = $_) =~ s/ .t (?:$Config{_exe})? \z /.C/xms; -e $source ? ($source => $_) : () } @{ $self->stash('test_file') };
	}
	else {
		return map { (my $test = $_) =~ s/ .C \z /.t$Config{_exe}/xms; ( $_ => $test ) } glob portable('t/*.C');
	}
}

my %action_map = (
	source    => sub {
		my $builder = shift;
		while (my ($input, $output) = each %cpp_files) {
			$builder->process_cpp($input, $output);
		}
		
		$builder->process_perl(portable('perl++/source/evaluate.C.PL'), portable('perl++/source/evaluate.C'));
	},
	'perl++'  => sub {
		my $builder = shift;
		$builder->dispatch('source');

		$builder->copy_files(portable('perl++/headers'), portable('blib/headers/perl++'));

		$builder->build_library(
			name          => 'perl++',
			input_dir     => portable('perl++/source'),
			linker_append => ldopts,
			include_dirs  => [ portable(qw(blib/headers source)) ],
			'C++'         => 1,
		);
	},
	tap       => sub {
		my $builder = shift;

		$builder->copy_files('tap++/headers', 'blib/headers/tap++');
		$builder->build_library(
			name         => 'tap++',
			input_dir    => portable('tap++/source'),
			include_dirs => [ portable(qw(blib/headers)) ],
			'C++'        => 1,
		);
	},
	build     => sub { 
		my ($builder, $next) = @_;
		$builder->dispatch('perl++');
		$builder->dispatch('tap');
		$builder->dispatch_next($next);
	},
	examples  => sub {
		my $builder = shift;
		$builder->dispatch('build');

		for my $example_name (@{$examples{executables}}) {
			$builder->build_executable(
				output       => catfile('examples', $example_name),
				input_files  => [ catfile('examples', "$example_name.C") ],
				include_dirs => [ portable('blib/headers') ],
				libs         => [ 'perl++' ],
				libdirs      => [ portable('blib/so') ],
				'C++'        => 1,
			);
		}
		for my $example_name (@{$examples{libraries}}) {
			$builder->build_library(
				name         => $example_name,
				input_files  => [ catfile('examples', "$example_name.C") ],
				include_dirs => [ portable('blib/headers') ],
				libs         => [ 'perl++' ],
				libdirs      => [ portable('blib/so') ],
				libfile      => catfile('examples', "$example_name\.$Config{dlext}"),
				'C++'        => 1,
			);
		}
	},
	testbuild => sub {
		my ($builder, $next) = @_;
		$builder->dispatch_next($next);

		my %test_executable_for = test_map($builder);
		for my $test_source (sort keys %test_executable_for) {
			$builder->build_executable(
				output       => $test_executable_for{$test_source},
				input_files  => [ $test_source ] ,
				include_dirs => [ portable('blib/headers') ],
				libs         => [ qw/perl++ tap++/ ],
				libdirs      => [ portable('blib/so') ],
				'C++'        => 1,
			);
		}
	},
);

sub mixin {
	my $builder = shift;
	$builder->register_actions(%action_map);
	$builder->register_dirty(
		test     => [ glob 't/*0-*.t' ],
		source   => [ portable(qw{source/ppport.h perl++/source/evaluate.C perl++/headers/config.h perl++/headers/extend.h}) ],
		examples => [ portable(qw{examples/combined examples/game examples/Extend.so}) ],
	);
	return;
}

1;
