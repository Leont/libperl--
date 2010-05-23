# perl
use strict;
use warnings;

my $name = $0;

$name =~ s/\.PL$// or die "Can't parse filename '$name'\n";

open my $fh, '>', $name or die "Can't open file '$name': $!\n";

print {$fh} <<'EOF';
#! /usr/bin/perl

use strict;
use warnings;

use lib 'inc';
use Library::Build;

dispatch(@ARGV);
EOF

my $current_mode = (stat $name)[2] or die "Can't stat '$name': $!\n";
chmod $current_mode | oct(111), $name or die "Can't make '$name' executable: $!\n";

close $fh or die "Can't close filehandle: $!\n";

