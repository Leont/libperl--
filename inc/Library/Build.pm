package Library::Build;

use 5.006;
use strict;
use warnings FATAL => 'all';

our $VERSION = 0.003;

use Carp         ();
use Config '%Config';
use File::Spec ();
use Text::ParseWords ();

use Library::Build::Config ();

sub new {
	my ($class, $name, $version, $meta) = @_;
	my $self = {
		name    => $name,
		version => $version,
	};
	my $new_class = '_Singletons::' . (0 + $self);    # See Advanced Perl, or Class::SingletonMethods
	{
		no strict 'refs';
		@{ $new_class . '::ISA' } = ($class);
	}
	bless $self, $new_class;
	$self->stash(verbose => 0);
	$self->register_argument(
		verbose => 0,
		quiet   => sub (;$) {
			my $sub = @_ ? shift : 1;
			$self->stash('verbose', $self->stash('verbose') - $sub);
		},
		config  => sub($) {
			my $raw = shift;
			my ($key, $value) = split /=/, $raw, 2;
			$self->config($key, $value);
		},
	);
	return $self;
}

sub _parse_options {
	my ($self, $argument_list) = @_;
	while ($argument_list && @{$argument_list}) {
		my $argument = shift @{$argument_list};
		if ($argument eq '--') {
			push @{ $self->{arguments} }, @{$argument_list};
			last;
		}
		if ($argument =~ / \A -- (.+) \z /xms) {
			my $cb = $self->{argument_callback}{$1};
			if ($cb) {
				my $proto = prototype $cb;
				if (not defined $proto or $proto eq '') {
					$cb->();
				}
				elsif ($proto eq '$') {
					$cb->(shift @{ $argument_list });
				}
				elsif ($proto eq ';$') {
					my @args = @{$argument_list} && $argument_list->[0] !~ /^--/ ? shift @{ $argument_list } : ();
					$cb->(@args);
				}
				elsif ($proto eq '\@') {
					$cb->($argument_list);
				}
				else {
					Carp::croak("unknown prototype for --$1 handler");
				}
			}
			else {
				Carp::croak("Unknown option '--$1'");
			}
		}
		else {
			push @{ $self->{arguments} }, $argument;
		}
	}
	return;
}

sub parse {
	my ($self, $meta_arguments) = @_;
	my %meta_arguments = %{$meta_arguments};
	@{ $meta_arguments{envs} } = Text::ParseWords::shellwords($ENV{PERL_MB_OPT}) if not defined $meta_arguments{envs} and $ENV{PERL_MB_OPT};

	my $action = @{ $meta_arguments{argv} } && $meta_arguments{argv}[0] !~ / \A -- /x ? shift @{ $meta_arguments{argv} } : 'build';
	$self->stash('action', $action);

	$meta_arguments{qw/config_all config_command/} = @{ Library::Build::Config::read_config($action) }{'*', $action};

	for my $argument_list (map { $meta_arguments{$_} } qw/config_all cached config_command envs argv/) {
		$self->_parse_options($argument_list);
	}
	return;
}

sub name {
	my $self = shift;
	return $self->{name};
}

sub version {
	my $self = shift;
	return $self->{version};
}

sub stash {
	my ($self, $name, $value) = @_;
	if (@_ > 2) {
		$self->{stash}{$name} = $value;
	}
	return $self->{stash}{$name};
}

sub config {
	my ($self, $name, $value) = @_;
	if (@_ > 2) {
		$self->{config}{$name} = $value;
	}
	else {
		return exists $self->{config}{$name} ? $self->{config}{$name} : $Config{$name};
	}
}

sub register_actions {
	my ($self, %action_map) = @_;
	while (my ($name, $sub) = each %action_map) {
		unshift @{ $self->{action_map}{$name} }, $sub;
	}
	return;
}

sub inject_roles {
	my ($self, $roles) = @_;

	my %counter_for;
	my %method_for;
	for my $role (keys %{$roles}) {
		for my $method (keys %{ $roles->{$role} }) {
			push @{ $counter_for{$method} }, $role;
			$method_for{$method} = $roles->{$role}{$method};
		}
	}
	if (my @fail = grep { @{ $counter_for{$_} } != 1 } keys %counter_for) {
		Carp::croak('Role collision: ' . join '; ', map { "$_ is defined in " . join ', ', @{ $counter_for{$_} } } @fail);
	}
	while (my ($name, $sub) = each %method_for) {
		Carp::croak("method for '$name' is not a coderef") if not ref($sub) eq 'CODE';
		no strict 'refs';
		*{ ref($self) . "::$name" } = $sub;
	}
	return;
}

sub register_argument {
	my ($self, %arguments) = @_;
	while (my ($name, $destination) = each %arguments) {
		my @stash_default_callbacks = (
			sub(;$) {
				my $add = @_ ? shift : 1;
				$self->stash($name, ($self->stash($name) || 0) + $add);
			},
			sub($) {
				my $value = shift;
				$self->stash($name, $value);
			},
			sub($) {
				my $value = shift;
				my $elem = $self->stash($name);
				$elem ||= [];
				push @{$elem}, $value;
				$self->stash($name, $elem);
			}
		);
		$self->{argument_callback}{$name} = ref($destination) ? $destination : $stash_default_callbacks[$destination];
	}
	return;
}

sub register_help {
	my ($self, %what) = @_;
	while (my ($command, $description) = each %what) {
		$self->{help_for}{$command} = $description;
	}
	return;
}

my $load = sub {
	my ($module, $try) = @_;
	my @parts = split /::/, $module;
	my $file = $^O eq 'MSWin32' ? join "/", @parts : File::Spec->catfile(@parts);
	$file .= ".pm";
	$file = VMS::Filespec::unixify($file) if $^O eq 'VMS';
	return $try ? eval { require $file } : require $file;
};

sub mixin {
	my ($self, @modules) = @_;
	for my $module (@modules) {
		next if $self->{mixed_in}{$module}++;
		$load->($module, 0);
		my $method = "$module\::mixin";
		$self->$method();
	}
	return;
}

sub dispatch {
	my ($self, @action_names) = @_;
	Carp::croak('No action defined') if not @action_names;
	for my $action_name (@action_names) {
		next if $self->{dispatched}{$action_name}++;
		my $action_ref = $self->{action_map}{$action_name} or Carp::croak("No action '$action_name' defined");
		$self->dispatch_next($action_ref);
	}
	return;
}

sub dispatch_next {
	my ($self, $action_ref) = @_;
	if (@{$action_ref}) {
		my ($action_sub, @action_rest) = @{$action_ref};
		$action_sub->($self, \@action_rest);
	}
	return;
}

sub dispatch_default {
	my $self = shift;
	$self->dispatch($self->stash('action'));
	return;
}

sub DESTROY {
	my $self = shift;
	delete $_Singletons::{ 0 + $self . '::' };
	return;
}

1;
