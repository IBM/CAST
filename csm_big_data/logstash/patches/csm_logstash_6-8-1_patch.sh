#!/bin/bash

#================================================================================
#
#    csm_logstash_6-8-1_patch.sh
#
#    © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================



#=================
#
# Reason for this patch: 
#
# When the JRuby Application (logstash) is loaded a LoadError is thrown by ffi/ffi. The bubbled error message is somewhat vague (to a user ignorant to JRuby).
#
# [2019-05-03T10:41:38,701][ERROR][org.logstash.Logstash    ] 
# java.lang.IllegalStateException: Logstash stopped processing because of an error: 
# (LoadError) load error: ffi/ffi -- java.lang.NullPointerException: null
#
# I was able to trace the bug to jruby/lib/ruby/stdlib/ffi/platform/powerpc64-linux/. It looks
# as though the platform.conf file was not created for this platform. Copying the types.conf file to platform.conf appears to resolve the problem.
#
# GitHub Issue: https://github.com/elastic/logstash/issues/10755
#
#==================


STARTDIR=$(pwd)
JARDIR="/usr/share/logstash/logstash-core/lib/jars"
JAR="jruby-complete-9.2.7.0.jar"

JRUBYDIR="${JAR}-dir"
PLATDIR="META-INF/jruby.home/lib/ruby/stdlib/ffi/platform/powerpc64le-linux"

cd ${JARDIR}
unzip -d ${JRUBYDIR} ${JAR}
cd "${JRUBYDIR}/${PLATDIR}"
cp -n types.conf platform.conf
cd "${JARDIR}/${JRUBYDIR}"

zip -r jruby-complete-9.2.7.0.jar *
mv  -f jruby-complete-9.2.7.0.jar ..
cd ${JARDIR}
rm -rf ${JRUBYDIR}

sync
sync
cd ${STARTDIR}
