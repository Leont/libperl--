package Library::Build::PL;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use B::Deparse;
use Carp qw/croak/;
use Data::Dumper;
use File::Copy qw/copy/;
use File::Spec::Functions qw/catfile/;
use FindBin;

use Library::Build::Config;

my %registries = (
	dirty_files   => 'register_dirty',
	install_paths => 'register_paths',
);

sub new {
	my ($class, @args) = @_;
	my %args = (
		argv   => \@ARGV,
		config => [ map { @{$_} } grep { $_ }  @{Library::Build::Config::read_config('Build_PL')}{'*', 'Build_PL'} ],
		@args
	);
	return bless \%args, $class;
}

sub write_build {
	my $self = shift;
	my $filename = shift || "$FindBin::Bin/Build";

	open my $fh, '>', $filename or die "Can't open file '$filename': $!\n";

	local $Data::Dumper::Terse  = 1;
	local $Data::Dumper::Indent = 0;
	print {$fh} "$_\n" for ("#! $^X", '', 'use strict;', 'use warnings;', 'use FindBin;', 'BEGIN { chdir $FindBin::Bin };');
	printf {$fh} "use lib %s;\n", join ', ', Dumper(@{ $self->{inc} }) if $self->{inc};
	print {$fh} "use Library::Build;\n";
	print {$fh} "use $_;\n" for @{ $self->{use} };
	print {$fh} "\n";

	print {$fh} "my \$builder = Library::Build->new('$self->{name}', '$self->{version}');\n";
	if (!$self->{skip_base}) {
		printf {$fh} "\$builder->mixin(%s);\n", join ',', map { "'Library::Build::$_'" } qw/Util Install Build Compile Test Author/;
	}
	printf {$fh} "\$builder->mixin(%s);\n", join ', ', Dumper(@{ $self->{mixin} }) if $self->{mixin};
	printf {$fh} "\$builder->parse({ argv => \\\@ARGV, cached => %s });\n", Dumper($self->{argv});
	if (defined $self->{action_map}) {
		my $deparser = B::Deparse->new;
		while (my ($key, $subs) = each %{ $self->{action_map} }) {
			for my $sub (@{$subs}) {
				printf {$fh} "\$builder->register_actions('%s' => sub %s);\n", $key, $deparser->coderef2text($sub);
			}
		}
	}
	while (my ($key, $method) = each %registries) {
		printf {$fh} "\$builder->$method(%%{ %s });\n", Dumper($self->{$key}) if defined $self->{$key};
	}
	print {$fh} "$self->{extra}\n" if $self->{extra};
	print {$fh} "\$builder->dispatch_default();\n";
	my $current_mode = (stat $fh)[2] or croak "Can't stat '$self->{filename}': $!\n";
	chmod $current_mode | oct(111), $fh or croak "Can't make '$self->{filename}' executable: $!\n" unless $^O eq 'MSWin32';
	close $fh or die "Can't close filehandle: $!\n";

	return;
}

sub write_mymeta {
	my $self = shift;
	my $base = shift || $FindBin::Bin;
	copy(catfile($FindBin::Bin, 'META.yml'), catfile($base, 'MYMETA.yml'));
	return;
}

sub check_compiler {
	my $self = shift;
	require ExtUtils::CBuilder;
	my $builder = ExtUtils::CBuilder->new;
	croak 'You don\'t seem to have a working C compiler' if not $builder->have_compiler;
	return;
}

sub check_cplusplus {
	my $self = shift;
	require ExtUtils::CBuilder;
	my $builder = ExtUtils::CBuilder->new;
	croak 'You don\'t seem to have a working C++ compiler' if not $builder->have_cplusplus;
	return;
}

sub register_dirty {
	my ($self, %files_map) = @_;
	while (my ($category, $files) = each %files_map) {
		push @{ $self->{dirty_files}{$category} }, @{$files};
	}
	return;
}

sub register_actions {
	my ($self, %action_map) = @_;
	while (my ($name, $sub) = each %action_map) {
		push @{ $self->{action_map}{$name} }, $sub;
	}
	return;
}

sub register_paths {
	my ($self, %paths) = @_;
	while (my ($source, $destination) = each %paths) {
		$self->{install_paths}{$source} = $destination;
	}
	return;
}

1;
