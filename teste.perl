#!/usr/bin/perl

if ("http://www.google.com/imagem" =~ m{^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?} ) {
  my($host, $path) = ($4, $5);
  print "$host => $path";
  print "\nIt matches\n";
}
else {
  print "It doesn't match\n";
}
