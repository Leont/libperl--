package Library::Build::PL;

use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp qw/croak/;
use Data::Dumper;
use File::Copy qw/copy/;
use File::Spec::Functions 'catfile';
use FindBin;

sub new {
	my ($class, @args) = @_;
	my %args = (
		mixin    => [],
		argv     => \@ARGV,
		inc      => [ 'inc' ],
		version  => '0.001',
		extra    => '',
		@args
	);
	$args{filename} ||= do {
		my $filename = $FindBin::Script;
		$filename =~ s/\.PL$// or croak "Can't parse filefilename '$filename'";
		$filename;
	};
	return bless \%args, $class;
}

sub write_build {
	my ($self, @args) = @_;

	open my $fh, '>', $self->{filename} or die "Can't open file '$self->{filename}': $!\n";

	my $arguments = Dumper($self->{argv});
	local $Data::Dumper::Terse  = 1;
	local $Data::Dumper::Indent = 0;
	my $inc       = join ', ', Dumper(@{$self->{inc}});
	my $mixin     = join ', ', Dumper(@{$self->{mixin}});
	print {$fh} <<"EOF";# or croak 'Can\'t write Build: $!';
#! $^X

use strict;
use warnings;

use lib $inc;
use Library::Build;
my \$module  = '$self->{name}';
my \$version = '$self->{version}';

my \$builder = Library::Build->new(\$module, \$version, { argv => \\\@ARGV, cached => $arguments });
\$builder->mixin($mixin);
\$builder->dispatch_default();
$self->{extra}
EOF

	my $current_mode = (stat $fh)[2] or croak "Can't stat '$self->{filename}': $!\n";
	chmod $current_mode | oct(111), $fh or croak "Can't make '$self->{filename}' executable: $!\n";
	close $fh or die "Can't close filehandle: $!\n";

	return;
}

sub write_mymeta {
	my $self = shift;
	my $base = shift || $FindBin::Bin;
	copy(catfile($FindBin::Bin, 'META.yml'), catfile($FindBin::Bin, 'MYMETA.yml'));
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

1;
