#!/usr/bin/perl -w

use strict;
use constant DICT => "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#\$\%&'()*+,-./:;<=>?\@[\\]^_`|~ \n";

sub decode_token($)
{
   my ($token) = @_;

   if( substr($token, 0, 1) eq "S" )
   {
      foreach my $c (unpack "C*", substr($token, 1))
      {
         print substr(DICT, $c - 33, 1);
      }
   }
   elsif( $token =~ /^[ILv]/ )
   {
      my $n = 0;
      foreach my $c (unpack "C*", substr($token, 1))
      {
         $n = $n * 94 + $c - 33;
      }
      print "$token = ", substr($token, 0, 1), " $n ";
   }
}

while( my $line = <> )
{
   foreach my $token (split /\s/, $line)
   {
      decode_token($token);
   }
}
print "\n";
