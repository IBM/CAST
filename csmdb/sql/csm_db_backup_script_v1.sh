#!/bin/bash
#================================================================================
#
#    csm_db_backup_script_v1.sh
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

#================================================================================
#   usage:              Backup CSM DB related data, tables, triggers, functions, etc.
#   current_version:    1.2
#   created:            03-26-2018
#   last modified:      12-05-2018
#================================================================================

#----------------------------------------------------------------
# Defined variables
#----------------------------------------------------------------
# Command line variables passed in:
# 1. Database Name,
# 2. Value of archive records to be processed
#----------------------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'
OPTERR=0

DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
backupdir="/var/lib/pgsql/backups/"
logname="csm_db_backup_script.log"
cd "${BASH_SOURCE%/*}" || exit
dbname=$DEFAULT_DB
BASENAME=`basename "$0"`
now=$(date '+%Y-%m-%d.%H.%M.%S.%N')

script_name="csm_db_backup_script_v1.sh"

line1_out="------------------------------------------------------------------------------------------------------------------------"
line2_log="------------------------------------------------------------------------------------"
line3_log="-------------------------------------------------------------------------------------------------------------------"

#-------------------------------------------------------------------------------
# Current user connected
#-------------------------------------------------------------------------------

current_user=`id -u -n`
db_username="postgres"
now1=$(date '+%Y-%m-%d %H:%M:%S')

#------------------------------------------------------------------------------------------------------------------------------
# Usage function for -h --help options
#------------------------------------------------------------------------------------------------------------------------------

function usage () {
    echo "================================================================================================================="
    echo "[Info ] $BASENAME : csmdb /tmp/csmdb_backup/"
    echo "[Info ] $BASENAME : csmdb"
    #echo "----------------------------------------------------------------------------------------------------------------"
    echo "[Usage] $BASENAME : [OPTION]... [/DIR/]"
    echo "-----------------------------------------------------------------------------------------------------------------"
    echo "[Options]"
    echo "----------------|------------------------------------------------------------------------------------------------"
    echo "  Argument      | Description"
    echo "----------------|------------------------------------------------------------------------------------------------"
    echo "   -h, --help   | help menu"
    echo "----------------|------------------------------------------------------------------------------------------------"
    echo "[Examples]"
    echo "-----------------------------------------------------------------------------------------------------------------"
    echo "   $BASENAME [DBNAME]                 | (default) will backup database to /var/lib/pgpsql/backups/ (directory)"
    echo "   $BASENAME [DBNAME] [/DIRECTORY/]   | will backup database to specified directory"
    echo "                                                       | if the directory doesn't exist then it will be made and written."
    echo "================================================================================================================="
}

#----------------------------------------------------
# long options to short along with fixed length
#----------------------------------------------------

for arg in "$@"; do
  shift
  case "$arg" in
    --help)     set -- "$@" -h ;;
    -h)         set -- "$@" "$arg" ;;
    -*)         usage 2>>/dev/null &&
                exit 0 ;;
    *)          set -- "$@" "$arg"
  esac
done

# Default behavior
rest=false; ws=false

#--------------------------------------------
# now we can drop into the short getopts
#--------------------------------------------

OPTIND=1

while getopts "h" opt; do
  case $opt in
    h) usage && exit 0 ;;
    ?) usage >&2 exit 1 ;;
  esac
done
shift $(expr $OPTIND - 1) # remove options from positional parameters

#----------------------------------------------------------------
# Below checks the arguments passed in on the command line
#----------------------------------------------------------------

    if [ "$#" -ne 1 ] && [ "$#" -ne 2 ]; then
        echo "[Info   ] Database name is required"
        usage
        exit 1
    fi

    dbname=$1
    
    if [[ "$#" -eq 2 ]]; then
        data_dir=$2
    else
        data_dir=$backupdir
    fi
        cur_path=$data_dir
        #logpath=$data_dir #<----- This file will live in "/var/log/ibm/csm/db"

#----------------------------------------------------------------
# Below makes the directory if it does not exist
#----------------------------------------------------------------

    if [[ ! -e $data_dir ]]; then
        mkdir -p $data_dir 2>>/dev/null
        if [ $? -ne 0 ]; then
            echo "${line1_out}"
            echo "[Error  ] make directory failed for: $data_dir" 2>>/dev/null
            echo "[Info   ] User: $current_user does not have permission to write to this directory"
            echo "[Info   ] Please specify a valid directory"
            echo "[Info   ] Or log in as the appropriate user"
            echo "${line1_out}"
            exit 1
        else
            chown postgres:postgres $data_dir
            chmod 755 $data_dir
        fi
    elif [[ ! -d $data_dir ]]; then
        echo "$data_dir already exists but is not a directory" 1>&2
        exit 1
    fi

#-------------------------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory
#-------------------------------------------------------------------------------

     if [ -d "$logpath" -a -w "$logpath" ]; then #<--- if logpath exist and u have permission
         logdir="$logpath"
     else
         logdir="/tmp"
     fi
     logfile="${logdir}/${logname}"

#-------------------------------------------------------------------------------
# Log Message
#-------------------------------------------------------------------------------

    function LogMsg () {
    LogTime=$(date '+%Y-%m-%d.%H:%M:%S')
    echo "$LogTime ($current_user) $1" >> $logfile
    }

    echo "${line1_out}"
    echo "[Start   ] Welcome to CSM datatbase backup process:"
    LogMsg "[Start ] Welcome to CSM datatbase backup process:"
    LogMsg "${line2_log}"

#-------------------------------------------------
# Check if postgresql exists already and root user
#-------------------------------------------------
string1="$now1 ($current_user) [Info  ] DB Users:"
    psql -U $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw root
        if [ $? -ne 0 ]; then
            db_user_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(usename,' | ') from pg_user;
EOF`
            echo "$string1 $db_user_query" | sed "s/.\{60\}|/&\n$string1 /g" >> $logfile
            echo "[Error   ] Postgresql may not be configured correctly. Please check configuration settings."
            LogMsg "[Error ] Postgresql may not be configured correctly. Please check configuration settings."
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
            exit 0
        fi

#-------------------------------------------------
# Check if postgresql exists already and DB name
#-------------------------------------------------
string2="$now1 ($current_user) [Info  ] DB Names:"
    psql -lqt | cut -d \| -f 1 | grep -qw $dbname 2>>/dev/null
        if [ $? -eq 0 ]; then       #<------------This is the error return code
            db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(datname,' | ') from pg_database;
EOF`
            echo "$string2 $db_query" | sed "s/.\{60\}|/&\n$string2 /g" >> $logfile
            LogMsg "[Info  ] PostgreSQL is installed"
        else
            echo "${line1_out}"
            echo "[Error ] PostgreSQL may not be installed or DB: $dbname may not exist."
            echo "[Info  ] Please check configuration settings or psql -l"
            echo "${line1_out}"
            LogMsg "[Error ] PostgreSQL may not be installed or DB $dbname may not exist."
            LogMsg "[Info  ] Please check configuration settings or psql -l"
            echo "${line3_log}" >> $logfile
            exit 1
        fi

        LogMsg "[Info  ] csm_db_backup_script_v1.sh"

#----------------------------------------------------------------
# Check if database exists
#----------------------------------------------------------------

    db_exists="no"
    psql -lqt | cut -d \| -f 1 | grep -qw $dbname
        if [ $? -eq 0 ]; then
        db_exists="yes"
        fi

#----------------------------------------------------------------
# End it if the input argument requires an existing database
# (Database does not exist)
#----------------------------------------------------------------

    if [ $db_exists == "no" ]; then

        LogMsg "[Info  ] Database does not exist."
        echo "${line1_out}"
        echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
        echo "[Info    ] Please provide a valid DB that exists on the system (hint: psql -l)."
        echo "[Info    ] Backup/log directory:    | $data_dir"
        echo "${line1_out}"
        LogMsg "[Error ] Cannot perform action because the $dbname database does not exist. Exiting."
        LogMsg "[Info  ] Backup/log directory:  |   $cur_path"
        echo "${line2_log}" >> $logfile
        LogMsg "[End   ] Please provide a valid DB that exists on the system (hint: psql -l)."
        echo "${line3_log}" >> $logfile
        exit 1
    fi

#----------------------------------------------------------------
# All the raw combined timing results before trimming
#----------------------------------------------------------------

    all_results="$data_dir/$dbname.backup.$now.timings"

#----------------------------------------------------------------
# Query the DB for the schema version (used in backup file name)
#----------------------------------------------------------------

s_ver=`psql -U $db_username -d $dbname -t -c "SELECT version from csm_db_schema_version;"`
trim=$(echo "$s_ver" | sed 's/^ //')

#----------------------------------------------------------------
# Query the DB for the connection count
#----------------------------------------------------------------

conn_count=`psql -U $db_username -d postgres -tA -c "SELECT sum(numbackends) AS DB_Conns FROM pg_stat_database where datname='$dbname';"`
conn_trim=$(echo "$conn_count" | sed 's/^ //') 

#----------------------------------------------------------------
# Checks the connections to the DB
# If no connections then backup
# If connections then exit script with error message
#----------------------------------------------------------------

if [ $conn_count == "0" ]; then
    #echo "${line1_out}"
    echo "[Info    ] There are no connections to: $dbname DB                                "
    LogMsg "[Info  ] There are no connections to: $dbname DB                                  "
    echo "[Info    ] Backing up: $dbname DB                                                 "
    LogMsg "[Info  ] Backing up: $dbname DB                                                   "
    [ "${data_dir: -1}" != "/" ] && data_dir="${data_dir}/"
    pg_dump -U $db_username -Fc $dbname > "${data_dir}${dbname}_${trim}_`date +%d-%m-%Y"_"%H_%M_%S`.backup"
else
    echo "${line1_out}"
    echo "[Error   ] Cannot perform action because the $dbname database currently has connections. Exiting."
    LogMsg "[Error ] Cannot perform action because the $dbname database currently has connections. Exiting."
    echo "[Info    ] Please kill all connections to $dbname database before backing up."
    LogMsg "[Info  ] Please kill all connections to $dbname database before backing up."
    echo "[Info    ] (hint: run ./csm_db_connections_script.sh -h for more options)."
    LogMsg "[Info  ] (hint: run ./csm_db_connections_script.sh -h for more options)."
fi

#----------------------------------------------------------------
# Output results to the screen.
# 1. DB name
# 2. Connection count
# 3. DB schema version
# 4. Backup log directory
#----------------------------------------------------------------

    echo "[Info    ] ${line2_log}"
    LogMsg "${line2_log}"
    echo "[Info    ] Database_Name:           |  $dbname"
    LogMsg "[Info  ] Database_Name:         |   $dbname"
    echo "[Info    ] Connection_count:        |  $conn_count"
    LogMsg "[Info  ] Connection_count:      |   $conn_count"
    echo "[Info    ] DB_Version:              |  $trim"
    LogMsg "[Info  ] DB_Version:            |   $trim"
    echo "[Info    ] Backup directory:        | $data_dir"
    LogMsg "[Info  ] Backup directory:      |   $cur_path"
    echo "[Info    ] Log directory:           | $logdir/$logname"
    LogMsg "[Info  ] Log directory:         |   $logdir/$logname"
    
    #-----------------------------------------
    # Logging info based on connection status
    #-----------------------------------------
    if [ $conn_count == "0"  ]; then
        echo "${line3_log}" >> $logfile
        LogMsg "[End   ] Backup process complete"
    else
        echo "${line2_log}" >> $logfile
        LogMsg "[End   ] Backup process aborted"
    fi
    echo "${line1_out}"
echo "${line3_log}" >> $logfile
