#!/bin/bash
#--------------------------------------------------------------------------------
#
#    csm_db_utility_setup_script.sh
#
#  © Copyright IBM Corporation 2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#--------------------------------------------------------------------------------

#--------------------------------------------------------------------------------
#   usage:              ./csm_db_utility_setup_script.sh
#   current_version:    01.0
#   create:             02-20-2019
#   last modified:      03-07-2019
#--------------------------------------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'

OPTERR=0
logpath="/var/log/ibm/csm/db"
#logpath=`pwd` #<------- Change this when pushing to the repo.
logname="csm_db_utility_setup_script.log"
cd "${BASH_SOURCE%/*}" || exit
cur_path=`pwd`


#----------------------------------------------
# Current user connected
#----------------------------------------------
current_user=`id -u -n`
db_username="postgres"
csmdb_user="csmdb"
root_user="root"
xcat_admin_user="xcatadm"
now1=$(date '+%Y-%m-%d %H:%M:%S')
now=$(date '+%Y-%m-%d %H:%M:%S')
hostname=`hostname -I | awk '{print $1}'`
ext="321"
host_ext="$hostname/$ext"
pg_hba_conf_dir="/var/lib/pgsql/data/"
pg_hba_conf_file="/var/lib/pgsql/data/pg_hba.conf"

line1_out="------------------------------------------------------------------------------------------------------------------------"
line2_log="------------------------------------------------------------------------------------"
line3_log="---------------------------------------------------------------------------------------------------------------------------"
line4_out="-------------------------------------------------------------------------------------------------------------"

BASENAME=`basename "$0"`

#-----------------------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory.
# The current version will only display results to the screen
#-----------------------------------------------------------------------------

if [ -w "$logpath" ]; then
    logdir="$logpath"
else
    logdir="/tmp"
fi
logfile="${logdir}/${logname}"

#----------------------------------------------
# Log Message
#----------------------------------------------

function LogMsg () {
now=$(date '+%Y-%m-%d %H:%M:%S')
echo "$now ($current_user) $1" >> $logfile
}

#--------------------------------
# Log messaging intro. header
#--------------------------------

echo "${line1_out}"
LogMsg "[Start   ] Welcome to CSM database utility setup script."
LogMsg "${line2_log}" >> $logfile
echo "[Start   ] Welcome to CSM database utility setup script."
echo "${line1_out}"
echo "[Info    ] Log Directory:                     | $logfile"

#----------------------------------------------
# Usage Command Line Functions
#----------------------------------------------

function usage () {
echo "------------------------------------------------------------------------------------------------------------------------"
echo "[Info    ] $BASENAME : Load CSM DB root, csmdb, xcatadm user(s) if not presnt in system"
echo "[Usage   ] $BASENAME : $BASENAME (no additional arguments required)"
echo "------------------------------------------------------------------------------------------------------------------------"
echo "  Argument       | Description"
echo "-----------------|------------------------------------------------------------------------------------------------------"
echo "  script_name    | This script checks to see if the PostgreSQL RPMs are installed, the pg_hba.conf file is present"
echo "                 | in the /var/lib/pgsql/data directory, checks the server status if its running, and also"
echo "                 | creates a root, csmdb, and xcatadm db user(s) if not present in the system."
echo "-----------------|------------------------------------------------------------------------------------------------------"
echo "------------------------------------------------------------------------------------------------------------------------"
}

#---------------------------------------------
# The optstring for input.
#---------------------------------------------

optstring="h"

while getopts $optstring OPTION
do
    case $OPTION in
        h|*)
            usage; exit 1;;
    esac
done

#------------------------------------------------
# Check to see if PostgreSQL RPMs are installed
#------------------------------------------------

rpm -qa | grep postgresql | grep server > /dev/null

    if [ $? -ne 0 ]; then
        echo "[Error   ] PostgreSQL RPMs:                   | Are not installed"
        echo "[Info    ] Please check:                      | Your configuration settings or install PostreSQL RPMs"
        echo "[Info    ] Once the PostgreSQL RPMs:          | Are installed please re-run this script for setup"
	echo "${line1_out}"
        LogMsg "[Error   ] PostgreSQL RPMs:                     | Are not installed"
        LogMsg "[Info    ] Please check:                        | Your configuration settings or install PostreSQL RPMs"
        LogMsg "[Info    ] Once the PostgreSQL RPMs:            | Are installed please re-run this script for setup"
	LogMsg "${line2_log}"
	LogMsg "[End     ] Exiting $0 script"
        echo "${line3_log}" >> $logfile
	exit 0
    else
        echo "[Info    ] PostgreSQL RPMs:                   | Are already installed"
        LogMsg "[Info    ] PostgreSQL RPMs:                     | Are already installed"
    fi

#----------------------------------------------------------------
# Check the see if the /var/lib/pgsql/data/ directory exists
#----------------------------------------------------------------

if [ -d "$pg_hba_conf_dir" ]; then
    echo "[Info    ] PostgreSQL Data Directory:         | $pg_hba_conf_dir already exists"
    LogMsg "[Info    ] PostgreSQL Data Directory:           | $pg_hba_conf_dir already exists"
else
    echo "[Error   ] PostgreSQL Data Directory:         | $pg_hba_conf_dir does not exist"
    echo "[Info    ] Something went wrong:              | In the PostgreSQL RPM install."
    echo "[Info    ] Please ensure the:                 | $pg_hba_conf_dir exists for proper setup."
    echo "${line1_out}"
    LogMsg "[Error   ] PostgreSQL Data Directory:           | $pg_hba_conf_dir does not exist"
    LogMsg "[Info    ] Something went wrong:                | In the PostgreSQL RPM install."
    LogMsg "[Info    ] Please ensure the:                   | $pg_hba_conf_dir exists for proper setup."
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting $0 script"
    echo "${line3_log}" >> $logfile
    exit 0
fi

#----------------------------------------------------------------
# Check the see if the pg_hba.conf file exists
#----------------------------------------------------------------

if [ -f $pg_hba_conf_file ]; then
    echo "[Info    ] PostgreSQL pg_hba.conf file:       | $pg_hba_conf_file already exists"
    LogMsg "[Info    ] PostgreSQL pg_hba.conf file:         | $pg_hba_conf_file already exists"
else
    echo "[Info    ] Running:                           | The initdb setup"
    su - postgres -c /usr/bin/initdb < /dev/null > /dev/null 2>&1
	
	#----------------------------------------------------------------
	# Check the see if the pg_hba.conf file exists after
	#----------------------------------------------------------------
	
	if [ -f $pg_hba_conf_file ]; then
	    echo "[Complete] PostgreSQL pg_hba.conf file:	      | $pg_hba_conf_file exists in directory"
	    LogMsg "[Complete] PostgreSQL pg_hba.conf file:         | $pg_hba_conf_file exists in directory"
	else
	    echo "[Error   ] File:                          | $pg_hba_conf_file does not exist in directory"
    	    echo "[Info    ] Please ensure:                 | the pg_hba.conf exists for proper setup."
	    LogMsg "[Error   ] File:                          | $pg_hba_conf_file does not exist in directory"
    	    LogMsg "[Info    ] Please ensure:                 | the pg_hba.conf exists for proper setup."
            LogMsg "${line2_log}"
            LogMsg "[End     ] Exiting $0 script"
            echo "${line1_out}"
	    exit 0 
	fi
fi

#----------------------------------------------------------------
# Check the status of the PostgreSQl server
#----------------------------------------------------------------

systemctl status postgresql > /dev/null

if [ $? -ne 0 ]; then
    echo "[Info    ] The PostgreSQL server:             | Is currently inactive and needs to be started"
    LogMsg "[Info    ] The PostgreSQL server:               | Is currently inactive and needs to be started"
    systemctl start postgresql > /dev/null
    
    if [ $? -eq 0 ]; then
    	echo "[Complete] The PostgreSQL server:             | Is currently running"
    	LogMsg "[Complete] The PostgreSQL server:         	   | Is currently running"
    else
	echo "[Error   ] The PostgreSQL server:             | Is not currently running"	
	echo "[Info    ] If there is an issue please run:   | systemctl status postgresql for more info."
	LogMsg "[Error   ] The PostgreSQL server:             | Is not currently running"	
	LogMsg "[Info    ] If there is an issue please run:   | systemctl status postgresql for more info."
    fi
fi

#-------------------------------------------------------
# Check to see if specific user root (as postgres user)
# Create if not already in system
#-------------------------------------------------------

if [ $current_user == "postgres" ]; then
    if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $root_user; then
        echo "[Info    ] PostgreSQL DB user:                | $root_user already exists"
        LogMsg "[Info    ] PostgreSQL DB user:                  | $root_user already exists"
    else
        create_users=`psql -v ON_ERROR_STOP=1 -X -t -q -U postgres -d postgres << THE_END 2>/dev/null
        CREATE USER $root_user;
THE_END`
        echo "[Complete] Created PostgreSQL DB user:        | $root_user"
        LogMsg "[Complete] Created PostgreSQL DB user:          | $root_user"
    fi
fi

#----------------------------------------------------------
# Check to see if specific user root (as root user) exists
# Create if not already in system
#----------------------------------------------------------

if [ $current_user != "postgres" ]; then
    root_query=$( su - postgres -c "psql -d postgres --tuples-only -P format=unaligned -c \"SELECT usename FROM pg_catalog.pg_user where usename = 'root'\"" )
    
    if [ "$root_query" != "root" ]; then
        echo "[Info    ] PostgreSQL DB user:                | $root_user does not exist"
        LogMsg "[Info    ] PostgreSQL DB user:                  | $root_user does not exist"
        sudo -u postgres createuser root
        echo "[Complete] PostgreSQL DB created user:        | $root_user"
        echo "$now ($current_user) [Complete] PostgreSQL DB created user:          | $root_user" >> $logfile
    else
        echo "[Info    ] PostgreSQL DB user:                | $root_user already exists"
        LogMsg "[Info    ] PostgreSQL DB user:                  | $root_user already exists"
    fi
fi

#-----------------------------------------------
# Check to see if specific user csmdb exists
# Create if not already in system
#-----------------------------------------------

if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
    echo "[Info    ] PostgreSQL DB user:                | $csmdb_user already exists"
    LogMsg "[Info    ] PostgreSQL DB user:                  | $csmdb_user already exists"
else
    create_users=`psql -v ON_ERROR_STOP=1 -X -t -q -U postgres -d postgres << THE_END 2>/dev/null
    CREATE USER $csmdb_user;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO $csmdb_user;
THE_END`
    echo "[Complete] PostgreSQL DB created user:        | $csmdb_user"
    echo "$now ($current_user) [Complete] PostgreSQL DB created user:          | $csmdb_user" >> $logfile
fi

#-----------------------------------------------
# Check to see if specific user xcatadm exists
# Create if not already in system
#-----------------------------------------------

if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $xcat_admin_user; then
    echo "[Info    ] PostgreSQL DB user:                | $xcat_admin_user already exists"
    LogMsg "[Info    ] PostgreSQL DB user:                  | $xcat_admin_user already exists"
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting $0 script"
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
else
    create_users=`psql -v ON_ERROR_STOP=1 -X -t -q -U postgres -d postgres << THE_END 2>/dev/null
    CREATE USER $xcat_admin_user;
THE_END`
    echo "[Complete] PostgreSQL DB created user:        | $xcat_admin_user"
    echo "$now ($current_user) [Complete] PostgreSQL DB created user:          | $xcat_admin_user" >> $logfile
    echo "${line1_out}"
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting $0 script"
    echo "${line3_log}" >> $logfile
fi
