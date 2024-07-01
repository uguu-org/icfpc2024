#!/usr/bin/perl -w

if( $#ARGV != 0 )
{
   die "$0 {a}\n";
}
my ($a) = @ARGV;

my $p = 1;

while( $a != 0 )
{
   my $t1 = $a % 10;
   $a = $a / 10;
   if( $t1 == 1 )
   {
      $p--;
      if( $p == 0 )
      {
         print "0\n";
         exit;
      }
   }
   if( $t1 == 2 )
   {
      $p++;
   }
}
if( $p == 1 )
{
   print "1\n";
}
else
{
   print "0\n";
}
