#!/usr/bin/perl -w

use strict;
use constant DICT => "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#\$\%&'()*+,-./:;<=>?\@[\\]^_`|~ \n";

my $text = join '', <>;
print "S";
print foreach map {chr(index(DICT, chr($_)) + 33)} unpack "C*", $text;
print "\n";
