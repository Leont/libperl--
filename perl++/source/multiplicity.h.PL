#! /usr/bin/perl -T

use strict;
use warnings FATAL => 'all';
use Config;

local $\ = "\n";
print q/#ifndef pTHX_/;
if ($Config{usethreads}) {
	print q/#define aTHX interp/;
	print q/#define aTHX_ aTHX,/;
	print q/#define pTHX register interpreter* interp/;
	print q/#define pTHX_ pTHX,/;
}
else {
	print q/#define aTHX/;
	print q/#define aTHX_/;
	print q/#define pTHX/;
	print q/#define pTHX_/;
}
print q/#endif/;
