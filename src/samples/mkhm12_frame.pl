#/usr/bin/perl -w

my $c;
for (my $y = 0; $y < 288; $y += 16) { 
    for (my $x = 0; $x <  720; $x += 16) { 
        for (my $i = 0; $i < 16; $i++) { 
            $c = ($i&1)?'y':'Y';
            my $p = int(($y + ($i+1)) % 10);
            if (!$x) {
                print "$p" . "$c"x15;
            } elsif ($x == 704) {
                print "$c"x15 . "$p";
            } else {
                print "$c"x16;
            }

#            print "yYtTyYtTyYtTyYtT";
#           print substr("$x." . ($y+$i) . "yYtTyYtTyYtTyYtT", 0, 16);
#           print substr(";Y... $x x " . ($y + $i) . " .....", 0, 16);
        }
    }
}

for (my $y = 0; $y < 288; $y += 32) { 
    for (my $x = 0; $x <  720; $x += 16) { 
        for (my $i = 0; $i < 32; $i+=2) { 
#            print "uvUVwxWXuvUVwxWX";
            $c = ($i & 2)?'uv':'UV';
            my $p = int(($y + ($i+1)) % 10);
            if (!$x) {
                print "$p." . "$c"x7;
            } elsif ($x == 704) {
                print "$c"x7 . ".$p";
            } else {
                print "$c"x8;
            }
#            print "$c"x8;
#            print chr(($y & 224) + $i)x16;
#            print chr($c).chr($c).chr($c+1).chr($c+1).chr($c+2).chr($c+2).chr($c+3).chr($c+3).chr($c+4).chr($c+4).chr($c+5).chr($c+5).chr($c+6).chr($c+6).chr($c+7).chr($c+7);

#           print substr("$x." . ($y+$i) . "uvUVwxWXuvUVwxWx", 0, 16);
#           print substr(";UV.. $x x " . ($y + $i) . " .......", 0, 16);
        }
    }
}
