package Library::Build::Test;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use File::Spec::Functions qw/catfile catdir splitdir/;
use POSIX qw/strftime/;
use TAP::Harness;

my %test_methods = (
	run_tests => sub {
		my ($self, @test_goals) = @_;
		my $library_var = $self->stash('library_var') || $self->config('ldlibpthname');
		printf "Report %s\n", strftime('%y%m%d-%H:%M', localtime) if $self->stash('verbose') > -2;
		my $harness = TAP::Harness->new({
			verbosity => $self->stash('verbose'),
			exec => sub {
				my (undef, $file) = @_;
				return -B $file ? [ $file ] : [ $^X, '-I' . catdir(qw/blib lib/), $file ];
			},
			merge => 1,
			color => -t STDOUT,
		});

		my $ld_dir = catdir(qw/blib so/);
		local $ENV{$library_var} = $ld_dir if -d $ld_dir;
		return $harness->runtests(@test_goals);
	},
);

my %test_actions = (
	testbuild => sub {
		my $builder = shift;
		$builder->dispatch('build');
	},
	test      => sub {
		my $builder = shift;

		$builder->dispatch('testbuild');
		my $exe = $builder->config('_exe');
		my @tests = defined $builder->stash('test_file') ? @{ $builder->stash('test_file') } : $builder->find_files('t', qr/ \. t (?:$exe)? \z /xms);
		$builder->run_tests(@tests);
	},
	testclean => sub {
		my $builder = shift;
		$builder->remove_dirty_files('test');
	},
);

sub mixin {
	my $builder = shift;

	$builder->inject_roles({ test => \%test_methods });
	$builder->register_actions(%test_actions);
	$builder->register_dirty(test => [ '_build/t' ]);
	$builder->register_argument(library_var => 1, test_file => 2);
	return;
}

1;
