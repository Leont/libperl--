package Library::Build::PL;

use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp qw/croak/;
use Data::Dumper;
use Exporter 5.57 'import';
use File::Copy qw/copy/;
use File::Spec::Functions 'catfile';
use FindBin;

our @EXPORT = qw/write_build write_mymeta check_compiler check_cplusplus/;

sub new {
	my $class = shift;
	return bless {}, $class;
}

sub write_build {
	my ($self, @args) = @_;
	my %args = (
		builder  => 'Library::Build',
		function => 'new',
		argv     => \@ARGV,
		inc      => [ 'inc' ],
		version  => '0.001',
		@args
	);
	$args{filename} ||= do {
		my $filename = $FindBin::Script;
		$filename =~ s/\.PL$// or croak "Can't parse filefilename '$filename'";
		$filename;
	};

	open my $fh, '>', $args{filename} or die "Can't open file '$args{filename}': $!\n";

	local $Data::Dumper::Terse  = 1;
	local $Data::Dumper::Indent = 0;
	my $arguments = Dumper($args{argv});
	print {$fh} <<"EOF" or croak 'Can\'t write Build: $!';
#! $^X

use strict;
use warnings;

use lib qw{@{$args{inc}}};
use $args{builder};
my \$VERSION = '$args{version}';

my \$builder = $args{builder}->$args{function}(\\\@ARGV, $arguments, \$VERSION);
\$builder->dispatch_default();
EOF

	my $current_mode = (stat $fh)[2] or croak "Can't stat '$args{filename}': $!\n";
	chmod $current_mode | oct(111), $fh or croak "Can't make '$args{filename}' executable: $!\n";
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
