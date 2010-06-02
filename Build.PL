# perl
use strict;
use warnings;

use File::Copy qw/copy/;
use FindBin;
use Data::Dumper;
$Data::Dumper::Terse = 1;
$Data::Dumper::Indent = 0;

use ExtUtils::CBuilder;
my $builder = ExtUtils::CBuilder->new;
die "You don't seem to have a working C++ compiler" if not $builder->have_cplusplus;

my $name = $0;

$name =~ s/\.PL$// or die "Can't parse filename '$name'\n";

open my $fh, '>', $name or die "Can't open file '$name': $!\n";

my $arguments = Dumper(\@ARGV);
print {$fh} <<"EOF";
#! $^X

use strict;
use warnings;

use lib 'inc';
use Library::Build;

dispatch(\\\@ARGV, $arguments);
EOF

my $current_mode = (stat $name)[2] or die "Can't stat '$name': $!\n";
chmod $current_mode | oct(111), $name or die "Can't make '$name' executable: $!\n";

close $fh or die "Can't close filehandle: $!\n";

copy "$FindBin::Bin/META.yml", "$FindBin::Bin/MYMETA.yml";
