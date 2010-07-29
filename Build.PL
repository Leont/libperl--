# perl
use strict;
use warnings;

use lib 'inc';
use Library::Build::PL;

my $VERSION = 0.003;

my $buildpl = Library::Build::PL->new(name => 'libperl++', version => $VERSION, mixin => [ 'Perlpp::Build' ], inc => [ qw/inc/ ]);
$buildpl->check_cplusplus;
$buildpl->write_build;
$buildpl->write_mymeta;
