#!/usr/bin/perl
#================================================================================  
#                                                                                  
#    csmalertbpg.pl                                                                
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
#================================================================================*/
#
# background tasks to pick up alerts dropped off by
# csmalertfwd.sh.
#
# this will rename the csmalert.log to csmalert.out while holding the file csmalert.lock under
# an advisory flock..
#


use Fcntl qw(:flock SEEK_END);
use REST::Client;
use JSON;
use Data::Dumper; 
use POSIX qw(strftime);
use Getopt::Long;
use warnings;
use strict;


# using a self-signed certificate we don't need to verify it
$ENV{'PERL_LWP_SSL_VERIFY_HOSTNAME'} = 0;


my $WORKDIR="/tmp/csmrestd";
my $logfile="$WORKDIR/csmalert.log";
my $logprime="$WORKDIR/csmalert.out";
my $lockfile="$WORKDIR/csmalert.lock";


my $cfgfile="/u/ralphbel/bluecoral/csmrestd/tests/coral.cfg";

GetOptions ("config=s"  => \$cfgfile);   # flag


#print "start pgm " . `date +%T` . "\n";

#
# parse a ras event and return the raw ras event data to send to the ras handler...
# params.
#    $rdata -- ras data to parse.
#    returns -- processed ras event json string..
#               will route ras event to unknown.ras.event
#               ras id if we don't recognize it...
sub parse_ras_event {
   my ($rdata) = @_;
   # // how do we reconstruct the raw data such that it is NOT json any more...
   # we need to raw data from the original message, which we don't have yet...
   my $jdata = decode_json( $rdata );
   #print Dumper($jdata);
   my $alertLogRecord = $jdata->{"alertLogRecord"};
   my $alertName = $jdata->{"alertName"};
   my $timestamp = $jdata->{"timestamp"};
   
   # timestamp is a purely numeric timestamp, we may have to convert this...
   # or take it from the original field..

   # alert name formatted as: 'alertName' => 'test.testcat.alert01/syslog/0'
   # extract our message id from this?
   my ($msg_id) = $alertName =~ m/(.*?)\//;

   my $rasmsg = {};     # new ras message hash reference...

   #   'msg_id' => "test.testcat01.test01",
   #   'time_stamp' => $time_stamp,        
   #   'location_name' => 'fsgb001',       
   #   'raw_data' => 'raw data',           
   #   'kvcsv' => 'k1=v1,k2=v2'            
   my $datetime=strftime('%m/%d/%YT%H:%M:%S', localtime($timestamp/1000));
   my $ms = $timestamp % 1000;
   my $timezone = strftime('%z', localtime($timestamp/1000));

   $rasmsg->{"msg_id"} = $msg_id;
   $rasmsg->{"time_stamp"} = $datetime . "." . $ms . " " . $timezone;
   $rasmsg->{"raw_data"} = $alertLogRecord;

   # what do we want to do with a time stamp...
   # needs to be compatable with timestamp for 
   # start to assemble the message...
   if ($msg_id eq "test.testcat01.test01") {
      $rasmsg->{"location_name"} = $jdata->{"alertLogRecordAnnotations"}{"syslogHostname"};
   }
   else {
      $rasmsg->{"msg_id"} =  "unknown.ras.msg";
   }

   #print Dumper($jdata);
   #print Dumper($rasmsg);
   return(encode_json($rasmsg) );

}


# get configuration data from cfg file.  
# NOTE: need to find out where we install this on the BDS system.
#
open FH, "$cfgfile" || die "cannot open $cfgfile";
my $json_text   = join("",<FH>);
close FH;
my $cfg_data = decode_json( $json_text );


#print Dumper($cfg_data);
#print Dumper($cfg_data->{"csmrestd"}{"listenip"});

my $csmresturl=$cfg_data->{"csmrestd"}{"url"};
#print "csmresturl=$csmresturl\n";
$csmresturl="http://10.4.3.16:5555";   # tempoarily override thisl

if (! -e $lockfile) {
   die "can't find lock file $lockfile";
}


my $rascreate_url = "$csmresturl"."/csmi/V1.0/ras/event/create";


my $client = REST::Client->new();
$client->setHost($csmresturl);

my $loopdone=0;
while ( ! $loopdone) {
   # next steps are done under lock.
   # check for new ouut put file, and rename if it is there...
   # print "open $lockfile " . `date +%T` . "\n";
   open A_LOCK, $lockfile || die "cannot open lock file $lockfile";
   flock A_LOCK, LOCK_EX || die "cannot lock $lockfile\n";
   # inside the lock, rename the file...
   if ( -e $logprime ) { 
      unlink $logprime; 
   }
   if ( -e $logfile ) {
      rename $logfile,$logprime;
   } 
   else {
      $loopdone = 1;
   }
   close A_LOCK;
   if ($loopdone) {
      exit 0;
   }

   open JH, "$logprime" || die "cannot open $logprime";

   #print "process file $logprime here\n";
   while (<JH>) {
      my $l = $_;
      my $rasmsg = parse_ras_event($l);

      print "$rasmsg\n";
      $client->POST('/csmi/V1.0/ras/event/create',$rasmsg);
      if ($client->responseCode != 200) {
         my ($firstline) = $client->responseContent() =~ /^(.*)$/m;
         print "client->responseCode = ".$client->responseCode . ", " . $firstline . "\n";
      }
   }

   close JH;

}

