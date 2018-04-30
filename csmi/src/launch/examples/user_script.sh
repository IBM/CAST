#! /bin/bash
#================================================================================
#
#    csmi/src/launch/examples/user_script.sh
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
#================================================================================

#========================= Options and default values  ==========================
# By default the number of tasks is set based on the nodes in the allocation
# This can be overridden by passing "-n TASKS" to the user script.
#echo "Arguments: $@"

while getopts ":n:" opt; do
  case $opt in
    n)
      export NP=$OPTARG
      echo "-n: setting NP = $NP."
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

if [ -z "$LSB_JOBID" ]; then
  echo "LSB_JOBID is not set, defaulting to 1234"
  export LSB_JOBID=1234
fi  

if [[ $CSM_ALLOCATION_ID ]] ; then
  echo "Querying CSM_ALLOCATION_ID = $CSM_ALLOCATION_ID:"
  ALLOCATION_QUERY_OUT="$(csm_allocation_query -a $CSM_ALLOCATION_ID)"

  echo "${ALLOCATION_QUERY_OUT}"
  echo ""

  NUM_NODES=$(echo "${ALLOCATION_QUERY_OUT}" | grep "num_nodes:" | awk '{ print $2; }')
  export MPIRUN="jsrun"
  export LN_COUNT=1;
else
  echo "CSM_ALLOCATION_ID is not set"
  export MPIRUN="mpirun -TCP"
  export LN_COUNT=0;
fi

if [ -z "$NP" ]; then
  if [ -z "$LSB_DJOB_NUMPROC" ]; then
    export NP=1
    echo "Unable to determine the number of allocated resources, defaulting NP = 1"
  else
    export NP=$(($LSB_DJOB_NUMPROC-$LN_COUNT))
    echo "From LSF: LSB_DJOB_NUMPROC = $LSB_DJOB_NUMPROC, setting NP = $NP"
  fi
else
  if [ -z "$LSB_DJOB_NUMPROC" ]; then
    echo "Unable to determine the number of allocated resources, NP = $NP from -n"
  else
    echo "From LSF: LSF_DJOB_NUMPROC = $LSB_DJOB_NUMPROC, NP = $NP from -n"
  fi
fi  

# Run MPI example programs using jsrun
#$MPIRUN -np $NP /usr/bin/sleep 30
$MPIRUN -np $NP /ugadmin/csmvm/spectrum_mpi/examples/hello_world
#$MPIRUN -np $NP /ugadmin/csmvm/spectrum_mpi/examples/ping_pong_ring 100
#$MPIRUN -np $NP /ugadmin/csmvm/spectrum_mpi/examples/io
$MPIRUN -np $NP /usr/bin/hostname
#/usr/bin/hostname
