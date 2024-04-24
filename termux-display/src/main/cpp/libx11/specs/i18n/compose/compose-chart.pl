#! /usr/bin/perl
#
# Copyright (c) 2009, 2010, Oracle and/or its affiliates.
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
# Make a DocBook chart showing compose combinations for a locale
#
# See perldoc at end (or run with --help or --man options) for details
# of command-line options.
#

# Compose file grammar is defined in modules/im/ximcp/imLcPrs.c

use strict;
use warnings;
use Getopt::Long;
use Pod::Usage;

my $error_count = 0;

my $charset;
my $locale_name;
my $output_filename = '-';
my $man = 0;
my $help = 0;
my $make_index = 0;

GetOptions ('charset:s' => \$charset,
	    'locale=s' => \$locale_name,
	    'output=s' => \$output_filename,
	    'index' => \$make_index,
	    'help|?' => \$help,
	    'man' => \$man)
    or pod2usage(2);
pod2usage(1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;

if (!defined($charset) || ($charset eq "")) {
  if (defined($locale_name)) {
    my $guessed_charset = $locale_name;
    $guessed_charset =~ s{^.*\.}{};
    if ($guessed_charset =~ m{^(utf-8|gbk|gb18030)$}i) {
      $charset = $1;
    } elsif ($guessed_charset =~ m{iso8859-(\d+)}i) {
      $charset = "iso-8859-$1";
    } elsif ($guessed_charset =~ m{^microsoft-cp(125\d)$}) {
      $charset = "windows-$1";
    }
  }
  if (!defined($charset) || ($charset eq "")) {
    $charset = "utf-8";
  }
}

if ($make_index) {
  # Print Docbook output
  open my $OUTPUT, '>', $output_filename
      or die "Could not create $output_filename: $!";

  print $OUTPUT
      join ("\n",
	    qq(<?xml version="1.0" encoding="$charset" ?>),
	    q(<!DOCTYPE article PUBLIC "-//OASIS//DTD DocBook XML V4.3//EN"),
	    q( "http://www.oasis-open.org/docbook/xml/4.3/docbookx.dtd">),
	    q(<article id="libX11-keys">),
	    q(  <articleinfo>),
	    q(    <title>Xlib Compose Key Charts</title>),
	    q(  </articleinfo>),
	    ( map { qq(  <xi:include xmlns:xi="http://www.w3.org/2001/XInclude"  href="$_.xml">\
    <xi:fallback><section><title>$_</title><para></para></section></xi:fallback>\
  </xi:include>) }
	      @ARGV ),
	    q(</article>),
	    "\n"
      );

  close $OUTPUT or die "Couldn't write $output_filename: $!";

  exit(0);
}

foreach my $a (@ARGV) {
  $error_count += make_compose_chart($a);
}

exit($error_count);

sub make_compose_chart {
  my ($filename) = @_;
  my $errors = 0;

  my @compose_table = ();
  my @included_files = ();

  my $line = 0;
  my $pre_file = ($filename =~ m{\.pre$}) ? 1 : 0;
  my $in_c_comment = 0;
  my $in_comment = 0;
  my $keyseq_count = 0;

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
      $cl =~ s{^\s*XCOMM}{#};		# Translate pre-processing comments
    }

    chomp($cl);

    if ($cl =~ m{^\s*#\s*(.*)$}) {	# Comment only lines
      # Combine comment blocks
      my $comment = $1;

      if ($in_comment) {
	my $prev_comment = pop @compose_table;
	$comment = join(' ', $prev_comment->{-comment}, $comment);
      } else {
	$in_comment = 1;
      }

      push @compose_table, { -type => 'comment', -comment => $comment };
      next COMPOSE_LINE;
    }

    $in_comment = 0;

    if ($cl =~ m{^\s*$}) {		# Skip blank lines
      next COMPOSE_LINE;
    }
    elsif ($cl =~ m{^(STATE\s+|END_STATE)}) {
      # Sun extension to compose file syntax
      next COMPOSE_LINE;
    }
    elsif ($cl =~ m{^([^:]+)\s*:\s*(.+)$}) {
      my ($seq, $action) = ($1, $2);
      $seq =~ s{\s+$}{};

      my @keys = grep { $_ !~ m/^\s*$/ } split /[\s\<\>]+/, $seq;

      push @compose_table, {
	-type => 'keyseq',
	-keys => [ @keys ],
	-action => $action
      };
      $keyseq_count++;
      next COMPOSE_LINE;
    } elsif ($cl =~ m{^(STATE_TYPE:|\@StartDeadKeyMap|\@EndDeadKeyMap)}) {
      # ignore
      next COMPOSE_LINE;
    } elsif ($cl =~ m{^include "(.*)"}) {
      my $incpath = $1;
      $incpath =~ s{^X11_LOCALEDATADIR/(.*)/Compose}{the $1 compose table};

      push @included_files, $incpath;
      next COMPOSE_LINE;
    } else {
      print STDERR ('Unrecognized pattern in ', $filename,
		    ' on line #', $line, ":\n  ", $cl, "\n");
    }
  }
  close $COMPOSE;

  if ($errors > 0) {
    return $errors;
  }

  # Print Docbook output
  open my $OUTPUT, '>', $output_filename
      or die "Could not create $output_filename: $!";

  print $OUTPUT
      join ("\n",
	    qq(<?xml version="1.0" encoding="$charset" ?>),
	    q(<!DOCTYPE section PUBLIC "-//OASIS//DTD DocBook XML V4.3//EN"),
	    q( "http://www.oasis-open.org/docbook/xml/4.3/docbookx.dtd">),
	    qq(<section id="$locale_name">),
	    qq(<title>Xlib Compose Keys for $locale_name</title>),
	    q(<para>Applications using Xlib input handling should recognize),
	    q( these compose key sequences in locales using the),
	    qq( $locale_name compose table.</para>),
	    "\n"
      );

  if (@included_files) {
    print $OUTPUT
	q(<para>This compose table includes the non-conflicting),
	q( entries from: ),
	join(',', @included_files),
	q(.  Those entries are not shown here - see those charts for the),
	q( included key sequences.</para>),
	"\n";
  }

  my @pretable_comments = ();

  if ($keyseq_count == 0) {
    @pretable_comments = @compose_table;
  } elsif ($compose_table[0]->{-type} eq 'comment') {
    push @pretable_comments, shift @compose_table;
  }

  foreach my $comment_ref (@pretable_comments) {
    print $OUTPUT
	qq(<para>), xml_escape($comment_ref->{-comment}), qq(</para>\n);
  }

  if ($keyseq_count > 0) {
    start_table($OUTPUT);
    my $row_count = 0;

    foreach my $cr (@compose_table) {

      if ($row_count++ > 750) {
	# Break tables every 750 rows to avoid overflowing
	# xmlto/xsltproc limits on the largest tables
	end_table($OUTPUT);
	start_table($OUTPUT);
	$row_count = 0;
      }

      if ($cr->{-type} eq 'comment') {
	print $OUTPUT
	    qq(<row><entry namest='seq' nameend='action'>),
	    xml_escape($cr->{-comment}), qq(</entry></row>\n);
      } elsif ($cr->{-type} eq 'keyseq') {
	my $action = join(" ", xml_escape($cr->{-action}));
	if ($action =~ m{^\s*"\\([0-7]+)"}) {
	  my $char = oct($1);
	  if ($char >= 32) {
	    $action =~ s{^\s*"\\[0-7]+"}{"&#$char;"};
	  }
	}
	$action =~ s{^\s*"(.+)"}{"$1"};

	print $OUTPUT
	    qq(<row><entry>),
	    qq(<keycombo action='seq'>),
	    (map { qq(<keysym>$_</keysym>) } xml_escape(@{$cr->{-keys}})),
	    qq(</keycombo>),
	    qq(</entry><entry>),
	    $action,
	    qq(</entry></row>\n);
      }
    }

    end_table($OUTPUT);
  } else {
    print $OUTPUT
	qq(<para><emphasis>),
	qq(This compose table defines no sequences of its own.),
	qq(</emphasis></para>\n);
  }
  print $OUTPUT "</section>\n";

  close $OUTPUT or die "Couldn't write $output_filename: $!";

  return $errors;
}

sub xml_escape {
  my @output;

  foreach my $l (@_) {
      $l =~ s{\&}{&amp;}g;
      $l =~ s{\<}{&lt;}g;
      $l =~ s{\>}{&gt;}g;
      push @output, $l;
  }
  return @output;
}

sub start_table {
  my ($OUTPUT) = @_;

  print $OUTPUT
      join("\n",
	   qq(<table><title>Compose Key Sequences for $locale_name</title>),
	   qq(<tgroup cols='2'>),
	   qq( <colspec colname='seq' /><colspec colname='action' />),
	   qq( <thead><row>),
	   qq(  <entry>Key Sequence</entry><entry>Action</entry>),
	   qq( </row></thead>),
	   qq( <tbody>\n),
      );
}

sub end_table {
  my ($OUTPUT) = @_;

  print $OUTPUT "</tbody>\n</tgroup>\n</table>\n";
}

__END__

=head1 NAME

compose-chart - Make DocBook/XML charts of compose table entries

=head1 SYNOPSIS

compose-chart [options] [file ...]

 Options:
    --charset[=<cset>]	character set to specify in XML doctype
    --locale=<locale>	name of locale to display in chart
    --output=<file>	filename to output chart to
    --index		make index of charts instead of individual chart
    --help		brief help message
    --man		full documentation

=head1 OPTIONS

=over 8

=item B<--charset>[=I<cset>]

Specify a character set to list in the doctype declaration in the XML output.
If not specified, attempts to guess from the locale name, else default to
"utf-8".

=item B<--locale>=I<locale>

Specify the locale name to use in the chart titles and introductory text.

=item B<--output>=I<file>

Specify the output file to write the DocBook output to.

=item B<--index>

Generate an index of the listed locale charts instead of a chart for a
specific locale.

=item B<--help>

Print a brief help message and exit.

=item B<--man>

Print the manual page and exit.

=back

=head1 DESCRIPTION

This program will read the given compose table file(s) and generate
DocBook/XML charts listing the available characters for end-user reference.

=cut
