#!/usr/bin/perl
####################################################
#    genras.pl
#
#    Copyright IBM Corporation 2018,2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
###################################################

use JSON;
use Cwd 'abs_path';
use Carp qw( croak );
use Getopt::Long;
use POSIX;

($rasjson, $rash, $rascsv) = @ARGV;

exit(-1) if(!-e $rasjson);
exit(-1) if($rash eq "");
exit(-1) if($rascsv eq "");

$txt = `cat $rasjson`;
$js = decode_json($txt);

$toplevel = "bb";
@VALIDFIELDS = split(",","msg_id,severity,message,description,control_action,threshold_count,threshold_period,enabled,set_state,visibile_to_users");
%VALIDMASK = (
    "severity" => ["FATAL", "ERROR", "WARNING", "INFO"],
    "control_action" => ["NONE"]
    );
%DEFAULTVALUES = (
    "enabled" => "t",
    "visibile_to_users" => "t",
    "threshold_count" => 1,
    "threshold_period" => 0,
    "control_action" => "NONE",
    "message" => "undefined",
    "description" => "no description",
    "set_state" => "",
);

structInit();
searchForRAS($js->{$toplevel});
structTerm();

$mystruct .= "#ifdef SETRASIDS\n";
foreach $key (keys %RAS)
{
    $mystruct .= "$toplevel.$key = \"$toplevel.$key\";\n";
}
$mystruct .= "#endif\n";

open(TMP, ">$rash");
print TMP "$mystruct\n";
close(TMP);

open(TMP, ">$rascsv");
print TMP "#" . join(",", @VALIDFIELDS) . "\n";
foreach $key (sort keys %RAS)
{
    my @col = ();
    foreach $field (@VALIDFIELDS)
    {
	if($field eq "msg_id")
	{
	    push(@col, "$toplevel.$key");
	}
	else
	{
	    push(@col, $RAS{$key}{$field});
	}
    }
    print TMP join(",",@col) . "\n";
}
close(TMP);

exit(0);

sub structInit
{
    $mystruct='/*******************************************************************************
 |    bbras.h
 |
 |    Copyright IBM Corporation 2018,2018. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


#ifndef BB_BBRAS_H_
#define BB_BBRAS_H_

typedef struct { 
';

}
sub structTerm
{
    $mystruct.=" } bbras_t;
#ifndef NORASEXTERN
extern
#endif
bbras_t $toplevel;
#endif
";
}
sub structIndent
{
    my($key) = @_;
    $mystruct .= "struct {\n";
}
sub structOutdent
{
    my($key) = @_;
    $mystruct .= "} $key;\n";
}
sub structField
{
    my($key) = @_;
    $mystruct .= "const char* $key;\n";
}

sub searchForRAS
{
    my($hash, $prefix) = @_;
    my @k = keys %{$hash};
    foreach $key (keys %{$hash})
    {
	if(exists $hash->{$key}{"severity"})
	{
	    my $tmp = $key;
	    $tmp = "$prefix.$tmp" if($prefix ne "");
	    $RAS{$tmp} = $hash->{$key};
	    checkRASType(\$RAS{$tmp});
	    structField($key);
	}
	else
	{
	    my $tmp = $key;
	    $tmp = "$prefix.$tmp" if($prefix ne "");
	    
	    structIndent($key);
	    searchForRAS($hash->{$key}, $tmp);
	    structOutdent($key);
	}
    }
}

sub checkRASType
{
    my($hash) = @_;
    
    %fieldmask = map { $_ => 1} @VALIDFIELDS;
    
    foreach $key (keys %DEFAULTVALUES)
    {
	if($$hash->{$key} eq "")
	{
	    $$hash->{$key} = $DEFAULTVALUES{$key};
	}
    }
    
    foreach $key (keys %{$$hash})
    {
	if($$hash->{$key} =~ /,/)
	{
	    die "Invalid character: $key\n";
	}
	if(!exists $fieldmask{$key})
	{
	    die "Invalid field: $key\n";
	}
    }
    foreach $col (keys %VALIDMASK)
    {
	%vmask = map { $_ => 1 } @{$VALIDMASK{$col}};
	
	$v = $$hash->{$col};
	if(!exists $vmask{$$hash->{$col}})
	{
	    die "Invalid field value: $col $key value=$v\n";
	}
    }
}
