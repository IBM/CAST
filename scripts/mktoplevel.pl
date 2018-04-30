#!/usr/bin/perl
###########################################################
#     mktoplevel.pl
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

foreach $toplevel (@ARGV)
{
    print "Building toplevel: $toplevel\n";
    mktoplevel($toplevel);
}

sub mktoplevel
{
    my($toplevel) = @_;

    $root = `git rev-parse --show-toplevel`;
    if($? != 0)
    {
	print "Not in source tree\n";
	exit(-1);
    }
    chomp($root);
    
    $base = $root . "/" . $toplevel;
    system("mkdir -p $base/src");
    system("mkdir -p $base/include");
    system("mkdir -p $base/tests");
    
    open($fp, ">$base/src/CMakeLists.txt");
    select $fp;
    print "# cmake example template: commented out\n";
    print "# add_executable(helloworld hello.c)\n";
    print "# target_compile_definitions(helloworld PRIVATE -D_FILE_OFFSET_BITS=64)\n";
    print "# target_include_directories(helloworld PRIVATE \"/usr/local/include/fuse\")";
    print "# target_link_libraries(helloworld -lpthread)\n";
    print "# flightgen(helloworld hello_flightlog.h)\n";
    print "# install(TARGETS helloworld COMPONENT $toplevel DESTINATION $toplevel/bin)\n";
    select STDOUT;
    close($fp);
    
    open($fp, ">$base/include/CMakeLists.txt");
    select $fp;
    print "file(GLOB all_includes *.h)\n";
    print "install(FILES ${all_includes} COMPONENT $toplevel DESTINATION $toplevel/include)\n";
    select STDOUT;
    close($fp);

    open($fp, ">$base/tests/CMakeLists.txt");
    select $fp;
    print "# Testcases:\n";
    select STDOUT;
    close($fp);

    open($fp, ">$base/CMakeLists.txt");
    select $fp;
    print "add_subdirectory(src)\n";
    print "add_subdirectory(include)\n";
    print "add_subdirectory(tests)\n";
    select STDOUT;
    close($fp);
    
    $data = `cat $root/CMakeLists.txt`;
    $data =~ s/(\(ALL_TOPLEVELS\s+.*?)\)/$1 $toplevel\)/s;
    
    open(TMP, ">$root/CMakeLists.txt");
    print TMP $data;
    close(TMP);
}
