#!/usr/bin/perl -w

use strict;

my %adjust =
(
   1 =>  {-1 => "7", 0 => "8", 1 => "9"},
   0 =>  {-1 => "4", 0 => "5", 1 => "6"},
   -1 => {-1 => "1", 0 => "2", 1 => "3"},
);

my ($x, $y) = (0, 0);
my ($vx, $vy) = (0, 0);
while( my $line = <> )
{
   last unless( $line =~ /(-?\d+)\s+(-?\d+)/ );
   my ($new_x, $new_y) = ($1, $2);

   my $dx = $new_x - ($x + $vx);
   my $dy = $new_y - ($y + $vy);

   unless( exists $adjust{$dy}{$dx} )
   {
      die "No trivial solution available to go from ($x,$y) to ($new_x,$new_y) at v=($vx,$vy)\n";
   }

   my $move = $adjust{$dy}{$dx};
   print $move;

   if( $move eq "1" )    { $vx--; $vy--; }
   elsif( $move eq "2" ) { $vy--;        }
   elsif( $move eq "3" ) { $vx++; $vy--; }
   elsif( $move eq "4" ) { $vx--;        }
   elsif( $move eq "6" ) { $vx++;        }
   elsif( $move eq "7" ) { $vx--; $vy++; }
   elsif( $move eq "8" ) {        $vy++; }
   elsif( $move eq "9" ) { $vx++; $vy++; }

   $x += $vx;
   $y += $vy;
}

print "\n";
