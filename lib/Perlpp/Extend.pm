package Perlpp::Extend;

use 5.008;
use strict;
use warnings;

use DynaLoader;
use Config;
#use Carp qw/die/;

sub import {
	my $package = shift;
	my $module  = shift || caller;
	load_module($module);
}

sub load_module {
	my $module = shift;
	(my $file_rel = $module) =~ s{::|'}{/}g;
	my $file = DynaLoader->dl_findfile("$file_rel.$Config{dlext}") or die "Can't find $file_rel.$Config{dlext}: ".DynaLoader::dl_error;

	my $handle = DynaLoader::dl_load_file($file, 0x01) or die DynaLoader::dl_error();
	my $perlpp_exporter = DynaLoader::dl_find_symbol($handle, 'perlpp_exporter') or die DynaLoader::dl_error;
	my $sub = DynaLoader::dl_install_xsub('Perl++::exporter', $perlpp_exporter, $file) or die DynaLoader::dl_error;
	$sub->($module) or die 'Something went horribly wrong during module loading';
}

1;
