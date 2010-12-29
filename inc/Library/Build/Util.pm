package Library::Build::Util;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = '0.003';

use Carp 'croak';
use File::Basename qw/dirname/;
use File::Copy qw/copy/;
use File::Find qw/find/;
use File::Path qw/mkpath rmtree/;
use File::Spec::Functions qw/catfile/;

my $SECURE      = oct 744;

my %util_methods = (
	create_dir => sub {
		my ($self, @dirs) = @_;
		mkpath(\@dirs, $self->stash('verbose') >= 0, $SECURE);
		return;
	},
	copy_files => sub {
		my ($self, $source, $destination) = @_;
		if (-d $source) {
			$self->create_dir($destination);
			opendir my $dh, $source or croak "Can't open dir $source: $!";
			for my $filename (readdir $dh) {
				next if $filename =~ / \A \. /xms;
				$self->copy_files(catfile($source, $filename), catfile($destination, $filename));
			}
			closedir $dh;
		}
		elsif (-f $source) {
			$self->create_dir(dirname($destination));
			if (not -e $destination or -M $source < -M $destination) {
				print "cp $source $destination\n" if $self->stash('verbose') >= 0;
				copy($source, $destination) or croak "Could not copy '$source' to '$destination': $!";
			}
		}
		return;
	},
	remove_tree => sub {
		my ($self, @files) = @_;
		rmtree(\@files, $self->stash('verbose') >= 0, 0);
		return;
	},
	find_files => sub {
		my ($self, $dir, $regexp) = @_;
		my @ret;
		find({
			wanted   => sub {
				push @ret, $_ if $_ =~ $regexp; 
			},
			no_chdir => 1,
		} , $dir) if -d $dir;
		@ret = sort @ret;
		return @ret;
	},
	prompt => sub {
		my ($self, $question, $default) = @_;
		return $default if $ENV{PERL_MM_USE_DEFAULT} or not -t STDIN and eof STDIN;
		print "$question [$default] ";
		my $answer = <STDIN>;
		return $default if not defined $answer;
		chomp $answer;
		return $answer;
	},
	yes_no => sub {
		my ($self, $question, $default) = @_;
		while (1) {
			my $answer = $self->prompt($question, $default ? 'y' : 'n');
			return 1 if $answer =~ / \A y ( es )? \z /xmsi;
			return 0 if $answer =~ / \A n ( o  )? \z /xmsi;
			print "Please answer 'y' or 'n'.\n";
		}
	},
	remove_dirty_files => sub {
		my ($self, @categories) = @_;
		my @keys = map { $_ eq 'all' ? keys %{ $self->{dirty_files} } : $_ } @categories;
		my @files = map { @{ $self->{dirty_files}{$_} } } sort @keys;
		$self->remove_tree(@files);
		return;
	},
	register_dirty => sub {
		my ($self, %files_map) = @_;
		while (my ($category, $files) = each %files_map) {
			push @{ $self->{dirty_files}{$category} }, @{$files};
		}
		return;
	},
);

my %util_actions = (
	help      => sub {
		my $builder = shift;
		print "Available commands:\n";
		for my $command (sort keys %{$builder->{action_map}}) {
			print "$command\n";
		}
	},
	clean     => sub {
		my $builder = shift;
		$builder->remove_dirty_files($builder->stash('what') || 'all');
	},
	realclean => sub {
		my $builder = shift;
		$builder->dispatch('clean');
		$builder->remove_tree('Build', 'MYMETA.yml');
	},
);

sub mixin {
	my $builder = shift;

	$builder->inject_roles({ util => \%util_methods });
	$builder->register_actions(%util_actions);
	$builder->register_argument(what => 1);
	return;
}

1;
