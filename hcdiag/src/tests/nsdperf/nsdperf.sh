#!/bin/bash
#=============================================================================
#   
#    hcdiag/src/tests/nsdperf/nsdperf.sh
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
#=============================================================================

# nsdperf - Test network performance, simulating GPFS NSD client / server
# it is shipped under /usr/lpp/mmfs/samples/perf

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

readonly me=${0##*/}
thisdir=`pwd`

# The definitions here should be customized
# 
NSDPERF=${thisdir}/nsdperf
test_input=${thisdir}/nsdperf-test.input
n_server=1


trap 'rm -f /tmp/*$$' EXIT


# ---------------
# usage function
# ---------------
usage () {
    cat <<EOF

USAGE:
    $0 target [-s NSERVERS] [-i FILE] [-p PORT] [-r RDMAPORTS] [-t NRCV] [-w NWORKERS] [-6] [-d] [-h]
    -s NSERVERS   Number of servers (remaining will be clients). Default 1 
    -i FILE       nsdperf test input file (default nsdperf-test.input in this directory ) 
    -p PORT       TCP port to use (default 6668) 
    -r RDMAPORTS  RDMA devices and ports to use (default first device, port 1)
    -t NRCV       Number of receiver threads (default nCPUs, min 2)
    -w NWORKERS   Number of message worker threads (default 32
    -6            Use IPv6 rather than IPv4
    -d            Include debug output                                              ]
    -h            Print help message and exit
EOF
}

# ----------------------------------------------
# function to start/stop/cleanup nsfperf program
# ----------------------------------------------
tfile=/tmp/t$$
nsdperf_control() {
   cmd=$1
   target="$2"
   if [ "$cmd" == "start" ]; then
      echo "starting ${NSDPERF} on $target."
      /opt/xcat/bin/xdsh "$target" "${NSDPERF} -s </dev/null >/dev/null 2>&1 &"
      return;
   fi
   if [ "$cmd" == "stop" ]; then
      echo "stopping nsdperf on $target."
      /opt/xcat/bin/xdsh "$target" "pkill nsdperf"
      return;
   fi
   /opt/xcat/bin/xdsh "$target" "ps -ef |grep nsdperf | egrep -v grep |wc -l 2>/dev/null" 1> $tfile

   if [ "$cmd" == "status" ]; then
      echo "checking nsdperf status on $target"
      cat $tfile
   fi

   ## cleanup
   if [ "$cmd" == "cleanup" ]; then
      echo "cleanup pending nsdperf process on $target"
      ## check if it is down, if not bring it down
      count=`cat $tfile | awk '{print $2}'| sort -u | wc -l`

      if [ $count -eq 1 ]; then
         # check if they all have the same state
         cur_state=`head -1 $tfile  | awk '{print $2}'`
         if [ "$cur_state" -eq "1" ]; then      # it is up
            # cleanup
            echo "stopping nsdperf on $target"
           /opt/xcat/bin/xdsh "$target" "pkill nsdperf"
         fi
      else   # go one by one
         while IFS= read -r line; do
             node=$(echo "$line" | cut -d':' -f1)
             cur_state=$(echo "$line" | cut -d':' -f2)
             if [ $cur_state -eq 1 ]; then
                echo "stopping nsdperf on $node"
               /opt/xcat/bin/xdsh "$node" "pkill nsdperf"
             fi
          done < $tfile
      fi
   fi 
}   

# ----------------------------------------------
# main
# ----------------------------------------------
args=""
noderange=""
[ $# -lt 1 ] && usage && exit 1
if [ ! -x $NSDPERF ]; then echo "ERROR: $NSDPERF not found or invalid permission."; exit 1; fi

# Process arguments
#
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -p|-r|-t|-w)
      args+=" $key $2"
      shift
      shift
      ;;
    -i)
      test_input=$2
      shift
      shift
      ;;
    -h)
      usage
      exit 0
      ;;
    -d)
      args+=" $key"
      shift
      ;;
    -s)
      n_server="$2"
      shift
      shift
      ;;
    *)
      noderange="$1"
      shift
      ;;
  esac
done

nodes=(`echo $noderange | sed 's/,/\n/g'`)
n_node=${#nodes[@]]}
[ $n_node -lt 2 ] && echo "ERROR: at least two nodes is needed for nsdperf test." && exit 1
[ $n_server -eq $n_node ] && echo "ERROR: number of server(s): $n_server can not be the same number of nodes: $n_node." && exit 1

# Let's build the real input file for the nsdperf
#
ifile=/tmp/i$$
ofile=/tmp/o$$
touch ${ifile}
i=0
# add server and client
#
for node in ${nodes[*]}; do
  if [ $i -lt ${n_server} ]; then
     echo "server $node" >> ${ifile}
     let i+=1
  else
     echo "client $node" >> ${ifile}
  fi
done

threshold=()
# get data from the input file
#
while IFS= read -r line; do
   if [ -z "$line" ] || [[ "$line" == \#* ]]; then continue; fi
   tag=$(echo "$line" | cut -d'#' -f1)
   if [[ "$tag" == threshold* ]]; then
     value=`echo $tag | cut -d " " -f2`
     threshold+=("$value")
   else
      echo "$tag" >> ${ifile}
   fi
done < $test_input

#noderange=$(printf ",%s" "${nodes[@]}")
#noderange=${noderange:1}

# start the servers
nsdperf_control "start" "${noderange}"
nsdperf_control "status" "${noderange}"
                
# run the test
${NSDPERF} -i ${ifile} | tee ${ofile}

# check if servers are really gone
nsdperf_control "cleanup" "${noderange}"


echo -e "\nProcessing results:"
# let's parse the output
err=0
i=0
while read -r line; do
   msg=""
   t=`echo "$line" | awk '{print $2}'`
   b=`echo "$line" | awk '{print $3}'`
   u=`echo "$line" | awk '{print $4}'`

   if (( $(echo "${threshold[$i]} > ${b}" | bc -l) )); then msg="ERROR: "; let err+=1; fi
   echo "${msg}test $t: bandwith expected: ${threshold[$i]} $u, got $b $u."
   let i+=1
done < <(grep "msg/sec" $ofile)
                     
if [ "$err" -eq "0" ]; then
   echo "$me test PASS, rc=0"
   exit 0
fi

echo "$me test FAIL, rc=$err"
exit $err
  
