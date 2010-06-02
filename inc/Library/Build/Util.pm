package Library::Build::Util;

use 5.006;
use strict;
use warnings;

our $VERSION = 0.002;

use autodie;
use Carp 'croak';
use Config;
use ExtUtils::CBuilder;
use List::MoreUtils qw/any first_index/;
use POSIX qw/strftime/;
use TAP::Harness;
use File::Path qw/mkpath rmtree/;

my $compiler = $Config{cc} eq 'cl' ? 'msvc' : 'gcc';

sub cc_flags {
	if ($compiler eq 'gcc') {
		return qw/--std=gnu++0x -ggdb3 -DDEBUG -Wall -Wshadow -Wnon-virtual-dtor -Wsign-promo -Wextra -Winvalid-pch/;
	}
	elsif ($compiler eq 'msvc') {
		return qw{/Wall};
	}
}

sub read_config {
	my $action = shift;

	my @ret;
	
	my @files = (
		($ENV{MODULEBUILDRC} ? $ENV{MODULEBUILDRC}               : ()), 
		($ENV{HOME} ?          "$ENV{HOME}/.modulebuildrc"       : ()), 
		($ENV{USERPROFILE} ?   "$ENV{USERPROFILE}/.modulebuldrc" : ()),
	);
	FILE:
	for my $file (@files) {
		next FILE if not -f $file;

		open my $fh, '<', $file or croak "Couldn't open configuration file '$file': $!";
		my @lines = split / \n (?! \s) /xms, join '', <$fh>;
		for (@lines) {
			next if / ^ \# /xms;
			s{ \n \s+ }{ };
			next if / ^ \s* $ /xms;


		   	if (/ \A \s* (\* | \w+ ) \s+ (.*?) \s* \z /xms) {
				my @args = split /\s+/, $2;
				push @ret, @args if $1 eq $action or $1 eq '*';
			}
			else {
				croak "Can't parse line '$_'";
			}
		}
	}
	return @ret;
}

sub parse_options {
	my %meta_arguments = @_;
	@{ $meta_arguments{envs} } = split / /, $ENV{PERL_MB_OPT} if $ENV{PERL_MB_OPT};

	my %options = (
		silent => 0,
		action => 'build',
	);

	for my $meta_argument ( map { $meta_arguments{$_} } qw/argv envs/ ) {
		my $position = first_index { ! m/ ^ -- /xms and !m/=/xms } @{$meta_argument};
		$options{action} = splice @{$meta_argument}, $position, 1 and last if $position != -1;
	}

	@{ $meta_arguments{config} } = read_config($options{action});

	for my $argument_list (map { $meta_arguments{$_} } qw/config cached envs argv/) {
		for my $argument (@{ $argument_list }) {
			$argument =~ s/ ^ -- //xms;
			if ($argument =~ / ^ (\w+) = (.*) $ /xms) {
				$options{$1} = $2;
			}
			else {
				$options{$argument} = 1;
			}
		}
	}
	$options{silent} = -$options{verbose} if not $options{silent} and $options{verbose};
	return %options;
}

sub get_input_files {
	my $library = shift;
	if ($library->{input_files}) {
		if (ref $library->{input_files}) {
			return @{ $library->{input_files} };
		}
		else {
			return $library->{input_files}
		}
	}
	elsif ($library->{input_dir}){
		opendir my($dh), $library->{input_dir};
		my @ret = grep { /^ .+ \. C $/xsm } readdir $dh;
		closedir $dh;
		return @ret;
	}
}

sub linker_flags {
	my ($libs, $libdirs, %options) = @_;
	my @elements;
	if ($compiler eq 'gcc') {
		push @elements, map { "-l$_" } @{$libs};
		push @elements, map { "-L$_" } @{$libdirs};
		if ($options{'C++'}) {
			push @elements, '-lstdc++';
		}
	}
	elsif ($compiler eq 'msvc') {
		push @elements, map { "$_.dll" } @{$libs};
		push @elements, map { qq{-libpath:"$_"} } @{$libdirs};
	}
	push @elements, $options{append} if defined $options{append};
	return join ' ', @elements;
}

use namespace::clean;

sub new {
	my ($class, %meta) = @_;
	my %options = parse_options(%meta);
	return bless {
		%options,
		builder => ExtUtils::CBuilder->new(quiet => $options{silent}),
	}, $class;
}

sub create_by_system {
	my ($self, $sub, @names) = @_;
	for (@names) {
		if (not -e $_) {
			my @call = $sub->();
			print "@call\n" if $self->{silent} <= 0;
			system @call;
		}
	}
	return;
}

sub create_dir {
	my ($self, @dirs) = @_;
	mkpath(\@dirs, $self->{silent} <= 0, oct 744);
	return;
}

sub build_library {
	my ($self, $library_name, $library_ref) = @_;
	my %library    = %{ $library_ref };
	my @raw_files  = get_input_files($library_ref);
	my $input_dir  = $library{input_dir} || '.';
	my $output_dir = $library{output_dir} || 'blib';
	my %object_for = map { ( "$input_dir/$_" => "$output_dir/build/".$self->{builder}->object_file($_) ) } @raw_files;
	for my $source_file (sort keys %object_for) {
		my $object_file = $object_for{$source_file};
		next if -e $object_file and -M $source_file > -M $object_file;
		$self->{builder}->compile(
			source               => $source_file,
			object_file          => $object_file,
			'C++'                => $library{'C++'},
			include_dirs         => $library{include_dirs},
			extra_compiler_flags => $library{cc_flags} || [ cc_flags ],
		);
	}
	my $library_file = $library{libfile} || "$output_dir/arch/lib".$self->{builder}->lib_file($library_name);
	my $linker_flags = linker_flags($library{libs}, $library{libdirs}, append => $library{linker_append}, 'C++' => $library{'C++'});
	$self->{builder}->link(
		lib_file           => $library_file,
		objects            => [ values %object_for ],
		extra_linker_flags => $linker_flags,
		module_name        => 'libperl++',
	) if not -e $library_file or any { (-M $_ < -M $library_file ) } values %object_for;
	return;
}

sub build_executable {
	my ($self, $prog_source, $prog_exec, %args) = @_;
	my $prog_object = $args{object_file} || $self->{builder}->object_file($prog_source);
	my $linker_flags = linker_flags($args{libs}, $args{libdirs}, append => $args{linker_append}, 'C++' => $args{'C++'});
	$self->{builder}->compile(
		source      => $prog_source,
		object_file => $prog_object,
		extra_compiler_flags => [ cc_flags ],
		%args
	) if not -e $prog_object or -M $prog_source < -M $prog_object;

	$self->{builder}->link_executable(
		objects  => $prog_object,
		exe_file => $prog_exec,
		extra_linker_flags => $linker_flags,
		%args,
	) if not -e $prog_exec or -M $prog_object < -M $prog_exec;
	return;
}

sub run_tests {
	my ($self, @test_goals) = @_;
	my $library_var = $self->{library_var} || $Config{ldlibpthname};
	local $ENV{$library_var} = 'blib/arch';
	printf "Report %s\n", strftime('%y%m%d-%H:%M', localtime) if $self->{silent} < 2;
	my $harness = TAP::Harness->new({
		verbosity => -$self->{silent},
		exec => sub {
			my (undef, $file) = @_;
			return [ $file ];
		},
		merge => 1,
	});

	return $harness->runtests(@test_goals);
}

sub remove_tree {
	my ($self, @files) = @_;
	rmtree(\@files, $self->{silent} <= 0, 0);
	return;
}

1;
