#! /usr/bin/perl -T

use 5.008;
use strict;
use warnings;

use DynaLoader;
use Config;

unshift @DynaLoader::dl_library_path, 'blib';

my $file = DynaLoader->dl_findfile("extend.$Config{dlext}") or die DynaLoader::dl_error;

my $handle = DynaLoader::dl_load_file($file, 0x01) or die DynaLoader::dl_error();
my $perlpp_exporter = DynaLoader::dl_find_symbol($handle, 'perlpp_exporter') or die DynaLoader::dl_error;
my $sub = DynaLoader::dl_install_xsub('Perl++::exporter', $perlpp_exporter, 'filename') or die DynaLoader::dl_error;
$sub->();
