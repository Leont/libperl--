# perl
use strict;
use warnings;

use lib 'inc';
use Perlpp::Build 'write_build';

my $VERSION = 0.003;

write_build($VERSION);
