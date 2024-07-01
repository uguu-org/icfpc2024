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
}

while( my $line = <> )
{
   foreach my $token (split /\s/, $line)
   {
      decode_token($token);
   }
}
print "\n";
