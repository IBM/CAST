#!/usr/bin/perl
#================================================================================
#
#    csmrestd/tests/test_loopback.pl
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


use POSIX qw/strftime/;
use REST::Client;
use JSON;

#
# little trick to get a stack trace when die is called deep inside
# a library...
#*CORE::GLOBAL::die = sub { require Carp; Carp::confess };

# won't connect if we don't turn this off...
# 
# using a self-signed certificate we don't need to verify it
$ENV{'PERL_LWP_SSL_VERIFY_HOSTNAME'} = 0;


#The basic use case
my $client = REST::Client->new();


#"test.testcat01.test01",
#ftime,
#"fsgb001",
#"test ras message",
#"k1=v1,k2=v2");

#define CSM_RAS_FKEY_MSG_ID          "msg_id"
#define CSM_RAS_FKEY_SEVERITY        "severity"
#define CSM_RAS_FKEY_TIME_STAMP      "time_stamp"
#define CSM_RAS_FKEY_LOCATION_NAME   "location_name"
#define CSM_RAS_FKEY_PROCESSOR       "processor"
#define CSM_RAS_FKEY_COUNT           "count"
#define CSM_RAS_FKEY_CONTROL_ACTION  "control_action"
#define CSM_RAS_FKEY_MESSAGE         "message"
#define CSM_RAS_FKEY_RAW_DATA        "raw_data"


my %rec_hash = (
  'msg_id' => "test.testcat01.test01",
  'time_stamp' => strftime('%FT%T',localtime),
  'location' => 'fsgb001',
  'raw_data' => 'raw data',
  'k1' => 'v1',
  'k2' => 'v2'
  );

#my %rec_hash = ('a' => 1, 'b' => 2, 'c' => 3, 'd' => 4, 'e' => 5);
my $json = encode_json \%rec_hash;


$client->setHost("http://127.0.0.1:5555");
$client->POST('/csmi/V1.0/loopback/test',$json);
my $res = $client->responseContent();
#print $res . "\n";
if ($json ne $res ) {
   print "bad loopback data\n";
   print "expected = " . $json . "\n";
   print "got      = " . $res  . "\n";
   print "TEST FAILED\n";
   exit 1;
}
print "TEST PASSED\n";



