#================================================================================
#
#    csmd/src/daemon/tests/bringup_daemons.sh
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
#!/bin/bash

usage() {
    cat <<-EOF

Usage: `basename $0` [OPTIONS]
  -i <daemonpath>    path to daemon executable (default: ./work/csmd/bin)
  -r [macu]            roles definition and ordering (see below) (default: macu)
  -c <config_path>     base dir for config files (default: ./myconfig)
                         (looking for filenames:
                    <config_path>/csm_master.cfg
                    <config_path>/csm_aggregator.cfg
                    <config_path>/csm_compute.cfg
                    <config_path>/csm_utility.cfg
  -d <precommand>   run the last requested daemon with this prefix (e.g. valgrind, gdb)

  [macu] string determines order and types of daemons to start
  default is "macu"
  for example:
    mac - starts: master, aggregator, compute  (no utility)
    aum - starts: aggregator, utility, master  (no aggregator)
    m   - starts: just the master
    
  the order of letters matters. There's a 1s sleep before next daemon starts

  shell stays open waiting for any key pressed
  if pressed it calls: killall ${CSMD}

  adjust configuration and log directories accordingly

example:
  `basename $0` -i ./work/csmd/bin -r mac -c ./myconfig

    executes:
      ./work/csmd/bin/csmd -r m -f ./myconfig/csm_master.cfg
      sleep 1
      ./work/csmd/bin/csmd -r a -f ./myconfig/csm_aggregator.cfg
      sleep 1
      ./work/csmd/bin/csmd -r c -f ./myconfig/csm_compute.cfg

    and then waits for any key to be pressed before runing:
     killall ./work/csmd/bin/csmd

EOF
}


DAEMON_PATH=./work/csmd/bin
CONFBASE=./myconfig
ROLES="macu"
DEBUG_PREF=""
DEBUG_ACTIVE=0

while getopts c:d:hi:r: opt
do
    case $opt in
        i) DAEMON_PATH=$OPTARG;;
        r) ROLES=$OPTARG;;
        c) CONFBASE=$OPTARG;;
        d) DEBUG_PREF=${OPTARG}; DEBUG_ACTIVE=1;;
        h) usage; exit 0;;
        *) usage; exit 1;;
    esac
done


CSMD=${DAEMON_PATH}/csmd

MASTER_CFG=${CONFBASE}/csm_master.cfg
AGGREGATOR_CFG=${CONFBASE}/csm_aggregator.cfg
COMPUTE_CFG=${CONFBASE}/csm_compute.cfg
UTILITY_CFG=${CONFBASE}/csm_utility.cfg

MASTER_PID=0
AGGREGATOR_PID=0
COMPUTE_PID=0
UTILITY_PID=0

start_master()
{
    echo "Starting Master. with ${MASTER_CFG}"
    if [ $1 -ne 0 ]; then
        ${DEBUG_PREF} ${CSMD} -r m -f ${MASTER_CFG} &> /tmp/debug.out &
    else
        ${CSMD} -r m -f ${MASTER_CFG} &
    fi
    MASTER_PID=$!
}

start_aggregator()
{
    echo "Starting Aggregator. with ${AGGREGATOR_CFG}"
    if [ $1 -ne 0 ]; then
        ${DEBUG_PREF} ${CSMD} -r a -f ${AGGREGATOR_CFG} &> /tmp/debug.out &
    else
        ${CSMD} -r a -f ${AGGREGATOR_CFG} &
    fi
    AGGREGATOR_PID=$!
}

start_compute()
{
    echo "Starting Compute. with ${COMPUTE_CFG}"
    if [ $1 -ne 0 ]; then
        ${DEBUG_PREF} ${CSMD} -r c -f ${COMPUTE_CFG} &> /tmp/debug.out &
    else
        ${CSMD} -r c -f ${COMPUTE_CFG} &
    fi
    COMPUTE_PID=$!
}

start_utility()
{
    echo "Starting Utility. with ${UTILITY_CFG}"
    if [ $1 -ne 0 ]; then
        ${DEBUG_PREF} ${CSMD} -r u -f ${UTILITY_CFG} &> /tmp/debug.out &
    else
        ${CSMD} -r u -f ${UTILITY_CFG} &
    fi
    UTILITY_PID=$!
}



while [ ${#ROLES} -gt 0 ]; do

    if [ ${#ROLES} -eq 1 ] && [ ${DEBUG_ACTIVE} -eq 1 ]; then
        echo "RUNNING DEBUG"
        RUN_DEBUG=1
    else
        echo "RUNNING REGULAR"
        RUN_DEBUG=0
    fi

    DAEMON=${ROLES:0:1}

    case $DAEMON in
        "m") start_master ${RUN_DEBUG};;
        "a") start_aggregator ${RUN_DEBUG};;
        "c") start_compute ${RUN_DEBUG};;
        "u") start_utility ${RUN_DEBUG};;
        *) echo "unrecognized role: $DAEMON";;
    esac

    sleep 1
    ROLES=${ROLES:1}

done

sleep 2
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
read -p "Press any key to exit and kill every ${CSMD} ... " -n1 -s

if [ ${MASTER_PID} -ne 0 ]; then
    echo "KILLING MASTER. PID=${MASTER_PID}"
    kill ${MASTER_PID}
    sleep 1
fi

if [ ${AGGREGATOR_PID} -ne 0 ]; then
    echo "KILLING AGGREGATOR. PID=${AGGREGATOR_PID}"
    kill ${AGGREGATOR_PID}
    sleep 1
fi

if [ ${COMPUTE_PID} -ne 0 ]; then
    echo "KILLING COMPUTE. PID=${COMPUTE_PID}"
    kill ${COMPUTE_PID}
    sleep 1
fi

if [ ${UTILITY_PID} -ne 0 ]; then
    echo "KILLING UTILITY. PID=${UTILITY_PID}"
    kill ${UTILITY_PID}
fi
