#!/bin/bash
# encoding: utf-8
# ================================================================================
#
# setup.py
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
# ================================================================================

wget -r http://www.haproxy.org/download/1.6/src/haproxy-1.6.9.tar.gz -nd
wget -r http://www.haproxy.org/download/1.6/src/haproxy-1.6.9.tar.gz.md5 -nd
md5sum -c haproxy-1.6.9.tar.gz.md5
if [[ $? -gt 0 ]]
then
    exit 1
fi

#tar -xvf haproxy-1.6.9.tar.gz

# Determine the target to build to.
linux_version=$(uname -r)
target="linux24"

if [[ ${linux_version}  =~ 3.*  || ${linux_version}  =~ 2.6.28.* ]]
then
    target="linux2628"

elif [[ ${linux_version}  =~ 2.6.* ]]
then
    target="linux26"

elif [[ ${linux_version}  =~ 2.4.21.* ]]
then
    target="linux24e"

elif [[ ${linux_version}  =~ 2.4..* ]]
then
    target="linux24"
fi


# Determine if pcre is installed.
pcre_settings="USE_PCRE=1"
rpm -q pcre-devel >/dev/null 2>&1
if [[ $? -gt 0 ]]
then
    yum install -y pcre-devel
    rpm -q pcre-devel >/dev/null 2>&1
    if [[ $? -gt 0 ]]
    then
        pcre_settings=""
    fi

fi 

cd haproxy-1.6.9
make TARGET=${target} ARCH=64 ${pcre_settings} install
