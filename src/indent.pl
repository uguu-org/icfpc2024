#!/usr/bin/perl -w

use strict;

sub indent($$$);
sub indent($$$)
{
   my ($level, $index, $tokens) = @_;

   print "  " x $level, $$tokens[$$index], "\n";
   if( $$tokens[$$index] =~ /^[UL]/ )
   {
      ++$$index;
      indent($level + 1, $index, $tokens);
   }
   elsif( $$tokens[$$index] =~ /^B/ )
   {
      ++$$index;
      indent($level + 1, $index, $tokens);
      indent($level + 1, $index, $tokens);
   }
   elsif( $$tokens[$$index] eq "?" )
   {
      ++$$index;
      indent($level + 1, $index, $tokens);
      indent($level + 1, $index, $tokens);
      indent($level + 1, $index, $tokens);
   }
   else
   {
      ++$$index;
   }
}

my $text = join "", <>;
$text =~ s/^\s*//s;
my @tokens = split /\s+/, $text;
my $index = 0;
indent(0, \$index, \@tokens);
