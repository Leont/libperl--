#!/usr/bin/perl

# Name:          run_all.pl
# Purpose:       A test scheduler and runner
# Origin:        TAP::Parser Cookbook

use strict;
use warnings;
use POSIX qw/strftime/;
use TAP::Parser qw/all/;
use TAP::Parser::Aggregator qw/all/;

#open my $out_file, ">>", "test_report.txt" or die "Cannot open outfile. $!\n";
printf "Report %s\n", strftime("%y%m%d-%H:%M", localtime);

opendir my($dh), 't' or die "Couldn't open test dir: $!";
my @files = map {"t/$_"} sort grep {/.t$/} readdir $dh;
closedir $dh;

foreach my $file (@files) {
	my $parser = TAP::Parser->new( { exec => [ $file ] } );
	while ( my $result = $parser->next ) {
		printf "%s results: %s\n", $file, $result->as_string;
	}
	my $aggregate = TAP::Parser::Aggregator->new;
	$aggregate->add('testcases', $parser);
	printf "\tPassed: %s\n\tFailed: %s\n", scalar $aggregate->passed, scalar $aggregate->failed;
}
