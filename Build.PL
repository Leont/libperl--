# perl
use strict;
use warnings;

use lib 'inc';
use Library::Build::PL;

my $VERSION = 0.003;

my $buildpl = Library::Build::PL->new;
$buildpl->check_cplusplus();
$buildpl->write_build(builder => 'Perlpp::Build', function => 'create_builder', version => $VERSION);
$buildpl->write_mymeta();
