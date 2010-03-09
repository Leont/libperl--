package Library::Build;

use 5.008;
use strict;
use warnings;

our $VERSION = 0.01;

use autodie;
use ExtUtils::CBuilder;
use List::MoreUtils 'any';
use POSIX qw/strftime/;
use TAP::Harness;
use File::Path qw/mkpath rmtree/;

use Exporter 5.57 qw/import/;

our @EXPORT = qw/create_by create_by_system create_dir build_library test_files_from build_executable run_tests remove_tree/;

my $builder = ExtUtils::CBuilder->new;

sub create_by(&@) {
	my ($sub, @names) = @_;
	for (@names) {
		$sub->() if ! -e $_;
	}
	return;
}

sub create_by_system(&@) {
	my ($sub, @names) = @_;
	for (@names) {
		if (not -e $_) {
			my @call = $sub->();
			print "@call\n";
			system @call;
		}
	}
	return;
}

sub create_dir {
	my (@dirs) = @_;
	mkpath($_, 1, oct 744) for @dirs;
	return;
}

sub _get_input_files {
	my $library = shift;
	if ($library->{input}) {
		if (ref $library->{input}) {
			return @{ $library->{input} };
		}
		else {
			return $library->{input}
		}
	}
	elsif ($library->{input_dir}){
		opendir my($dh), $library->{input_dir};
		my @ret = readdir $dh;
		closedir $dh;
		return @ret;
	}
}

sub build_library {
	my ($library_name, $library_ref) = @_;
	my %library    = %{ $library_ref };
	my @raw_files  = _get_input_files($library_ref);
	my $input_dir  = $library{input_dir} || '.';
	my $output_dir = $library{output_dir} || 'blib';
	my %object_for = map {; "$input_dir/$_" => "$output_dir/".$builder->object_file($_) } @raw_files;
	for my $source_file (sort keys %object_for) {
		my $object_file = $object_for{$source_file};
		next if -e $object_file and -M $source_file > -M $object_file;
		$builder->compile(
			source               => $source_file,
			object_file          => $object_file,
			'C++'                => $library{'c++'},
			include_dirs         => $library{include_dirs},
			extra_compiler_flags => $library{cc_flags} || [],
		);
	}
	my $library_file = 'blib/lib'.$builder->lib_file($library_name);
	$builder->link(
		lib_file           => $library_file,
		objects            => [ values %object_for ],
		extra_linker_flags => $library{linker_flags} || '',
		module_name        => 'libperl++',
	) if ! -e $library_file or any { (-M $_ < -M $library_file ) } values %object_for;
	return;
}

sub build_executable {
	my ($prog_source, $prog_exec, %args) = @_;
	my $prog_object = $args{object_file} || $builder->object_file($prog_source);
	$builder->compile(
		source      => $prog_source,
		object_file => $prog_object,
		'C++' => 1,
		%args
	) if !-e $prog_object or -M $prog_source < -M $prog_object;

	$builder->link_executable(
		objects  => $prog_object,
		exe_file => $prog_exec,
		%args,
	) if !-e $prog_exec or -M $prog_object < -M $prog_exec;
	return;
}

sub run_tests {
	my ($options, @test_goals) = @_;
	my $library_var = $options->{library_var} || 'LD_LIBRARY_PATH';
	local $ENV{$library_var} = 'blib';
	printf "Report %s\n", strftime('%y%m%d-%H:%M', localtime);
	my $harness = TAP::Harness->new({
		verbosity => $options->{verbose},
		exec => sub {
			my (undef, $file) = @_;
			return [ $file ];
		},
		merge => 1,
	});

	return $harness->runtests(@test_goals);
}

sub remove_tree {
	my @files = @_;
	rmtree(\@files, 1, 0);
	return;
}

1;

