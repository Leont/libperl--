package Library::Build::Base;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Archive::Tar;
use Carp 'croak';
use Config;
use ExtUtils::Install qw/install/;
use ExtUtils::Manifest qw/maniread manicheck mkmanifest/;
use File::Basename qw/basename dirname/;
use File::Spec::Functions qw/catfile splitdir/;
use Readonly ();

Readonly::Scalar my $NONREADABLE => ~oct 22;

my %default_actions = (
	lib       => sub {
		my $builder = shift;
		for my $ext (qw/pm pod/) {
			for my $file ($builder->find_files('lib', qr/ \. $ext \z /xms)) {
				$builder->copy_files($file, catfile('blib', $file));
				my @directories = splitdir(dirname($file));
				shift @directories;
				my $base = basename($file, $ext);
				$builder->pod2man($file, catfile('blib', 'libdoc', join('::', @directories, $base) . '3'));
			}
		}
	},
	build     => sub {
		my $builder = shift;
		$builder->dispatch('lib');
	},
	testbuild => sub {
		my $builder = shift;
		$builder->dispatch('build');
	},
	test      => sub {
		my $builder = shift;

		$builder->dispatch('testbuild');
		my @tests = defined $builder->stash('test_file') ? @{ $builder->stash('test_file') } : $builder->find_files('t', qr/ \. t (?:$Config{_exe})? \z /xms);
		$builder->run_tests(@tests);
	},
	install   => sub {
		my $builder = shift;
		$builder->dispatch('build');

		install([
			from_to => $builder->{install_paths},
			verbose => $builder->stash('verbose') >= 0,
			dry_run => $builder->stash('dry_run'),
		]);
	},
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
	manifest  => sub {
		mkmanifest();
	},
	distcheck => sub {
		my @missing = manicheck();
		croak "Missing files @missing" if @missing;
	},
	help      => sub {
		my $builder = shift;
		print "Available commands:\n";
		for my $command (sort keys %{$builder->{action_map}}) {
			print "$command\n";
		}
	},
	clean     => sub {
		my $builder = shift;
		$builder->remove_dirty_files($builder->stash('what') || 'all');
	},
	realclean => sub {
		my $builder = shift;
		$builder->dispatch('clean');
		$builder->remove_tree('Build');
	},
	testclean => sub {
		my $builder = shift;
		$builder->remove_dirty_files('test');
	},
);

sub mixin {
	my $builder = shift;
	$builder->register_actions(%default_actions);
	$builder->register_dirty(
		binary => [ qw/blib _build/ ],
		meta   => [ 'MYMETA.yml' ],
		test   => [ '_build/t' ],
		tarball  => [ $builder->name . '' . $builder->version . '.tar.gz' ],
	);
	$builder->register_argument($_, 0) for qw/verbose dry_run/;
	$builder->register_argument($_, 1) for qw/library_var what/;
	$builder->register_argument($_, 2) for qw/include_dir test_file/;
	$builder->register_paths(
		'so'      => $builder->arg('libdir') || (split ' ', $Config{libpth})[0],
		'headers' => $builder->arg('incdir') || $Config{usrinc},
		'lib'     => $builder->arg('moddir') || $Config{installsitelib},
	);
	return;
}

1;
