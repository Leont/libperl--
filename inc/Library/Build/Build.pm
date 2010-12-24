package Library::Build::Build;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp 'croak';
use Config;
use File::Basename qw/basename dirname/;
use File::Spec::Functions qw/catfile splitdir/;
use Pod::Man;

my %build_methods = (
	pod2man => sub {
		my ($self, $source, $dest) = @_;
		return if -e $dest and -M $source > -M $dest;
		$self->create_dir(dirname($dest));
		print "pod2man $source $dest\n" if $self->stash('verbose') >= 0;
		$self->{pod_parser} ||= Pod::Man->new;
		$self->{pod_parser}->parse_from_file($source, $dest);
		return;
	},
);

my %build_actions = (
	lib       => sub {
		my $builder = shift;
		for my $ext (qw/pm pod/) {
			for my $file ($builder->find_files('lib', qr/ \. $ext \z /xms)) {
				$builder->copy_files($file, catfile('blib', $file));
				my @directories = splitdir(dirname($file));
				shift @directories;
				my $base = basename($file, $ext);
				$builder->pod2man($file, catfile('blib', 'libdoc', join('::', @directories, $base) . '3')) if $^O ne 'MSWin32';
			}
		}
	},
	build     => sub {
		my $builder = shift;
		$builder->dispatch('lib');
	},
);

sub mixin {
	my $builder = shift;

	$builder->inject_roles({ build => \%build_methods });
	$builder->register_actions(%build_actions);
	$builder->register_dirty(binary  => [qw/blib/]);
	return;
}

1;
