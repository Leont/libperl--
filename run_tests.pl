#!/usr/bin/perl

# Name:          run_all.pl
# Purpose:       A test scheduler and runner
# Origin:        TAP::Parser Cookbook

use strict;
use warnings;
use POSIX qw/strftime/;
use TAP::Parser qw/all/;
use TAP::Parser::Aggregator qw/all/;

printf "Report %s\n", strftime("%y%m%d-%H:%M", localtime);
open STDERR, ">", "/dev/null";

my @files = @ARGV;

my ($passed, $failed) = ( 0 x 2 );
foreach my $file (@files) {
	my $parser = TAP::Parser->new( { exec => [ $file ] } );
	if ($ENV{VERBOSE}) {
		while ( my $result = $parser->next ) {
			printf "%s results: %s\n", $file, $result->as_string;
		}
	}
	else {
		1 while $parser->next;
		printf "%s results\n", $file;
	}
	my $aggregate = TAP::Parser::Aggregator->new;
	$aggregate->add($file, $parser);
	printf "\tPassed: %s\n\tFailed: %s\n", scalar $aggregate->passed, scalar $aggregate->failed;
	$passed += $aggregate->passed;
	$failed += $aggregate->failed;
}

printf "Total:\n\tPassed: %s\n\tFailed: %s\n", scalar $passed, scalar $failed if @files > 1;
