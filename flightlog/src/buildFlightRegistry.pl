#!/usr/bin/perl
################################################################################
#    buildFlightRegistry.pl
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
################################################################################


use Getopt::Long;
use XML::Simple;

chomp($BASEDIR = `pwd`);
$ARGUMENT_STR = join(" ", @ARGV);

setDefaults();
GetOptions("suffix=s"             => \$FILESUFFIX,
	   "registry=s"           => \$REGFILTER,
	   "path=s"               => \$PATH,
	   "depth=i"              => \$DEPTH,
	   "output=s"             => \$OUTPUT,
	   "append=s"             => \$APPEND,
	   "archive!"             => \$ARCHIVE,
	   "help!"                => \$HELP);

if($HELP)
{
    print "$0 --suffix=$FILESUFFIX --path=<path>\n";
    exit(0);
}

if($OUTPUT ne "")
{
    fileScan();

    if($APPEND ne "")
    {
	fileAppend();
    }
    output();
}

sub fileScan
{
    %validSuffix = map { $_ => 1 } split(",", $FILESUFFIX);

    open(FINDFILES, "find $PATH -maxdepth $DEPTH |");
    while($line = <FINDFILES>)
    {
	chomp($line);
	($suffix) = $line =~ /.*(\.\S+)/;
	if(exists $validSuffix{$suffix})
	{
	    parseSourceFile($line);
	}
    }
    close(FINDFILES);
}

sub output
{
    $a = 0; $b = 1;
    outputHeader("$OUTPUT.new");
    system("rsync -c $OUTPUT.new $OUTPUT");
    unlink("$OUTPUT.new");
    
    ($srcfile = $OUTPUT) =~ s/.h$/.c/;
    outputSource("$srcfile.new", $OUTPUT);
    system("rsync -c $srcfile.new $srcfile");
    unlink("$srcfile.new");
}

sub fileAppend
{
    open(TMP, $APPEND);
    while($line = <TMP>)
    {
	chomp($line);
	$LIB  = readXML($line);
	foreach $reg (keys %{$LIB->{"REGISTRY"}})
	{
	    $LIB->{$reg}{"EXTERN"} = 1;
	    $FILE->{"REGISTRY"}{$reg} = 1;
	    $FILE->{$reg} = $LIB->{$reg};
	}
    }
}

sub setDefaults
{
    $FILESUFFIX = ".c,.cc,.cpp";
    $REGFILTER  = "";
    $PATH       = ".";
    $DEPTH      = 99;
    $OUTPUT     = "";
    $APPEND     = "";
    $HELP       = 0;
}


###  Note:  XML::SIMPLE doesn't handle nested multiple-level hashes  $FILE->{"REGISTRY"}{"FOO"}{"BAR"} = 1;
sub writeXML
{
    my($hash) = @_;
    print "\n/* BEGINXML\n";
    print XMLout($hash) . "\n";
    print "ENDXML */ \n";
}

sub readXML
{
    my($file) = @_;
    my $filedata = `cat $file`;
    $filedata =~ s/.*BEGINXML\s*(.*?)\s*ENDXML.*/$1/s;
    return if($filedata !~ /\S/);
    return XMLin($filedata);
}

sub initRegistry
{
    my($reg) = @_;
    $FILE->{"REGISTRY"}{$reg} = 1;
    $FILE->{"ENUMS$reg"}{"FL_NULL_$reg"} = "{\"NULL\", \"Invalid log entry\" }";
    $FILE->{$reg}{"EXTERN"} = 0;
    $FILE->{$reg}{"ENUMCOUNT"} = 1;
    $FILE->{$reg}{"PREFIX"} = $reg if(!exists $FILE->{$reg}{"PREFIX"});
    $FILE->{$reg}{"SIZE"}   = 1024 if(!exists $FILE->{$reg}{"SIZE"});
    $FILE->{$reg}{"NAME"} = $reg   if(!exists $FILE->{$reg}{"NAME"});
    $FILE->{$reg}{"DECODER"} = ""  if(!exists $FILE->{$reg}{"DECODER"});
}

sub parseSourceFile
{
    my($file) = @_;
    open(TMP, $file);
    while($line = <TMP>)
    {
	chomp($line);
	if($line =~ /FL_SetSize/)
	{
	    ($sstr) = $line =~ /FL_SetSize\s*\((.*)/;
	    ($reg, $size) = split(",",$sstr);
	    $FILE->{$reg}{"SIZE"} = $size;
	}
	if($line =~ /FL_SetName/)
	{
	    ($sstr) = $line =~ /FL_SetName\s*\((.*)/;
	    ($reg, $name) = split(",",$sstr);
	    $name =~ s/\s+\"(.*)\".*/$1/;
	    $FILE->{$reg}{"NAME"} = $name;
	}
	if($line =~ /FL_SetDecoder/)
	{
	    ($sstr) = $line =~ /FL_SetDecoder\s*\((.*)/;
	    ($reg, $decoder) = split(",", $sstr);
	    $decoder =~ s/\s+\"(.*)\".*/$1/;
	    $FILE->{$reg}{"DECODER"} = $decoder;
	}
	if($line =~ /FL_SetPrefix/)
	{
	    ($sstr) = $line =~ /FL_SetPrefix\s*\((.*)/;
	    ($reg, $name) = split(",",$sstr);
	    $name =~ s/\s+\"(.*)\".*/$1/;
	    $FILE->{$reg}{"PREFIX"} = $name;
	}
	if($line =~ /FL_Write\d*\s*\(/)
	{
	    ($sstr) = $line =~ /FL_Write\d*\s*\((.*)/;
	    ($text) = $sstr =~ /\"(.*)\"/;
	    $sstr =~ s/\"(.*)\"//;
	    @values = split(",", $sstr);
	    $reg = $values[0];
	    next if(($REGFILTER ne "") && ($REGFILTER ne $reg));

	    $id  = $values[1];
	    $id =~ s/\s+//;
	    $str = $text;
	    initRegistry($reg) if(!exists $FILE->{"REGISTRY"}{$reg});

	    $idstr = $id;
	    $idstr =~ s/\s+/_/;
	    if(exists $FILE->{"ENUMS$reg"}{$id})
	    {
		print "Duplicate flightlog entry!  Registry=$reg   ID=$id\n";
		exit(-1);
	    }
	    $FILE->{"ENUMS$reg"}{$id} = "{\"$idstr\", \"$str\" }";
	    $FILE->{$reg}{"ENUMCOUNT"}++;
	}
    }
    close(TMP);
}

sub enumsort
{
    my($a, $b) = @_;
    return -1 if($a =~ /FL_NULL/);
    return  1 if($b =~ /FL_NULL/);
    return $a cmp $b;
}

sub is_pow2
{
    my($value) = @_;
    return not $value & ($value-1);
}


sub outputHeader
{
    my ($file) = @_;
    if($file ne "")
    {
        open($fp, ">$file");
        select $fp;
    }
    print "/*                                        */\n";
    print "/* Machine generated file.  DO NOT MODIFY */\n";
    print "/*                                        */\n";

    $offset = 0;
    while ( 1 )
    {
        $pos = index($file, "/", $offset);
        last if ( $pos < 0 );
        $offset++;
    }
    $filename =  substr $file, $offset;
    $filename =~ s/\.new//;
    ($ucfilename = uc$filename) =~ s{(.*)\.}{$1_}xms;

    printf("#ifndef %s_\n", $ucfilename);
    printf("#define %s_\n\n", $ucfilename);

    my $pow2 = 1;
    foreach $reg (sort keys %{$FILE->{"REGISTRY"}})
    {
        $pow2 &= is_pow2($FILE->{$reg}{"SIZE"});
    }
    if($pow2)
    {
        print "#define FLIGHTLOG_ASSUME_POWEROF2 1\n";
    }

    print "#include \"flightlog.h\"\n\n";

    foreach $reg (sort keys %{$FILE->{"REGISTRY"}})
    {
        next if($FILE->{$reg}{"EXTERN"});
        print "enum\n";
        print "{\n";
        print "\t" . join(",\n\t", sort { enumsort($a, $b) } keys %{$FILE->{"ENUMS$reg"}}) . "\n";
        print "};\n\n";
    }

    print "#ifdef FLIGHTLOG_OBJ\n";
    foreach $reg (sort keys %{$FILE->{"REGISTRY"}})
    {
        next if($FILE->{$reg}{"EXTERN"});
        print "FlightRecorderFormatter_t FLIGHTFMT_$reg\[\] =\n";
        print "{\n";
        my $prefix = "";
        foreach $key (sort { enumsort($a,$b) } keys %{$FILE->{"ENUMS$reg"}})
        {
            printf("%s\t%s", $prefix, $FILE->{"ENUMS$reg"}{$key}, $suffix);
            $prefix = ",\n";
        }
        print "\n};\n\n";
    }

    foreach $reg (sort keys %{$FILE->{"REGISTRY"}})
    {
        print "extern FlightRecorderFormatter_t* FLIGHTFMT_$reg;\n" if($FILE->{$reg}{"EXTERN"});
        print "extern " if($FILE->{$reg}{"EXTERN"});
        print "FlightRecorderRegistry_t* $reg;\n";
    }

    ($symbol = $OUTPUT) =~ s/.*\/(\S+)\./$1/;

    print "FlightRecorderCreate_t $symbol\[] = { \n";
    $prefix = "";
    foreach $reg (sort keys %{$FILE->{"REGISTRY"}})
    {
        printf("$prefix\t{ &%s, \"%s\", \"%s\", \"%s\", %d, (FlightRecorderFormatter_t**)&FLIGHTFMT_%s, %d }", $reg, $FILE->{$reg}{"NAME"}, $FILE->{$reg}{"PREFIX"}, $FILE->{$reg}{"DECODER"}, $FILE->{$reg}{"SIZE"}, $reg, $FILE->{$reg}{"ENUMCOUNT"});
        $prefix = ",\n";
    }
    print "};\n\n";
    print "size_t sizeof_$symbol = sizeof($symbol);\n";

    print "#else\n";

    foreach $reg (sort keys %{$FILE->{"REGISTRY"}})
    {
        print "extern FlightRecorderFormatter_t* FLIGHTFMT_$reg;\n";
        print "extern FlightRecorderRegistry_t* $reg;\n";
    }

    print "extern FlightRecorderCreate_t $symbol\[];\n";
    print "extern size_t sizeof_$symbol;\n";
    print "#undef FL_CreateAll\n";

    $csum = 0;
    if($file ne "")
    {
        select STDOUT;
        close($fp);
        ($csum) = `sum $file` =~ /(\d+)/;
        open($fp, ">>$file");
        select $fp;
    }

    print "#define FL_CreateAll(rootpath) FL_CreateRegistries(rootpath, sizeof_$symbol/sizeof(FlightRecorderCreate_t), $symbol, $csum)\n";
    print "#endif\n\n";

    writeXML($FILE);

    printf("#endif /* %s_ */\n\n", $ucfilename);

    if($file ne "")
    {
        select STDOUT;
        close($fp);
    }
}

sub outputSource
{
    my($file, $header) = @_;
    if($file ne "")
    {
        open($fp, ">$file");
        select $fp;
    }
    print "/*                                        */\n";
    print "/* Machine generated file.  DO NOT MODIFY */\n";
    print "/*                                        */\n";
    print "#define FLIGHTLOG_OBJ 1\n";
    print "#include \"$header\"\n\n";

    if($file ne "")
    {
        select STDOUT;
        close($fp);
    }

}
