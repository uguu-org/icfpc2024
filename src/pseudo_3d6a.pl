#!/usr/bin/perl -w

if( $#ARGV != 0 )
{
   die "$0 {a}\n";
}
my ($a) = @ARGV;

if( $a == 2 )
{
   print "1\n";
   exit;
}

my $t1 = 2;

while( $t1 != $a )
{
   if( $a % $t1 == 0 )
   {
      print "0\n";
      exit;
   }
   $t1++;
}
print "1\n";
