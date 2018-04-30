#!/bin/bash
#================================================================================  
#                                                                                  
#    csmalertfwd.pl                                                                
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
#================================================================================*/
# 
# script to forward json alert data to a background script that will
# forward the files to the CSM rest api daemon running on the CSM management node..
#


thisdir=`dirname "$0"`
thisscript=`basename "$0"`

CSMALERT_CFGFILE=$thisdir/csmalertfwd.conf

#if [ ! -e $BASHCSMALERT_JSON ] || [ $CSMALERT_JSON -nt $BASHCSMALERT_JSON ]; then
#    echo "need to extract config information..."
#
#fi

# what controls updating this...
if [ -e $CSMALERT_CFGFILE ]; then
    . $CSMALERT_CFGFILE
fi

if [ -z $CSMALERT_WORKDIR ]; then
    echo "missing CSMALERT_WORKDIR, FAIL"
    exit 1;
fi
# work directory, for config, and  and lock files...
#CSMALERT_WORKDIR=/tmp/csmrestd
# temporary test configuration file location for development...
#CSMALERT_JSON=/u/ralphbel/bluecoral/csmrestd/tests/coral.cfg


logfile=$CSMALERT_WORKDIR/csmalert.log
lockfile=$CSMALERT_WORKDIR/csmalert.lock
bkgrunning=$CSMALERT_WORKDIR/csmalertbkg.pid		# lock file for indicating we are running..

csmalertbkg=$thisdir/csmalertbkg.pl

if [ -t 0 ]; then
    echo "expecting json data on stdin, stdin cannot be a tty";
    exit 1;
fi

if [ ! -e $CSMALERT_WORKDIR ]; then
   mkdir -p $CSMALERT_WORKDIR
fi


# get alert json data from stdin...
alert_json_data=`cat`;

# make sure the lock file exists...
if [ ! -e $lockfile ]; then
    touch $lockfile;
fi



#echo $alert_json_data;
(
  #NOTE: this is not working, we hang in the flock in the background task,
  #      during the open step...
  # Wait for lock on /var/lock/.myscript.exclusivelock (fd 200) for 10 seconds
  echo "waiting for lock"
  flock -x 200 || exit 1;

  # if the logfile does not exist then we need to kick off the background task
  # to rename and send it on its way...
  kickstart_bkg=0;
  if [ ! -e $logfile ]; then
      kickstart_bkg=1;
  fi;
  if [ -e $bkgrunning ]; then
     p=$(cat $bkgrunning);
     if [ ! -e /proc/$p ]; then
	kickstart_bkg=1;
     fi
  fi

  
  echo $alert_json_data >>$logfile;	# append data to the log file...

  # execute this in a subscript and close the 200 file handle in a subscript
  # NOTE: If we don't do this exec 200, then the invoked program hangs when it attemps
  #       to lock the same file, because it also has the file handle open as id 200...
  if [ $kickstart_bkg -eq 1 ]; then
    echo "executing csmalertbkg"
    (exec 200<&-; 
      $csmalertbkg --config=$CSMALERT_CFGJSON  </dev/null >$CSMALERT_WORKDIR/bkg.log 2>&1 &
      bkgpid=$!;		
      echo $bkgpid >$bkgrunning;
      ) 	# run in the packground
  fi
  #./x.sh   </dev/null >bkg.log 2>&1 & 	# run in the packground
  
  #echo `date +%T`;
  #sleep 5;

) 200<$lockfile











