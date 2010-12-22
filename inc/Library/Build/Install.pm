package Library::Build::Install;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Config;
use ExtUtils::Install qw/install/;

my %install_dirs_for = (
	core   => {
		lib     => $Config{installprivlib},
		arch    => $Config{installarchlib},
		bin     => $Config{installbin},
		script  => $Config{installscript},
		bindoc  => $Config{installman1dir},
		libdoc  => $Config{installman3dir},
		binhtml => $Config{installhtml1dir},
		libhtml => $Config{installhtml3dir},
	},
	site   => {
		lib     => $Config{installsitelib},
		arch    => $Config{installsitearch},
		bin     => $Config{installsitebin},
		script  => $Config{installsitescript},
		bindoc  => $Config{installsiteman1dir},
		libdoc  => $Config{installsiteman3dir},
		binhtml => $Config{installsitehtml1dir},
		libhtml => $Config{installsitehtml3dir},
	},
	vendor => {
		lib     => $Config{installvendorlib},
		arch    => $Config{installvendorarch},
		bin     => $Config{installvendorbin},
		script  => $Config{installvendorscript},
		bindoc  => $Config{installvendorman1dir},
		libdoc  => $Config{installvendorman3dir},
		binhtml => $Config{installvendorhtml1dir},
		libhtml => $Config{installvendorhtml3dir},
	},
);

my %install_base_relpaths = (
	lib     => ['lib', 'perl5'],
	arch    => ['lib', 'perl5', $Config{archname}],
	bin     => ['bin'],
	script  => ['bin'],
	bindoc  => ['man', 'man1'],
	libdoc  => ['man', 'man3'],
	binhtml => ['html'],
	libhtml => ['html'],
);

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

		install([
			from_to => $builder->{install_paths},
			verbose => $builder->stash('verbose') >= 0,
			dry_run => $builder->stash('dry_run'),
		]);
	},
);

sub mixin {
	my $builder = shift;

	$builder->inject_roles({ install => \%install_methods });
	$builder->register_actions(%install_actions);

	$builder->register_argument(
		install_path => sub {
			my (undef, undef, $arguments) = @_;
			my $arg = shift @{$arguments};
			my ($name, $value) = $arg =~ / (\w+) = (.*) /x;
			$builder->register_paths($name => $value);
		},
		installdirs => sub {
			my (undef, undef, $arguments) = @_;
			my $type = shift @{$arguments};
			$builder->register_paths($install_dirs_for{$type});
			return;
		},
		install_base => sub {
			my (undef, undef, $arguments) = @_;
			my $base_path = shift @{$arguments};
			my %path_for;
			for my $typename (keys %install_base_relpaths) {
				$path_for{$typename} = catdir($base_path, @{ $install_base_relpaths{$typename} });
			}
			$builder->register_paths(%path_for);
		},
		dry_run => 0,
	);
	$builder->register_paths(%{ $install_dirs_for{'site'} });
	return;
}

1;
