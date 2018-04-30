#!/usr/bin/perl
#================================================================================
#   
#    hcdiag/src/tests/rvitals/rvitals.pm
# 
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#=============================================================================
# Check if rvitals matches the description on yaml file

use strict;
use warnings;
use Data::Dumper;
use Getopt::Long;
use Class::Struct;
use File::Temp qw/ tempfile tempdir /;
use File::Basename;
use lib dirname(__FILE__)."/../common";
use ClustConf;
#use LsfBrsvs;
use File::Temp qw/ tempdir /;

sub usage {
   print "$0 [options] noderange\n";
   print "    options\n";
   print "    -v|--verbose: set verbose on\n";
   print "    -h|--help : print this\n" ;
   exit 0
}

my $help=0;
my $verbose=0;
my $test=$0;
$test =~ s{.*/}{};      # removes path  
$test =~ s{\.[^.]+$}{}; # removes extension

GetOptions(
    'h|help'      => \$help,
    'v|verbose'   => \$verbose,
);

if ($help) { usage() }

my $Clustconf = new ClustConf;
$Clustconf->load();

my $tempdir = tempdir( CLEANUP => 1 );

my $noderange = shift @ARGV || do {
  print "Mandatory argument noderange missing\n";
  print "$test test FAIL, rc=1\n";
  exit 1;
};

if ($verbose) { print "nodes=".Dumper($noderange)."\n"; }

#
# scan for rvitals in the node range...
#  what should we return (tuple).
#  #   (errs, rvitalErrors);
#  #   errs: command execution errors).
#  #   rvitlalErrors:
#  #    hash of rvitalErrors indexed by each node
#  #      each record contains:
#  #      node : nodename
#  #      rvId : unique sensor id for the error.
#  #      rvData: error data read from record
#  #      err: error message to display, should we see this twice.
#
# 
sub scanRvitals {
   my ($Clustconf, $noderange, $testfile) = @_;
   my $rvitalsByNode={};
   my $rvitalErrors={};
   my $errs = [];
   # need to capture stdout separatly.
   my $tempdir = tempdir( CLEANUP => 1 );
   
   my $cmd = "rvitals $noderange 2>$tempdir/stderr";
   print $cmd . "\n";
   my $rval = `$cmd`;
   my $rc=$?;
   #if ($verbose) {print $rval . "\n";}
  
   foreach my $l (split(/\n/,`cat $tempdir/stderr`)) {
      chomp $l;
      push (@$errs, $l);
   }
   
   # scan, sort and organize the returned data by node.
   #
   foreach my $l (split(/\n/,$rval)) {
      chomp $l;
      if ($verbose) {print $l . "\n";}
      my ($node, $rvital) = $l =~ /^(\S+):\s+(.*)$/;
      continue if (!defined($node) || !defined($rvital));
      if (! exists $rvitalsByNode->{$node}) {
         $rvitalsByNode->{$node} = [];
      }
      my $rv = $rvitalsByNode->{$node};
      push(@$rv, $rvital);
   }
   
   foreach my $node (sort keys %$rvitalsByNode) {
      my $rvitalRecords = $Clustconf->findRvitals($node);
      if (!defined $rvitalRecords) {
         push(@$errs, "$node: Clustconf->findRvitals($node) failed");
         next;
      }
      my $rv=$rvitalsByNode->{$node};
      for my $rvout (@$rv) {
         if ($verbose) {print "$node: $rvout\n";}
         # rvitals have three two fields a unique identifier: data
         # BIOS Golden Side: 0                    (OEM Reserved 195)          
         my ($rvId,$rvData) = $rvout =~ /^(.*?):\s*(.*)$/;
         next if  (! defined $rvId || ! defined $rvData );
         # - {id: 'Power Status',   match: 'on' }
         # - {id: 'CPU Core Temp .*', regex: '(\S+) C', range: [10,75,'N/A']}
         my $rvRec = $Clustconf->findRvitalById($rvitalRecords, $rvId);
         next if (! defined $rvRec);
         # do we have an extraction regex...
         # regex extraction failures are test failures.
         my $rx = $rvRec->{regex};
         if (defined $rx) {
            my ($rvDataRx) = $rvData =~ qr/$rx/;
            if (!defined $rvDataRx) {
               
               #$rvitalErrors->{$node}->{$rvId} = { node=>$node,
               #                                   rvId=>$rvId,
               #                                   rvId=>$rvData,
               #                                   err=>"$node: $rvout : failed regEx, $rx"
               #                                 };
               #push(@$errs, "$node: $rvout : failed regEx, $rx");
            }
            $rvData = (defined $rvDataRx) ?  $rvDataRx : "";
         }
   
         my $match = $rvRec->{match};
         my $range = $rvRec->{range};
         if (defined $match)  {
            my $rx = "^".$match.'$';
            if (! ($rvData =~ /$rx/)) {
               $rvitalErrors->{$node}->{$rvId} = { node=>$node,
                                                  rvId=>$rvId,
                                                  rvId=>$rvData,
                                                  err=>"$node: $rvout, expected $match, got $rvData"
                                                };
               #push(@$errs, "$node: $rvout, expected $match, got $rvData");
            }
         }
         if (defined $range) {
            my ($min, $max, $other) = @$range;
            if (defined $other) {
               my $rx = "^".$other.'$';;
               if ($rvData =~ /$rx/) {
                  next; # matched so, next...
               }
            }
            #print "==> min=$min, max=$max, other=$other, $rvData\n";
            if ((! ($rvData =~ /\d+/)) ||
                ($rvData+0 < $min) ||
                ($rvData+0 > $max)) {
               $rvitalErrors->{$node}->{$rvId} = { node=>$node,
                                                  rvId=>$rvId,
                                                  rvId=>$rvData,
                                                  err=>"$node: $rvout, expected . ". join(',', @$range). "; got $rvData"
                                                };
               #push(@$errs, "$node: $rvout, expected . ". join(',', @$range). "; got $rvData");
            }
            
         }
      }
   }
   return($errs, $rvitalErrors);
}

sub fmtRange {
   my ($start,$end) = @_;
   if ($verbose) {
      print sprintf("fmtRange(start=%s,end=%s)\n",
                    defined($start)?$start:"undef",
                    defined($end)?$end:"undef"
                    );
   }
   my $nodeRange;
   if ((defined $start) && (defined $end)) {
      if ((length $start) != (length $end)) {
         die "fmtNodeRange program error, start and end need to be the same length ($start,$end)\n";
      }
      if ($start eq $end) {
         $nodeRange = sprintf("%s",$start);
      } elsif ($start > $end) {
         $nodeRange = sprintf("%s-%s",$end,$start);
      } else {
         $nodeRange = sprintf("%s-%s",$start,$end);
      }
   } elsif (defined $start) {
      $nodeRange = sprintf("%s",$start);
   } elsif (defined $end) {
      $nodeRange = sprintf("%s",$end);
   } else {
      die "fmtNodeRange program error, start and end undefined\n";
   }
   return($nodeRange);  
}

sub pushRangeArray {      
   my ($range, $start, $end) = @_;
   if ($verbose) {
      print sprintf("pushRangeArray(start=%s,end=%s)\n",
                    defined($start)?$start:"undef",
                    defined($end)?$end:"undef"
                    );
   }
   if (! defined $start) {
      $start = $end;
   }
   if (defined $start) {
      push (@$range,fmtRange($start,$end)); 
   }
}

sub pushNodeRange {
   my ($nodeRange,$base,$range) = @_;
   if ($verbose) { 
      print sprintf("PushNodeRange(base=%s,(range=(%s))\n",
                    defined($base)?$base:"undef",
                    join(",",@$range)
                    );
   }
   if ((defined $base) && (scalar(@{$range}) > 0)) {
      push(@$nodeRange, sprintf("%s[%s]",$base,join(",",@$range)));
   } 
   elsif (defined $base) {
      push(@$nodeRange, sprintf("%s",$base));
   }
}

#
# take a sorted list of nodes. and 
# consolidate them into a xcat node range.
#
sub createXcatNodeRange {
   my ($nodeList) = @_;
   if ($verbose) { print "createXcatNodeRange nodeList=".Dumper($nodeList)."\n"; }

   my $nodeRange = [];

   if ($verbose) { print "createXcatNodeRange\n"; };
   my $currRange = [];  # current range we are assembling.
   my $startNum;
   my $lastBase;
   my $lastNum;
   for my $n (@$nodeList) {
      # split off name into base and numeric portions.
      # only the last digits count...
      my ($base,$num) = $n =~ /^(.+?)([0-9]*)$/;
      # null strings are also undefined...
      $base = ((defined $base) && (length($base) > 0))?$base : undef;
      $num = ((defined $num) && (length($num) > 0))?$num : undef;
      if ($verbose) {
         print sprintf("\n$n: base=%s,num=%s,startNum=%s,lastNum=%s\n",
                       defined($base)?$base:"undef",
                       defined($num)?$num:"undef",
                       defined($startNum)?$startNum:"undef",
                       defined($lastNum)?$lastNum:"undef"
                       );
      }
      if (! defined $base) {
         die "invalid node name value: $n\n";
      }

      #
      # basename matches...
      #
      if ((defined $lastBase) && ($base eq $lastBase)) {
         if (! defined $lastNum) {    # did we have one previously, if not, then we push the last one out..
            pushNodeRange($nodeRange, $lastBase, $currRange); # push out the previous one.
            $currRange=[];
            $startNum = undef;
         }
         if (defined $num) {
            if (defined $lastNum) {
               if ((length $num) == (length $lastNum)) {
                  if ($num == ($lastNum+1)) {
                     if (! defined $startNum) {  
                        $startNum = $num;
                     }
                  }
                  else {      # discontigous number but same base name, accumulate ranges...
                     pushRangeArray($currRange, $startNum, $lastNum);
                     $startNum = $num;
                  }
               }
               else {      # length is different, same as teh base being different
                  if (defined $lastNum) {
                     pushRangeArray($currRange, $startNum, $lastNum);
                  }
                  pushNodeRange($nodeRange, $lastBase, $currRange); # push out the previous one.
                  $currRange=[];
                  $startNum = $num;
               }
            }
            else {
               pushRangeArray($currRange, $startNum, $lastNum);
               $startNum = $num;
            }
         }
      }
      else {      # base name does NOT match...
         if (defined $lastNum) {
            pushRangeArray($currRange, $startNum, $lastNum);
         }
         pushNodeRange($nodeRange, $lastBase, $currRange); # push out the previous one.
         $currRange=[];
         $startNum = $num;
      }
      $lastNum = $num;
      $lastBase = $base;
   }
   if (defined $lastNum) {
      pushRangeArray($currRange, $startNum, $lastNum);
   }
   pushNodeRange($nodeRange, $lastBase,$currRange);   # flush out the last one

   if ($verbose) {
      print "nodeRange=".Dumper(@$nodeRange)."\n";
   }
   return(join(",",@$nodeRange));
}



# two steps, 
# rvitals is very noisy so we will do this twice,
# once to get a list of nodes with rvitals that are not so good.
# second to see if they are still there on teh same pass.
#
# we will treat only those nodes with an rvital that is out of spec
# 2 times as in error.

# itteration first...
# construct a new node list from this one and 
# run this command again.

my $rvErrs=[];
my $errs1 = [];
my $rvitalErrors1;
my $errs2 = [];
my $rvitalErrors2;
($errs1, $rvitalErrors1) = scanRvitals($Clustconf, $noderange);
my $noderange2 = createXcatNodeRange([sort keys $rvitalErrors1]);

print "rvitalErrors: ".Dumper($rvitalErrors1);
if ($verbose) { print "noderange=".$noderange2."\n"; }
if (length($noderange2) > 0) {
   ($errs2, $rvitalErrors2) = scanRvitals($Clustconf, $noderange2);
   print "rvitalErrors: ".Dumper($rvitalErrors2);
   
   for my $node (sort keys $rvitalErrors2) {
      my $rvn = $rvitalErrors2->{$node};
      for my $rvId (sort keys $rvn) {
         my $rve = $rvn->{$rvId};
         # present on both...
         if (exists $rvitalErrors1->{$node}->{$rvId}) {
            push(@$rvErrs, $rvn->{$rvId}->{err});
         }
         else { # present on 2 and NOT 1
            print("(WARN) ". $rvn->{$rvId}->{err} . "\n");
         }
      }
   }

   for my $node (sort keys $rvitalErrors1) {
      my $rvn = $rvitalErrors1->{$node};
      for my $rvId (sort keys $rvn) {
         my $rve = $rvn->{$rvId};
         # present on 1 and NOT 2.
         if (! exists $rvitalErrors2->{$node}->{$rvId}) {
            print("(WARN) ". $rvn->{$rvId}->{err} . "\n");
         }
      }
   }

}


# print out the errors with an approrpiate error header...
my $errs = [@$errs1, @$errs2, @$rvErrs];
if (scalar @$errs) {
   for my $e (@$errs) {
      print "(ERROR) $e\n";
   }
   print "$test test FAIL, rc=1\n";
   exit 1;
} 


print "$test test PASS, rc=0\n";
exit 0;


