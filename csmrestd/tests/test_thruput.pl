#!/usr/bin/perl
#================================================================================
#
#    csmrestd/tests/test_thruput.pl
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
#================================================================================


# simple test to test ras thruput via this interface.
#
# this will measure the thruput of the restapi interface.
# 
# what it won't measure is the overhead of spinning up a perl script
# and the overhead of recognizing a ras event in the SCALA system.
#

use POSIX qw/strftime/;
use REST::Client;
use JSON;
use Data::Dumper;
use Time::Local;

#
# little trick to get a stack trace when die is called deep inside
# a library...
#*CORE::GLOBAL::die = sub { require Carp; Carp::confess };

# won't connect if we don't turn this off...
# 
# using a self-signed certificate we don't need to verify it
$ENV{'PERL_LWP_SSL_VERIFY_HOSTNAME'} = 0;

my $interval = 10;

my $end = time() + $interval;
#The basic use case
my $client = REST::Client->new();

# kludge to make ua keep alive work,
my $ua = LWP::UserAgent->new(keep_alive => 2);  #keep alive two connections...
$ua->agent("REST::Client/$VERSION");
$client->setUseragent($ua);




$client->setHost("http://127.0.0.1:5555");

my $numevents = 0;
   
while (time() < $end) {
   $numevents++;
   my $time_stamp = strftime('%FT%T',localtime);
   my %rec_hash = (
     'msg_id' => "test.testcat01.test01",
     'time_stamp' => $time_stamp,
     'location_name' => 'fsgb001',
     'raw_data' => 'raw data',
     'kvcsv' => 'k1=v1,k2=v2'
     );
   
   my $json = encode_json \%rec_hash;
   
   #$client->POST('/csmi/V1.0/loopback/test',$json);
   $client->POST('/csmi/V1.0/ras/event/create',$json);
   if ($client->responseCode != 200) {
      die "client->responseCode = ".$client->responseCode . ", " . $client->responseContent() . "\n";
   }

}


my $eventsPerSec = $numevents/$interval;
print "numevents = $numevents\n";
print "thruput = ". $eventsPerSec . "/s \n";








