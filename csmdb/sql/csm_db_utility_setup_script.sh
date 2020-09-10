#!/bin/bash
#--------------------------------------------------------------------------------
#
#    csm_db_utility_setup_script.sh
#
#  © Copyright IBM Corporation 2019-2020. All Rights Reserved
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
#   current_version:    01.1
#   create:             02-20-2019
#   last modified:      09-08-2020
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
postgresql_conf_file="/var/lib/pgsql/data/postgresql.conf"

mydate=$(date +'%Y')
ORIG_FILE="$pg_hba_conf_file"
ORIG_FILE_2="$postgresql_conf_file"
TEMP_DIR="/tmp/csm_db_utility_temp/tmp_pg_hba_conf"

#----------------------------------------------
# Output formatter
#----------------------------------------------
line1_out=$(printf "%0.s-" {1..120})
line2_log=$(printf "%0.s-" {1..84})
line3_log=$(printf "%0.s-" {1..123})
line4_out=$(printf "%0.s-" {1..109})

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
echo "[Info    ] Log Directory:                      | $logfile"

#----------------------------------------------
# Usage Command Line Functions
#----------------------------------------------

function usage () {
echo "------------------------------------------------------------------------------------------------------------------------"
echo "[Info    ] $BASENAME : Load PostgreSQL DB onto the node if not already installed."
echo "[Info    ] $BASENAME : Load PostgreSQL DB configuration files and modify for local environment."
echo "[Info    ] $BASENAME : Load CSM DB root, csmdb, xcatadm user(s) if not presnt in system"
echo "[Usage   ] $BASENAME : $BASENAME (no additional arguments required)"
echo "------------------------------------------------------------------------------------------------------------------------"
echo "  Argument       | Description"
echo "-----------------|------------------------------------------------------------------------------------------------------"
echo "  script_name    | This script checks to see if the PostgreSQL RPMs are installed, the pg_hba.conf, postgres.conf files"
echo "                 | are present in the /var/lib/pgsql/data directory, checks the server status if its running, and also"
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

#----------------------------------------------------------------
# Below makes the directory if it does not exist
#----------------------------------------------------------------

if [[ ! -e $TEMP_DIR ]]; then
	mkdir -p $TEMP_DIR 2>>/dev/null

	if [ $? -ne 0 ]; then
		echo "[Error   ] make directory failed for: $TEMP_DIR" 2>>/dev/null
		echo "[Info    ] User: $current_user does not have permission to write to this directory"
		echo "[Info    ] Please specify a valid directory"
		echo "[Info    ] Or log in as the appropriate user"
		echo "${line1_out}"
		exit 1
	else
		chown -R root:root {$TEMP_DIR} >/dev/null 2>&1
		chmod -R 755 {$TEMP_DIR} >/dev/null 2>&1
fi
elif [[ ! -d $TEMP_DIR ]]; then
	echo "[Info   ] $TEMP_DIR already exists but is not a directory" 1>&2
	echo "${line1_out}"
	exit 1
fi

#----------------------------------------------------------------
# Clean up directory if anything is left over from previous runs
#----------------------------------------------------------------
if ls $TEMP_DIR/pg_* >/dev/null 2>&1;then
	rm -rf $TEMP_DIR/pg_*;
	rm -rf $TEMP_DIR/postgresql*;
fi

#------------------------------------------------
# Check to see if PostgreSQL RPMs are installed
# If not then check to see which version of RHELS
# is currently running.
#------------------------------------------------

rpm -qa | grep postgresql | grep server > /dev/null

if [ $? -ne 0 ]; then
				echo "[Info    ] PostgreSQL RPMs:                    | Are not installed (Installing)"
				yum install -q -y postgresql* >/dev/null 2>&1
		#-----------------------------------------------------
		# Check to see if the PostgreSQL RPMs were installed.
		#-----------------------------------------------------
		rpm -qa | grep postgresql | grep server > /dev/null
		if [ $? -ne 0 ]; then
			echo "[Error   ] PostgreSQL RPMs:                    | Are not installed."
			echo "[Info    ] Please check:                       | Your configuration settings or /etc/yum.repos.d repo."
			echo "[Info    ] Once the PostgreSQL RPMs:           | Are installed please re-run this script for setup"
			echo "${line1_out}"
			LogMsg "[Error   ] PostgreSQL RPMs:                      | Are not installed"
			LogMsg "[Info    ] Please check:                         | Your configuration settings or install PostreSQL RPMs"
			LogMsg "[Info    ] Once the PostgreSQL RPMs:             | Are installed please re-run this script for setup"
			LogMsg "${line2_log}"
			LogMsg "[End     ] Exiting $0 script"
			echo "${line3_log}" >> $logfile
			exit 1
		else
			echo "[Info    ] PostgreSQL RPMs:                    | Are now installed."
			LogMsg "[Info    ] PostgreSQL RPMs:                      | Are now installed."
		fi
else
	echo "[Info    ] PostgreSQL RPMs:                    | Are already installed."
	LogMsg "[Info    ] PostgreSQL RPMs:                      | Are already installed."
fi

#----------------------------------------------------------------
# Check the see if the /var/lib/pgsql/data/ directory exists
#----------------------------------------------------------------

if [ -d "$pg_hba_conf_dir" ]; then
	echo "[Info    ] PostgreSQL Data Directory:          | $pg_hba_conf_dir already exists"
	LogMsg "[Info    ] PostgreSQL Data Directory:            | $pg_hba_conf_dir already exists"
else
	echo "[Error   ] PostgreSQL Data Directory:          | $pg_hba_conf_dir does not exist"
	echo "[Info    ] Something went wrong:               | In the PostgreSQL RPM install."
	echo "[Info    ] Please ensure the:                  | $pg_hba_conf_dir exists for proper setup."
	echo "${line1_out}"
	LogMsg "[Error   ] PostgreSQL Data Directory:            | $pg_hba_conf_dir does not exist"
	LogMsg "[Info    ] Something went wrong:                 | In the PostgreSQL RPM install."
	LogMsg "[Info    ] Please ensure the:                    | $pg_hba_conf_dir exists for proper setup."
	LogMsg "${line2_log}"
	LogMsg "[End     ] Exiting $0 script"
	echo "${line3_log}" >> $logfile
	exit 1
fi

#----------------------------------------------------------------
# Check the see if the pg_hba.conf or the postgresql.conf files
# exists in the /var/lib/pgsql/data directory.
#----------------------------------------------------------------

if [ -f $pg_hba_conf_file ] && [ -f $postgresql_conf_file ]; then
	echo "[Info    ] PostgreSQL pg_hba.conf file:        | $pg_hba_conf_file already exists"
	echo "[Info    ] PostgreSQL postgresql.conf file:    | $postgresql_conf_file already exists"
	LogMsg "[Info    ] PostgreSQL pg_hba.conf file:          | $pg_hba_conf_file already exists"
	LogMsg "[Info    ] PostgreSQL postgres.conf file:        | $postgresql_conf_file already exists"
else
	echo "[Info    ] Running:                            | The initdb setup"
	su -c "postgresql-setup initdb" postgres > /dev/null 2>&1
	#su - postgres -c /usr/bin/initdb < /dev/null > /dev/null 2>&1

	#----------------------------------------------------------------
	# Check the see if the pg_hba.conf file exists after
	#----------------------------------------------------------------

	if [ -f $pg_hba_conf_file ]; then
		echo "[Info    ] PostgreSQL pg_hba.conf file:        | $pg_hba_conf_file exists in directory"
		LogMsg "[Info    ] PostgreSQL pg_hba.conf file:          | $pg_hba_conf_file exists in directory"
	else
		echo "[Error   ] File:                               | $pg_hba_conf_file does not exist in directory"
		echo "[Info    ] Please ensure:                      | the pg_hba.conf exists for proper setup."
		LogMsg "[Error   ] File:                                 | $pg_hba_conf_file does not exist in directory"
		LogMsg "[Info    ] Please ensure:                        | the pg_hba.conf exists for proper setup."
		LogMsg "${line2_log}"
		LogMsg "[End     ] Exiting $0 script"
		echo "${line1_out}"
		echo "${line3_log}" >> $logfile
		exit 1
	fi

		#-----------------------------------------------------------------------------
		# If the file exists we can now:
		# 1. Copy the original file as a backup.
		# 2. Modifiy the file to reflect the IP Addresses.
		# 3. Comment out the replication section if uncommented.
		# 4. Add in the local host address.
		#-----------------------------------------------------------------------------
		BACKUP_FILE="$TEMP_DIR/pg_hba.conf"
		MOVE_FILE=`cp $ORIG_FILE $BACKUP_FILE.$(date +%s)`
		CP_ORIG=`cp -p $ORIG_FILE $TEMP_DIR/pg_hba.conf`
		FILE=$TEMP_DIR/pg_hba.conf

		sed -i "s/local   all             all                                     peer/local   all             all                                     trust/" ${FILE}
		sed -i "s|host    all             all             127.0.0.1/32            ident|host    all             all             127.0.0.1/32            trust|" ${FILE}
		sed -i "s|host    all             all             ::1/128                 ident|host    all             all             ::1/128                 trust|" ${FILE}
		sed -i "/local   replication     all                                     peer/c\#local   replication     all                                     peer" ${FILE}
		sed -i "/host    replication     all             127.0.0.1\/32            ident/c\#host    replication     all             127.0.0.1/32            ident" ${FILE}
		sed -i "/host    replication     all             ::1\/128                 ident/c\#host    replication     all             ::1/128                 ident" ${FILE}

		sed -i "/^# IPv6 local connections:/i host    all             all             ${hostname}/32             trust" ${FILE}

	#--------------------------------------------------------------------------------
	# Check and compare the backup file with the newly generated file.
	#--------------------------------------------------------------------------------
		if cmp -s $TEMP_DIR/pg_hba.conf $TEMP_DIR/pg_hba.conf.* > /dev/null 2>&1; then
			RESULT_1=$?
			echo "[Info    ] The pg_hba.conf file                | is the same as the newly generated script. No additional steps needed."
			LogMsg "[Info    ] The pg_hba.conf file                  | is the same as the newly generated script. No additional steps needed."
			rm -rf $TEMP_DIR/*
		else
			RESULT_1=$?
			echo "${line1_out}"
			echo "[Info    ] The pg_hba.conf file                | is different from the newly generated script"
			echo "[Info    ] The newly generated pg_hba.conf     | file will replace the existing one."
			echo "[Info    ] The previous pg_hba.conf file       | will saved to the following dir: $pg_hba_conf_dir"
			LogMsg "[Info    ] The pg_hba.conf file                  | is different from the newly generated script"
			LogMsg "[Info    ] The newly generated pg_hba.conf       | file will replace the existing one."
			LogMsg "[Info    ] The previous pg_hba.conf file         | will saved to the following dir: $pg_hba_conf_dir"
			chown postgres:postgres $TEMP_DIR/pg_hba.conf.*
			mv $TEMP_DIR/pg_hba.conf.* $pg_hba_conf_dir
			mv $TEMP_DIR/pg_hba.conf $pg_hba_conf_file
		fi

		#----------------------------------------------------------------
		# Check the see if the postgresql.conf file exists after
		#----------------------------------------------------------------

		if [ -f $postgresql_conf_file ]; then
			echo "[Info    ] PostgreSQL postgresql.conf file:    | $postgresql_conf_file already exists"
			LogMsg "[Info    ] PostgreSQL pg_hba.conf file:          | $postgresql_conf_file already exists"
		else
			echo "[Error   ] File:                               | $postgresql_conf_file does not exist in directory"
			echo "[Info    ] Please ensure:                      | the postgresql.conf file exists for proper setup."
			LogMsg "[Error   ] File:                                 | $postgresql_conf_file_file does not exist in directory"
			LogMsg "[Info    ] Please ensure:                        | the postgresql.conf file exists for proper setup."
			LogMsg "${line2_log}"
			LogMsg "[End     ] Exiting $0 script"
			echo "${line1_out}"
			echo "${line3_log}" >> $logfile
			exit 1
		fi
		#-----------------------------------------------------------------------------
		# If the file exists we can now:
		# 1. copy the original file as a backup.
		# 2. modifiy the file to reflect the IP Addresses.
		#-----------------------------------------------------------------------------
		BACKUP_FILE_2="$TEMP_DIR/postgresql.conf"
		MOVE_FILE_2=`cp $ORIG_FILE_2 $BACKUP_FILE_2.$(date +%s)`
		CP_ORIG_2=`cp -p $ORIG_FILE_2 $TEMP_DIR/postgresql.conf`
		FILE_2=$TEMP_DIR/postgresql.conf

		sed -i "/#log_error_verbosity/c\log_error_verbosity = verbose           # terse, default, or verbose messages" ${FILE_2}
		sed -i "/#log_statement = /c\log_statement = 'all'                   # none, ddl, mod, all" ${FILE_2}
		echo "listen_addresses = 'localhost,$hostname'                # what IP address(es) to listen on;" >> ${FILE_2}

		#--------------------------------------------------------------------------------
		# Check and compare the backup file with the newly generated file.
		#--------------------------------------------------------------------------------
		if cmp -s $TEMP_DIR/postgresql.conf $TEMP_DIR/postgresql.conf.* > /dev/null 2>&1; then
			RESULT_2=$?
			echo "[Info    ] The postgresql.conf                 | file is the same as the newly generated script. No additional steps needed."
			LogMsg "[Info    ] The postgresql.conf                     | file is the same as the newly generated script. No additional steps needed."
			rm -rf $TEMP_DIR/*
		else
			RESULT_2=$?
			echo "[Info    ] The postgresql.conf file            | is different from the newly generated script"
			echo "[Info    ] The newly generated postgresql.conf | file will replace the existing one."
			echo "[Info    ] The previous postgresql.conf file   | will saved to the following dir: $pg_hba_conf_dir"
			echo "${line1_out}"
			LogMsg "[Info    ] The postgresql.conf file              | is different from the newly generated script"
			LogMsg "[Info    ] The newly generated postgresql.conf   | file will replace the existing one."
			LogMsg "[Info    ] The previous postgresql.conf file     | will saved to the following dir: $pg_hba_conf_dir"
			chown postgres:postgres $TEMP_DIR/postgresql.conf.*
			mv $TEMP_DIR/postgresql.conf.* $pg_hba_conf_dir
			mv $TEMP_DIR/postgresql.conf $postgresql_conf_file
		fi
fi
#--------------------------------------------------------------------------------
# Reset the PostgreSQL for the changed to take effect.
#--------------------------------------------------------------------------------
if [[ $RESULT_1 -eq 1 ]] || [[ $RESULT_2 -eq 1 ]]; then
	echo "[Info    ] Restarting                          | The postgreSQL server."
	LogMsg "[Info    ] Restarting                            | The postgreSQL server."
	systemctl start postgresql > /dev/null
fi

#----------------------------------------------------------------
# Check the see if the pg_hba.conf file exists after
#----------------------------------------------------------------

if [ -f $pg_hba_conf_file ]; then
	echo "[Info    ] PostgreSQL pg_hba.conf file:	       | $pg_hba_conf_file exists in directory"
	LogMsg "[Info    ] PostgreSQL pg_hba.conf file:          | $pg_hba_conf_file exists in directory"
else
	echo "[Error   ] File:                               | $pg_hba_conf_file does not exist in directory"
	echo "[Info    ] Please ensure:                      | the pg_hba.conf exists for proper setup."
	LogMsg "[Error   ] File:                                 | $pg_hba_conf_file does not exist in directory"
	LogMsg "[Info    ] Please ensure:                        | the pg_hba.conf exists for proper setup."
	LogMsg "${line2_log}"
	LogMsg "[End     ] Exiting $0 script"
	echo "${line1_out}"
	echo "${line3_log}" >> $logfile
	exit 1
fi
#----------------------------------------------------------------
# Check the see if the postgresql.conf file exists after
#----------------------------------------------------------------

if [ -f $postgresql_conf_file ]; then
	echo "[Info    ] PostgreSQL postgresql.conf file:    | $postgresql_conf_file exists in directory"
	LogMsg "[Info    ] PostgreSQL postgresql.conf file:      | $postgresql_conf_file exists in directory"
else
	echo "[Error   ] File:                               | $postgresql_conf_file does not exist in directory"
	echo "[Info    ] Please ensure:                      | the postgresql.conf exists for proper setup."
	LogMsg "[Error   ] File:                                 | $postgresql_conf_file does not exist in directory"
	LogMsg "[Info    ] Please ensure:                        | the postgresql.conf exists for proper setup."
	LogMsg "${line2_log}"
	LogMsg "[End     ] Exiting $0 script"
	echo "${line1_out}"
	echo "${line3_log}" >> $logfile
	exit 1
fi

#----------------------------------------------------------------
# Check the status of the PostgreSQl server
#----------------------------------------------------------------

systemctl status postgresql > /dev/null

if [ $? -ne 0 ]; then
	echo "[Info    ] The PostgreSQL server:              | Is currently inactive and needs to be started"
	LogMsg "[Info    ] The PostgreSQL server:                | Is currently inactive and needs to be started"
	systemctl start postgresql > /dev/null

		if [ $? -eq 0 ]; then
			echo "[Info    ] The PostgreSQL server:              | Is currently running"
			LogMsg "[Info    ] The PostgreSQL server:         	    | Is currently running"
		else
				echo "[Error   ] The PostgreSQL server:              | Is not currently running after server start."
				echo "[Info    ] If there is an issue please run:    | systemctl status postgresql for more info."
				LogMsg "[Error   ] The PostgreSQL server:                | Is not currently running after server start."
				LogMsg "[Info    ] If there is an issue please run:      | systemctl status postgresql for more info."
				LogMsg "${line2_log}"
				LogMsg "[End     ] Exiting $0 script"
				echo "${line1_out}"
				echo "${line3_log}" >> $logfile
				exit 1
		fi
fi

#-------------------------------------------------------
# Display the PostgreSQL version
#-------------------------------------------------------
PSQL_VERSION=$(postgres -V | awk '{ print $3 }')
	echo "[Info    ] PostgreSQL Version:                 | ($PSQL_VERSION)"
	LogMsg "[Info    ] PostgreSQL Version:                   | ($PSQL_VERSION)"

#-------------------------------------------------------
# Check to see if specific user root (as postgres user)
# Create if not already in system
#-------------------------------------------------------

if [ $current_user == "postgres" ]; then
	if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $root_user; then
	echo "${line1_out}"
	echo "[Info    ] PostgreSQL DB user:                 | $root_user already exists"
	LogMsg "[Info    ] PostgreSQL DB user:                   | $root_user already exists"
else
	create_users=`psql -v ON_ERROR_STOP=1 -X -t -q -U postgres -d postgres << THE_END 2>/dev/null
	CREATE USER $root_user;
THE_END`
	echo "${line1_out}"
	echo "[Info    ] Created PostgreSQL DB user:         | $root_user"
	LogMsg "[Info    ] Created PostgreSQL DB user:           | $root_user"
	fi
fi

#----------------------------------------------------------
# Check to see if specific user root (as root user) exists
# Create if not already in system
#----------------------------------------------------------

if [ $current_user != "postgres" ]; then
	root_query=$( su - postgres -c "psql -d postgres --tuples-only -P format=unaligned -c \"SELECT usename FROM pg_catalog.pg_user where usename = 'root'\"" )

	if [ "$root_query" != "root" ]; then
		echo "${line1_out}"
		echo "[Info    ] PostgreSQL DB user:                 | $root_user does not exist"
		LogMsg "[Info    ] PostgreSQL DB user:                   | $root_user does not exist"
		sudo -u postgres createuser root
		echo "[Info    ] PostgreSQL DB created user:         | $root_user"
		echo "$now ($current_user) [Info    ] PostgreSQL DB created user:           | $root_user" >> $logfile
	else
		echo "${line1_out}"
		echo "[Info    ] PostgreSQL DB user:                 | $root_user already exists"
		LogMsg "[Info    ] PostgreSQL DB user:                   | $root_user already exists"
	fi
fi

#-----------------------------------------------
# Check to see if specific user csmdb exists
# Create if not already in system
#-----------------------------------------------

if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
	echo "[Info    ] PostgreSQL DB user:                 | $csmdb_user already exists"
	LogMsg "[Info    ] PostgreSQL DB user:                   | $csmdb_user already exists"
else
	create_users=`psql -v ON_ERROR_STOP=1 -X -t -q -U postgres -d postgres << THE_END 2>/dev/null
	CREATE USER $csmdb_user;
	GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO $csmdb_user;
THE_END`
	echo "[Info    ] PostgreSQL DB created user:         | $csmdb_user"
	echo "$now ($current_user) [Info    ] PostgreSQL DB created user:           | $csmdb_user" >> $logfile
fi

#-----------------------------------------------
# Check to see if specific user xcatadm exists
# Create if not already in system
#-----------------------------------------------

if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $xcat_admin_user; then
	echo "[Info    ] PostgreSQL DB user:                 | $xcat_admin_user already exists"
	LogMsg "[Info    ] PostgreSQL DB user:                   | $xcat_admin_user already exists"
	LogMsg "${line2_log}"
	LogMsg "[End     ] Exiting $0 script"
	echo "${line1_out}"
	echo "${line3_log}" >> $logfile
else
	create_users=`psql -v ON_ERROR_STOP=1 -X -t -q -U postgres -d postgres << THE_END 2>/dev/null
	CREATE USER $xcat_admin_user;
THE_END`
	echo "[Info    ] PostgreSQL DB created user:         | $xcat_admin_user"
	echo "$now ($current_user) [Info    ] PostgreSQL DB created user:           | $xcat_admin_user" >> $logfile
	echo "${line1_out}"
	LogMsg "${line2_log}"
	LogMsg "[End     ] Exiting $0 script"
	echo "${line3_log}" >> $logfile
fi
