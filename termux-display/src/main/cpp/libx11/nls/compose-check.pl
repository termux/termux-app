#! /usr/bin/perl
#
# Copyright (c) 2009, Oracle and/or its affiliates.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

#
# Check a compose file for duplicate/conflicting entries and other common errors
#

# Compose file grammar is defined in modules/im/ximcp/imLcPrs.c

use strict;
use warnings;

my $error_count = 0;

if (scalar(@ARGV) == 0) {
  if ( -f 'Compose' ) {
    push @ARGV, 'Compose';
  } else {
    push @ARGV, glob '*/Compose';
  }
}

foreach my $cf (@ARGV) {
  # print "Checking $cf\n";
  $error_count += check_compose_file($cf);
}

exit($error_count);

sub check_compose_file {
  my ($filename) = @_;
  my $errors = 0;

  my %compose_table = ();
  my $line = 0;
  my $pre_file = ($filename =~ m{\.pre$}) ? 1 : 0;
  my $in_c_comment = 0;

  open my $COMPOSE, '<', $filename or die "Could not open $filename: $!";

 COMPOSE_LINE:
  while (my $cl = <$COMPOSE>) {
    $line++;
    chomp($cl);
    my $original_line = $cl;

    # Special handling for changes cpp makes to .pre files
    if ($pre_file == 1) {
      if ($in_c_comment) {		# Look for end of multi-line C comment
	if ($cl =~ m{\*/(.*)$}) {
	  $cl = $1;
	  $in_c_comment = 0;
	} else {
	  next;
	}
      }
      $cl =~ s{/\*.\**/}{};		# Remove single line C comments
      if ($cl =~ m{^(.*)/\*}) {		# Start of a multi-line C comment
	$cl = $1;
	$in_c_comment = 1;
      }
      next if $cl =~ m{^\s*XCOMM};	# Skip pre-processing comments
    }

    $cl =~ s{#.*$}{};			# Remove comments
    next if $cl =~ m{^\s*$};		# Skip blank (or comment-only) lines
    chomp($cl);

    if ($cl =~ m{^(STATE\s+|END_STATE)}) { # Sun extension to compose file syntax
      %compose_table = ();
    }
    elsif ($cl =~ m{^([^:]+)\s*:\s*(.+)$}) {
      my ($seq, $action) = ($1, $2);
      $seq =~ s{\s+$}{};

      my @keys = grep { $_ !~ m/^\s*$/ } split /[\s\<\>]+/, $seq;

      my $final_key = pop @keys;
      my $keytable = \%compose_table;

      foreach my $k (@keys) {
	if ($k =~ m{^U([[:xdigit:]]+)$}) {
	  $k = 'U' . lc($1);
	}
	if (exists $keytable->{$k}) {
	  $keytable = $keytable->{$k};
	  if (ref($keytable) ne 'HASH') {
	    print
	      "Clash with existing sequence in $filename on line $line: $seq\n";
	    print_sequences([$line, $original_line]);
	    print_sequences($keytable);
	    $errors++;
	    next COMPOSE_LINE;
	  }
	} else {
	  my $new_keytable = {};
	  $keytable->{$k} = $new_keytable;
	  $keytable = $new_keytable;
	}
      }

      if (exists $keytable->{$final_key}) {
	print "Clash with existing sequence in $filename on line $line: $seq\n";
	print_sequences([$line, $original_line]);
	print_sequences($keytable->{$final_key});
	$errors++;
      } else {
	$keytable->{$final_key} = [$line, $original_line];
      }
    } elsif ($cl =~ m{^(STATE_TYPE:|\@StartDeadKeyMap|\@EndDeadKeyMap)}) {
      # ignore
    } elsif ($cl =~ m{^include "(.*)"}) {
      my $incpath = $1;
      if (($pre_file == 1) && ($incpath !~ m{^X11_LOCALEDATADIR/})) {
	print "Include path starts with $incpath instead of X11_LOCALEDATADIR\n",
	 " -- may not find include files when installed in alternate paths\n\n";
      }
    } else {
      print 'Unrecognized pattern in ', $filename, ' on line #', $line, ":\n  ",
	$cl, "\n";
    }
  }
  close $COMPOSE;

  return $errors;
}

sub print_sequences {
  my ($entry_ref) = @_;

  if (ref($entry_ref) eq 'HASH') {
    foreach my $h (values %{$entry_ref}) {
      print_sequences($h);
    }
  } else {
    my ($line, $seq) = @{$entry_ref};

    print "  line #", $line, ": ", $seq, "\n";
  }
}
