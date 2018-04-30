#!/usr/bin/perl
###########################################################
#     wearsim.pl
#
#     Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################


use Math::Random::MT;
use Statistics::Basic qw(:all);
use Getopt::Long;
use List::Util 'shuffle';

$NUMITERATIONS = 100;
$SEED          = 100;
GetOptions(
    "wear!"                   => \$WEAR,
    "random!"                 => \$RANDOM,
    "lru!"                    => \$LRU,
    "lowest!"                 => \$LOWEST,
    "iter=i"                  => \$NUMITERATIONS,
    "seed=i"                  => \$SEED
    );

if($WEAR + $RANDOM + $LRU + $LOWEST != 1)
{
    print "failure.  $0 [--wear] [--random] [--lru] [--lowest]\n";
    exit(-1);
}
$ALLOCATION_MODE = "WEAR"   if($WEAR);
$ALLOCATION_MODE = "LRU"    if($LRU);
$ALLOCATION_MODE = "LOWEST" if($LOWEST);
$ALLOCATION_MODE = "RANDOM" if($RANDOM);


$gen = Math::Random::MT->new($SEED);
$random_picker = Math::Random::MT->new($SEED);

@NODELIST_ORIG = @NODELIST = (0..5183);
%NODEWEAR  = ();

@JOBSIZES_ORIG = (1, 16, 128, 512, 1024, 2048, 4096, 5184);  # Number of Nodes
@JOBWEAR  = (0, 128, 1024);                                  # Total SSD I/O in GB for job
@JOBTIME  = (1..24);                                         # hours on machine

foreach $node (@NODELIST_ORIG)
{
    $NODEWEAR{$node} = 0;
}

foreach $iter (0..$NUMITERATIONS)
{
    if($iter % 72 == 0)
    {
	$index = pick(0..$#JOBSIZES_ORIG);
	@JOBSIZES = @JOBSIZES_ORIG;
	splice(@JOBSIZES, 0,$index);
    }
    do
    {
	$jobid = buildJob();
	$rc = placeJob($jobid);
    } while($rc == 0);
    
    foreach $jobid (sort keys %ACTIVEJOBS)
    {
	ageJob($jobid);
    }
    dumpNodeWear($iter);
}

sub pick
{
    my @array = @_;
    my $index = int($gen->rand($#array+1));
    return $array[ $index ];
}

sub buildJob
{
    $jobid = $nextjobid++;
    $ACTIVEJOBS{$jobid}{"SIZE"} = pick(@JOBSIZES);
    $ACTIVEJOBS{$jobid}{"WEAR"} = pick(@JOBWEAR);
    $ACTIVEJOBS{$jobid}{"TIME"} = pick(@JOBTIME);
    $overall_wear += $ACTIVEJOBS{$jobid}{"WEAR"};
    return $jobid;
}

sub placeJob
{
    my($jobid) = @_;
    
    $sz = $ACTIVEJOBS{$jobid}{"SIZE"};
    if($#NODELIST+1 < $ACTIVEJOBS{$jobid}{"SIZE"})
    {
	delete $ACTIVEJOBS{$jobid};
	return -1;
    }
    
    if($ALLOCATION_MODE eq "LRU")
    {
	@{$ACTIVEJOBS{$jobid}{"NODES"}} = splice(@NODELIST, 0, $ACTIVEJOBS{$jobid}{"SIZE"});
    }
    elsif($ALLOCATION_MODE eq "LOWEST")
    {
	@NODELIST = sort {$a <=> $b} @NODELIST;
	@{$ACTIVEJOBS{$jobid}{"NODES"}} = splice(@NODELIST, 0, $ACTIVEJOBS{$jobid}{"SIZE"});
    }
    elsif($ALLOCATION_MODE eq "WEAR")
    {
	@NODELIST = sort {$NODEWEAR{$a} <=> $NODEWEAR{$b}} @NODELIST;
	@{$ACTIVEJOBS{$jobid}{"NODES"}} = splice(@NODELIST, 0, $ACTIVEJOBS{$jobid}{"SIZE"});
    }
    elsif($ALLOCATION_MODE eq "RANDOM")
    {
	@NODELIST = shuffle(@NODELIST);
	@{$ACTIVEJOBS{$jobid}{"NODES"}} = splice(@NODELIST, 0, $ACTIVEJOBS{$jobid}{"SIZE"});
    }
    return 0;
}

sub ageJob
{
    my($jobid) = @_;
    $ACTIVEJOBS{$jobid}{"TIME"}--;
    if($ACTIVEJOBS{$jobid}{"TIME"} == 0)
    {
	foreach $node (@{$ACTIVEJOBS{$jobid}{"NODES"}})
	{
	    $NODEWEAR{$node} += $ACTIVEJOBS{$jobid}{"WEAR"};
	}
	push(@NODELIST, @{$ACTIVEJOBS{$jobid}{"NODES"}});
	delete $ACTIVEJOBS{$jobid};
	$JOBSRUN++;
    }
}

sub dumpNodeWear
{
    my($iter) = @_;
    
    my @values = ();
    foreach $node (@NODELIST_ORIG)
    {
	push(@values, $NODEWEAR{$node});
    }
    @values = sort { $a <=> $b} @values;
    printf("%5d %5d %5d %10d %10d %10d %10d\n", $iter, $JOBSRUN, $JOBSIZES[0], mean(@values), median(@values), $values[-1], stddev(@values));
}
