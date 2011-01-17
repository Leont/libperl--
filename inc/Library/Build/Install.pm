package Library::Build::Install;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use ExtUtils::Install qw/install uninstall/;
use File::Spec::Functions qw/catfile/;

sub install_dirs_for {
	my ($builder, $location) = @_;
	my %install_dirs_for = (
		core   => {
			lib     => $builder->config('installprivlib'),
			arch    => $builder->config('installarchlib'),
			bin     => $builder->config('installbin'),
			script  => $builder->config('installscript'),
			bindoc  => $builder->config('installman1dir'),
			libdoc  => $builder->config('installman3dir'),
			binhtml => $builder->config('installhtml1dir'),
			libhtml => $builder->config('installhtml3dir'),
		},
		site   => {
			lib     => $builder->config('installsitelib'),
			arch    => $builder->config('installsitearch'),
			bin     => $builder->config('installsitebin'),
			script  => $builder->config('installsitescript'),
			bindoc  => $builder->config('installsiteman1dir'),
			libdoc  => $builder->config('installsiteman3dir'),
			binhtml => $builder->config('installsitehtml1dir'),
			libhtml => $builder->config('installsitehtml3dir'),
		},
		vendor => {
			lib     => $builder->config('installvendorlib'),
			arch    => $builder->config('installvendorarch'),
			bin     => $builder->config('installvendorbin'),
			script  => $builder->config('installvendorscript'),
			bindoc  => $builder->config('installvendorman1dir'),
			libdoc  => $builder->config('installvendorman3dir'),
			binhtml => $builder->config('installvendorhtml1dir'),
			libhtml => $builder->config('installvendorhtml3dir'),
		},
	);
	return $install_dirs_for{$location};
}

my %install_methods = (
	register_paths => sub {
		my ($self, %paths) = @_;
		while (my ($source, $destination) = each %paths) {
			$self->{install_paths}{"blib/$source"} = $destination;
		}
		return;
	},
);

my %install_actions = (
	install   => sub {
		my $builder = shift;
		$builder->dispatch('build');

		my %from_to = %{ $builder->{install_paths} };
		if (my $destdir = $builder->stash('dest_dir')) {
			for my $destination (values %from_to) {
				$destination = catfile($destdir, $destination);
			}
		}

		install(\%from_to, $builder->stash('verbose') >= 0, $builder->stash('dry_run'), $builder->stash('uninst'));
	},
);

sub mixin {
	my $builder = shift;

	$builder->inject_roles({ install => \%install_methods });
	$builder->register_actions(%install_actions);

	$builder->register_argument(
		install_path => sub($) {
			my ($arg) = @_;
			my ($name, $value) = $arg =~ / (\w+) = (.*) /x;
			$builder->register_paths($name => $value);
		},
		installdirs  => sub($) {
			my ($type) = @_;
			$builder->register_paths(%{ install_dirs_for($builder, $type) });
			return;
		},
		install_base => sub($) {
			my ($base_path) = @_;

			my %install_base_relpaths = (
				lib     => ['lib', 'perl5'],
				arch    => ['lib', 'perl5', $builder->config('archname')],
				bin     => ['bin'],
				script  => ['bin'],
				bindoc  => ['man', 'man1'],
				libdoc  => ['man', 'man3'],
				binhtml => ['html'],
				libhtml => ['html'],
			);

			my %path_for;
			for my $typename (keys %install_base_relpaths) {
				$path_for{$typename} = catdir($base_path, @{ $install_base_relpaths{$typename} });
			}
			$builder->register_paths(%path_for);
		},
		dest_dir     => 1,
		dry_run      => 0,
		uninst       => 0,
	);
	$builder->register_paths(%{ install_dirs_for($builder, 'site') });
	return;
}

1;
