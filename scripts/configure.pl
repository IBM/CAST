#!/usr/bin/perl
###########################################################
#     configure.pl
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

use Getopt::Long;
use Pod::Usage;
use Cwd 'abs_path';
use XML::Simple;

setDefaults();

GetOptions(
    "disable=s"            => \@DISABLE_TOPLEVEL,
    "work=s"               => \$CFG->{"WORKDIR"},
    "floor=s"              => \$CFG->{"FLOOR"},
    "build=s"              => \$CFG->{"BUILDDIR"},
    "scripts=s"            => \$CFG->{"SCRIPTDIR"},
    "base=s"               => \$CFG->{"BASEDIR"},
    "setup=s"              => \$CFG->{"SETUPFILE"},
    "altinclude=s"         => \$CFG->{"ALTINCLUDE"},
    "parallel!"            => \$CFG->{"BUILD_PARALLEL"},
    "clang!"               => \$CFG->{"USE_CLANG"},
    "clanganalyze!"        => \$CFG->{"USE_CLANGANALYZE"},
    "rpmbuild!"            => \$CFG->{"RPMBUILD"},
    "strict"               => \$CFG->{"STRICT_WARN"},
    "xcode!"               => \$CFG->{"GENXCODE"},
    "verbose-compile"      => \$CFG->{"VERBOSE_COMPILE"},
    "remote=s"             => \$CFG->{"REMOTEIP"},
    "eclipse!"             => \$CFG->{"GENECLIPSEPROJECT"},
    "mincmake=s"           => \$CFG->{"MINCMAKE"},
    "optlevel=s"           => \$CFG->{"OPTLEVEL"},
    "dcgm!"                => \$CFG->{"DCGM"},
    "type=s"               => \$CFG->{"CMAKE_BUILD_TYPE"},
    "help|?"               => \$HELP,
    );
pod2usage(-exitstatus => 0, -verbose => 2) if($HELP);

open(FILE, ">" . $CFG->{"SETUPFILE"});
print FILE "#!/bin/sh\n";
print FILE "###########################################################\n";
printf(FILE "#     %s\n", $CFG->{"SETUPFILE"});
print FILE "#\n";
print FILE "#     Copyright IBM Corporation 2015,2016. All Rights Reserved\n";
print FILE "#\n";
print FILE "#     This program is licensed under the terms of the Eclipse Public License\n";
print FILE "#     v1.0 as published by the Eclipse Foundation and available at\n";
print FILE "#     http://www.eclipse.org/legal/epl-v10.html\n";
print FILE "#\n";
print FILE "#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure\n";
print FILE "#     restricted by GSA ADP Schedule Contract with IBM Corp.\n";
print FILE "###########################################################\n";

print FILE "\n";
print FILE "export PATH=\$PATH:" . $CFG->{"SCRIPTDIR"} . "\n";
close(FILE);

$flags  = "--graphviz=depgraph.dot";
foreach $k (sort keys %{$CFG}) { $flags .= " -D$k=" . $CFG->{$k}; }
$flags .= " -DDISABLE_TOPLEVELS=" . join(",", @DISABLE_TOPLEVEL) if($#DISABLE_TOPLEVEL >= 0);

$genflags = "";
if($CFG->{"GENXCODE"})
{
    $genflags = " -G Xcode";
    $CFG->{"BUILDDIR"} = ".xcode_build";
}
if($CFG->{"GENECLIPSEPROJECT"})
{
    $genflags = " -G \"Eclipse CDT4 - Unix Makefiles\" -DCMAKE_ECLIPSE_VERSION=4.4 -DCMAKE_BUILD_TYPE=Debug";
    # build directory cannot be a subtree of source dir in eclipse...
    $CFG->{"BUILDDIR"} = "../.eclipse_build";
}
if($CFG->{"USE_CLANG"})
{
    $ENV{"CC"} = "clang";
    $ENV{"CXX"}= "clang++";
}
if($CFG->{"USE_CLANGANALYZE"})
{
    $ENV{"CC"} = "/usr/local/libexec/ccc-analyzer";
    $ENV{"CXX"}= "/usr/local/libexec/c++-analyzer";
    $ENV{"CCC_CC"} = "clang";
    $ENV{"CCC_CXX"}= "clang++";
    $ENV{"LD"}= "clang++";
}

$cmakebin = findCmake();

cmd("rm -rf " . $CFG->{"BUILDDIR"});
cmd("mkdir " . $CFG->{"BUILDDIR"});
chdir($CFG->{"BUILDDIR"}) || die "Unable to change directory to " . $CFG->{"BUILDDIR"};
$CFG->{"CMAKEFLAGS"} = $flags . " -DCMAKEFLAGS=\"$flags -DRPMBUILDSRC=1\"";

if($CFG->{"REMOTEIP"} ne "")
{
    $remotecmd = "ssh " . $CFG->{"REMOTEIP"} . " \" cd " . $CFG->{"BUILDDIR"} . "; ";
    $remotepost= "\"";
}

cmd(join(" ", $remotecmd, $cmakebin, $CFG->{"CMAKEFLAGS"}, $genflags, $CFG->{"BASEDIR"}, $remotepost));

if($CFG->{"GENXCODE"} == 0)
{
    open(FILE, ">$BASEDIR/.config");
    print FILE XMLout($CFG);
    close(FILE);
}

print "\n";
print "################################################\n";
print "# Build instructions can be found at $BASEDIR/README\n";
print "#\n";

# Should "use Sort::Versions", but that isn't installed by default.  This should suffice:
sub versioncmp
{
    my($a, $b) = @_;
    my @v1 = split(/\./, $a);
    my @v2 = split(/\./, $b);
    for($x=0; $x <= $#v1; $x++)
    {
        $val = ($v1[$x] <=> $v2[$x]);
        return $val if($val != 0);
    }
    return 0;
}

sub findCmake
{
    $minversion = $CFG->{"MINCMAKE"};
    @searchpaths = (`which cmake`, "/usr/bin/cmake3", "/opt/ibm/cmake/bin/cmake", $ENV{HOME} . "/coraltools/cmake/bin/cmake");
    foreach $path (@searchpaths)
    {
        chomp($path);
        if(-f $path)
        {
            $ver_str = `$path --version`;
            ($ver) = $ver_str =~ /cmake version (\S+)/;
            print "cmake at $path is version = $ver\n";
            if(versioncmp($ver, $minversion) >= 0)
            {
                return $path;
            }
        }
    }
    print "cmake version $minversion or higher is not installed on this system. \n";
    print "Recommend running the scripts/prereq.pl script to install into your user environment.\n";
    print "Request that the system administrator run scripts/prereq.pl as root to make available to all users.\n";
    exit(-1);
}


sub getBaseDir
{
    my $path;
    $path = `git rev-parse --show-toplevel`;
    chomp($path);
    if($? != 0)
    {
	$path = "";
	do
	{
	    ($path) = $path =~ /(\S+)\/\S+/;
	    $path = abs_path($path);
	    if(($path =~ tr/\///) < 2)
	    {
		die "Could not find basedir or .git origin";
	    }
	}
	while(!-e "$path/mainpage.md");
    }
    return $path;
}

sub setDefaults
{
    $BASEDIR = getBaseDir();
    
    $CFG->{"BASEDIR"}          = $BASEDIR;
    $CFG->{"SCRIPTDIR"}        = "$BASEDIR/scripts";
    $CFG->{"WORKDIR"}          = "$BASEDIR/work";
    $CFG->{"BUILDDIR"}         = "$BASEDIR/.build";
    $CFG->{"FLOOR"}            = "";
    $CFG->{"SETUPFILE"}        = "$BASEDIR/SETUP.sh";
    $CFG->{"USE_CLANG"}        = 0;
    $CFG->{"USE_CLANGANALYZE"} = 0;
    $CFG->{"STRICT_WARN"}      = 0;
    $CFG->{"GENXCODE"}         = 0;
    $CFG->{"DCGM"}             = 1;
    $CFG->{"ALTINCLUDE"}       = "";
    $CFG->{"VERBOSE_COMPILE"}  = 0;
    $CFG->{"BUILD_PARALLEL"}   = 0;
    $CFG->{"MINCMAKE"}         = "3.2";
    $CFG->{"CMAKE_BUILD_TYPE"} = "Debug";
    $CFG->{"OPTLEVEL"}         = "-O2";
    
    # Set --dcgm/--nodcgm based on whether the datacenter-gpu-manager rpm is installed
    # This check will be overridden if the user explicits specifies either --dcgm or --nodcgm
    dcgmRpmCheck();
    
    @DISABLE_TOPLEVEL     = ();
    $HELP = 0;
}

sub safe_chdir
{
    my($dir) = @_;
    $BUILDOUTPUT .= "chdir $dir\n";
    chdir($dir) || die "FAIL: Unable to change to directory $dir";
}

sub cmd
{
    my($cmd, $errtext) = @_;
    print STDERR "cmd: $cmd\n";
    system($cmd);
    if($? != 0)
    {
	if($errtext)
	{
	    print "*** CONFIGURE FAILED  ($errtext)\n";
	}
	else
	{
	    print "*** CONFIGURE FAILED  (cmd=$cmd  rc=$?)\n";
	}
	exit(-1);
    }
}

sub dcgmRpmCheck
{
    system("rpm -q datacenter-gpu-manager");
    if($? != 0)
    {
        $CFG->{"DCGM"} = 0;
    }
    else
    {
        $CFG->{"DCGM"} = 1;
    }
}

__END__


=head1 CORAL build configurator

configure.pl sets up the build environment for CORAL open source technologies.


=head1 SYNOPSIS

scripts/configure.pl [options]

 Options:
   --work            Sets the work directory location
   --floor           Set the pointer to the (optional) floor directory
   --build           Sets the build directory location
   --scripts         Sets the location for scripts directory
   --setup           Sets the location of the SETUP.sh file
   --base            Sets the location of the root of the source code directory
   --parallel        Performs parallel build
   --clang           Uses Clang/LLVM for compiles
   --disable         Disable toplevel directory from compilation
   --strict          Enable strict warnings
   --xcode           Builds an xCode project file for the xCode IDE
   --altinclude      Adds an alternative include directory for all compiles
   --compile-verbose Compile using increased verbosity
   --dcgm            Enables compilation of CSM with support for Nvidia Data Center GPU Manager (DCGM)
   --help            brief help message

=head1 OPTIONS

=over 8

=item B<--disable>

Disables the specified toplevel directory from compilation.  Other toplevel components can still include headers.

Example:
   --disable=fshipcld  --disable=bbapi


=item B<--parallel>

Performs a parallel build utilizing all CPUs of the system.  If the parallel build fails, it will subsequently perform a serial build so that error messages are human readable.  

If the serial build does not have the same blocking failure, there is potentially a problem in the build dependencies.  

=item B<--clang>

Enables compilation using the installed Clang compiler

=item B<--setup>

Sets the location of the SETUP.sh file. 

=item B<--base>

Sets the location of the root of the source code repository

=item B<--work>

Sets the location of the work directory.  This directory will contain the installed images.  It will also be the path that RPMs would install into.  Default=$BASE/work

=item B<--floor>

Sets the location of the floor directory.  

=item B<--build>

Sets the location of the build directory.  This directory will contain the object files, generated headers, and build system metadata.  Default=$BASE/.build

=item B<--altinclude>

Adds an alternative include path for all compiles.  

=item B<--verbose-compile>

Increases the stdout verbosity of the compiler.  Useful for debugging build failures.

=item B<--dcgm>

Enables compilation of CSM with support for Nvidia Data Center GPU Manager (DCGM).

=item B<--strict>

Enables strict warnings on compiles.  


=item B<--help>

Print a brief help message and exits.

=back

=head1 DESCRIPTION

B<configure.pl> setup the CORAL build infrastructure.  

=cut
