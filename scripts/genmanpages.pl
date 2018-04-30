#!/usr/bin/perl
###########################################################
#     genmanpages.pl
# 
#     Copyright IBM Corporation 2018,2018. All Rights Reserved
# 
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
# 
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

use Getopt::Long;
use XML::Simple;

GetOptions(
    "src=s"            => \$SOURCE,
    "bin=s"            => \$BINARY,
    "template=s"       => \$TEMPLATE
    );

open(TMP, ">$BINARY/apidoxy.cfg");
print TMP 'GENERATE_XML           = YES
GENERATE_HTML          = NO
GENERATE_LATEX         = NO
XML_PROGRAMLISTING     = NO';
print TMP "\nXML_OUTPUT = $BINARY/xml\n";
print TMP "INPUT = $SOURCE\n";
close(TMP);

($XMLFILE = $SOURCE) =~ s/.*\///;
$XMLFILE =~ s/\.h/_8h.xml/;

system("doxygen $BINARY/apidoxy.cfg");

$LIB  = XMLin("$BINARY/xml/$XMLFILE");
foreach $func (keys %{$LIB->{"compounddef"}{"sectiondef"}{"memberdef"}})
{
    generate($func);
}

sub INSERTFUNCTION
{
    return $func;
}

sub INSERTPROTOTYPE
{
    return $LIB->{"compounddef"}{"sectiondef"}{"memberdef"}{$func}{"definition"} . 
	$LIB->{"compounddef"}{"sectiondef"}{"memberdef"}{$func}{"argsstring"}
}

sub INSERTBRIEF
{
    return flatten($LIB->{"compounddef"}{"sectiondef"}{"memberdef"}{$func}{"briefdescription"});
}

sub INSERTDETAILED
{
    return flatten($LIB->{"compounddef"}{"sectiondef"}{"memberdef"}{$func}{"detaileddescription"});
}

sub generate
{
    local($func) = @_;
    $podtemplate = `cat $TEMPLATE`;
    $podtemplate =~ s/(INSERT\S+)/&$1()/egs;
    open(TMP, ">$BINARY/$func.pod");
    print TMP $podtemplate;
    close(TMP);
}

sub flatten
{
    my($hash) = @_;
    my @lines = ();
    my %SIMPLE = map { $_ => 1} qw(para simplesect content emphasis ref simplesectsep parametername parameterdescription parameternamelist parameteritem);
    my %IGNORE = map { $_ => 1} qw(kind kindref refid direction parameterlist title);
    
    if(ref($hash) eq "")
    {
	return $hash;
    }
    if(ref($hash) eq "ARRAY")
    {
	foreach $v (@{$hash})
	{
	    push(@lines, flatten($v));
	}
	return join("\n", @lines);
    }
    
    foreach $key (keys %{$hash})
    {
	$keyval = flatten($hash->{$key});
	
	if($key eq "parameterlist")
	{
	    my @list = ();
	    push(@list, $hash->{$key})    if(ref($hash->{$key}) eq "HASH");
	    push(@list, @{$hash->{$key}}) if(ref($hash->{$key}) eq "ARRAY");
	    foreach $e (@list)
	    {
		$kind = getvalue($e, "kind");
		push(@lines, "\n=head2 $kind\n");
		
		my @list2 = ();
		push(@list2, $e->{"parameteritem"})    if(ref($e->{"parameteritem"}) eq "HASH");
		push(@list2, @{$e->{"parameteritem"}}) if(ref($e->{"parameteritem"}) eq "ARRAY");
		
		foreach $p (@list2)
		{
		    $pname = getvalue($p, "parametername");
		    $pdesc = getvalue($p, "parameterdescription");
		    push(@lines, "$pname = $pdesc\n");
		}
	    }
	    push(@lines, "");
	}
	elsif(exists $SIMPLE{$key})
	{
	    push(@lines, $keyval);
	}
	elsif(exists $IGNORE{$key})
	{
	}
	else
	{
	    print "unknown flatten key: $key\n";
	    exit(-1);
	}
    }
    return join("\n", @lines);
}

sub getvalue
{
    my($item, $search) = @_;
    if(ref($item) eq "HASH")
    {
	foreach $k (keys %{$item})
	{
	    if($k eq $search)
	    {
		$v = flatten($item->{$search});
		return $v;
	    }
	}
	foreach $k (keys %{$item})
	{
	    $v = getvalue($item->{$k}, $search);
	    return $v if($v ne "");
	}
    }
    elsif(ref($item) eq "ARRAY")
    {
	my $v;
	foreach $e (@{$item})
	{
	    $v = getvalue($e, $search);
	    return $v if($v ne "")
	}
    }
    else
    {
    }
    return "";
}
