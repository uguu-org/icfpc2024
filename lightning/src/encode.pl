#!/usr/bin/perl -w

use strict;
use constant DICT => "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#\$\%&'()*+,-./:;<=>?\@[\\]^_`|~ \n";

while( my $line = <> )
{
   chomp $line;
   print "S";
   print foreach map {chr(index(DICT, chr($_)) + 33)} unpack "C*", $line;
   print "\n";
}
