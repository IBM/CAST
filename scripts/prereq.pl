#!/usr/bin/perl
###########################################################
#     prereq.pl
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

#apt-get install mosquitto
#apt-get install libc-ares-dev
#apt-get install uuid-dev
#apt-get install libpq-dev



if(`whoami` !~ /root/)
{
    $useprefix = 1;
}


$JBUILD = "-j4";

build_cmake();
build_boost();
#build_doxygen();
#build_mosquitto();

#build_boostrpm();

sub build_cmake
{
    
    if (-e "./cmake") { $ver_str =  `./cmake --version` }
    else { $ver_str = `cmake --version`; }

    ($ver) = $ver_str =~ /cmake version (\S+)/;
    print "cmake ver: $ver\n";
    return if(($ver cmp "3.5") >= 0);
    
    $bname="cmake-3.5.2";
    $fname="$bname.tar.gz";

    if (-e $fname) { unlink ($fname); }
    if (-e $bname) { system("rm -rf $bname"); }

    system("wget http://cmake.org/files/v3.5/$fname");
    system("tar -xvzf $fname");
    system("patch cmake-3.5.2/Source/CPack/cmCPackGenerator.cxx cmake_template.patch");
    
    chdir("$bname");
    if($useprefix)
    {
	$options = "--prefix=$ENV{HOME}/coraltools/cmake";
	$ENV{"PATH"} .= ":$ENV{HOME}/coraltools/cmake/bin";
	system("ln -s $ENV{HOME}/coraltools/cmake/bin/cmake ../cmake");
    }
    system("./bootstrap $options");
    system("make $JBUILD install");
    chdir("..");
}

sub build_boostrpm
{
    system("wget ftp://rpmfind.net/linux/fedora-secondary/development/rawhide/source/SRPMS/b/boost-1.60.0-4.fc24.src.rpm");
    system("rpm -ivh boost-1.60.0-4.fc24.src.rpm");
    system("patch $ENV{HOME}/rpmbuild/SPECS/boost.spec boost_rpmbuild.patch");
    system("rpmbuild -ba $ENV{HOME}/rpmbuild/SPECS/boost.spec --without python3 --without mpich --without openmpi");
    
#   system("rpm -ivh boost-[1acfgijlmprstw]*rpm boost-devel-1.60.0-4.el7.ppc64.rpm boost-date-time-1.60.0-4.el7.ppc64.rpm");
}

sub build_boost
{    
   # check if we are already running the boost version we want...
   if($useprefix)
   {
      $boost_install_dir = "$ENV{HOME}/coraltools/libboost";
   }
   else
   {
       $boost_install_dir = "--prefix=/opt/boost";
   }
   $bv = `grep '#define BOOST_LIB_VERSION' $boost_install_dir/include/boost/version.hpp`;
   ($ver) = $bv =~ /\S+ \S+ "(\S+)"/;
   print "boost ver: $ver\n";
   return if ($ver eq "1_60");

    $bname="boost_1_60_0";
    $fname="$bname.tar.gz";
    if (-e $fname) { unlink ($fname); }
    if (-e $bname) { system("rm -rf $bname"); }
    system("wget http://downloads.sourceforge.net/project/boost/boost/1.60.0/$fname");
    system("tar -xvzf $fname");
    chdir("$bname");
    $options = "--prefix=$boost_install_dir";

    if($useprefix)
    {
	$options = "--prefix=$ENV{HOME}/coraltools/libboost";
    }
    else
    {
	$options = "--prefix=/opt/boost";
    }
    system("./bootstrap.sh $options --with-toolset=gcc");
    system("./b2 $JBUILD --layout=tagged threading=multi link=shared address-model=64 install");
    chdir("..");
}

sub build_doxygen
{
    $ver_str = `doxygen --version`;
    chomp($ver_str);
    return if(($ver cmp "1.8") >= 0);
    
    system("wget http://ftp.stack.nl/pub/users/dimitri/doxygen-1.8.10.src.tar.gz");
    system("tar -xvzf doxygen-1.8.10.src.tar.gz");
    chdir("doxygen-1.8.10");
    mkdir("build");
    chdir("build");
    system("cmake -G \"Unix Makefiles\" ..");
    system("make $JBUILD");
}

sub build_mosquitto
{
   my $pkgname = "mosquitto";
   my $bname="$pkgname-1.4.5";
   my $fname="$bname.tar.gz";
   my $mosquitto_url="http://mosquitto.org/files/source/$fname";
   my $install_dir="$ENV{HOME}/coraltools/mosquitto";
   # only build this for ubuntu, redhat has the correct version...
   # and only if we don't have a recent enough version installed...
   my $os_release=`(. /etc/os-release; echo -n \$ID)`;
   chomp $os_release;

   unless ( $os_release =~ /ubuntu/) {
      return;
   }

   my $pkg_info=`dpkg -s $pkgname 2>/dev/null`;
   my ($ins_version) = $pkg_info =~ /Version: (\S+)-/;

   my $iv = versiontonum($ins_version); # installed version.
   my $tv = versiontonum("1.4.5");      # target version


   if ($iv >= $tv) {
      print "mosquito version $ins_version already installed\n";
      return;
   }

   if (-e "$install_dir/usr/local/bin/mosquitto_pub") {
      print ("mosquitto already built\n");
      return;
   }


   print "mosquitto $tv not installed\n";


   print "bname=$bname\n";
   print "fname=$fname\n";
   if (-e $bname) { unlink ($bname); }
   if (-e $fname) { system("rm -rf $fname"); }

   system("wget $mosquitto_url");
   system("tar -zxf $fname");


   system("make DESTDIR=$install_dir -C $bname  install\n");

   

}

sub versiontonum() 
{
   my ($vstr) = @_;

   my @s = split(/\./,$vstr);
   my $vnum = shift @s;
   $vnum .= ".";
   foreach $n (@s) {
      $vnum .= sprintf("%03d", $n);
   }
   return($vnum);

}
