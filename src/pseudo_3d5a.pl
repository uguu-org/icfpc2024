#!/usr/bin/perl -w

use strict;

unless( $#ARGV == 1 )
{
   die "$0 {a} {b}\n";
}
my ($a, $b) = @ARGV;

my $t1 = $a;
my $t2 = $b;

while( $b != 0 )
{
   my $t3 = $b;
   $b = $a % $b;
   $a = $t3;
}
my $lcm = $t1 * $t2 / $a;

print "$lcm\n";
