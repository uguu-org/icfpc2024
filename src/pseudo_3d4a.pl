#!/usr/bin/perl -w

use strict;

unless( $#ARGV == 1 )
{
   die "$0 {a} {b}\n";
}
my ($a, $b) = @ARGV;

my $a_bigger = $a - $b;
my $b_bigger = $b - $a;

while( $a_bigger != 0 && $b_bigger != 0 )
{
   $a_bigger--;
   $b_bigger--;
}

if( $a_bigger == 0 )
{
   print "$a\n";
}
else
{
   print "$b\n";
}
