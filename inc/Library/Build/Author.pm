package Library::Build::Author;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Archive::Tar;
use Carp 'croak';
use Config;
use ExtUtils::Manifest qw/maniread fullcheck mkmanifest manicopy/;
$ExtUtils::Manifest::Quiet = 1;

my $NONREADABLE = ~oct 22;

my %author_actions = (
	dist      => sub {
		my $builder = shift;
		$builder->dispatch('build');
		my $manifest = maniread() or croak 'No MANIFEST found';
		my @files = keys %{$manifest};
		my $arch = Archive::Tar->new;
		$arch->add_files(@files);
		$_->mode($_->mode & $NONREADABLE) for $arch->get_files;
		my $release_name = $builder->name . '-' . $builder->version;
		print "tar xjf $release_name.tar.gz @files\n" if $builder->stash('verbose') >= 0;
		$arch->write("$release_name.tar.gz", COMPRESS_GZIP, $release_name);
	},
	distdir   => sub {
		my $builder = shift;
		$builder->dispatch('build');
		my $manifest = maniread() or croak 'No MANIFEST found';
		my $release_name = $builder->name . '-' . $builder->version;
		$builder->create_dir($release_name);
		manicopy($manifest, $release_name, 'cp');
	},
	manifest  => sub {
		mkmanifest();
	},
	distcheck => sub {
		my ($missing, $extra) = fullcheck();
		croak "Missing on filesystem: @{$missing}" if @{$missing};
		croak "Missing in MANIFEST: @{$extra}" if @{$extra}
	},
);

sub mixin {
	my $builder = shift;

	$builder->register_actions(%author_actions);

	$builder->register_dirty(tarball => [ $builder->name . '' . $builder->version . '.tar.gz' ]);
	return;
}

1;
