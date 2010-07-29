# perl
use strict;
use warnings;

use lib 'inc';
use Library::Build::PL;

my $VERSION = 0.003;

my $buildpl = Library::Build::PL->new;
$buildpl->check_cplusplus();
$buildpl->write_build(name => 'libperl++', version => $VERSION, mixin => [ 'Perlpp::Build' ]);
$buildpl->write_mymeta();
