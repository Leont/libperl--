#!/usr/bin/perl -w

# Name:          run_all.pl
# Purpose:       A test scheduler and runner

use strict;
use TAP::Parser qw/all/;
use TAP::Parser::Aggregator qw/all/;

#open my $out_file, ">>", "test_report.txt" or die "Cannot open outfile. $!\n";
printf "Report %s\n", `date +"%y%m%d"`;

my @files = qw( /home/leon/Code/perl++/svn/tap_tester );

foreach my $file (@files) {
	my $parser = TAP::Parser->new( { exec => [ $file ] } );
	while ( my $result = $parser->next ) {
		printf "%s results: %s\n", $file, $result->as_string;
	}
	my $aggregate = TAP::Parser::Aggregator->new;
	$aggregate->add( 'testcases', $parser );
	printf "\tPassed: %s\n\tFailed: %s\n", scalar $aggregate->passed, scalar $aggregate->failed;
}
