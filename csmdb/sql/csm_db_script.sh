#!/bin/bash
#--------------------------------------------------------------------------------
#   
#    csm_db_script.sh
# 
#  © Copyright IBM Corporation 2015-2020. All Rights Reserved
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
#   usage:              ./csm_db_script.sh <----- to create the csm_db
#   current_version:    10.18
#   create:             12-14-2015
#   last modified:      04-04-2019
#--------------------------------------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'

OPTERR=0
DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
#logpath=`pwd` #<------ change this when pushing to the repo.
logname="csm_db_script.log"
cd "${BASH_SOURCE%/*}" || exit
cur_path=`pwd`
BASENAME=`basename "$0"`

#----------------------------------------------
# Output formatter
#----------------------------------------------
line1_out=$(printf "%0.s-" {1..120})
line2_log=$(printf "%0.s-" {1..87})
line3_log=$(printf "%0.s-" {1..131})

#----------------------------------------------
# CSM_database_files
#----------------------------------------------
create_tables_file="csm_create_tables.sql"
drop_tables_file="csm_drop_tables.sql"
delete_data_file="csm_delete_data.sql"
create_triggers_file="csm_create_triggers.sql"
drop_functions_file="csm_drop_functions.sql"

#----------------------------------------------
# CSM_table_name & CSM_table_data_array
#----------------------------------------------
# declare associative array
#----------------------------------------------
declare -A table_data

#-----------------------------------------------------------
# CSM_defined_tables
#-----------------------------------------------------------
csm_db_schema_version_table="csm_db_schema_version"
csm_db_schema_version_data="csm_db_schema_version_data.csv"

#----------------------------------------------------------------
# Assign elements to the array
#----------------------------------------------------------------
table_data[csm_ras_type]=csm_ras_type_data.csv
table_data[csm_db_schema_version]=csm_db_schema_version_data.csv
# table_data[table3]=table3.csv

#----------------------------------------------
# Current user connected
#----------------------------------------------
current_user=`id -u -n`
db_username="postgres"
csmdb_user="csmdb"
now1=$(date '+%Y-%m-%d %H:%M:%S')

#----------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files
# to /tmp directory.
#----------------------------------------------------------------
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

#----------------------------------------------
# Create User with privileges
#----------------------------------------------
function create_db_user () {
now=$(date '+%Y-%m-%d %H:%M:%S')
psql -t -q -U postgres -d postgres -c "CREATE USER $csmdb_user;" 2>/dev/null
psql -t -q -U postgres -d postgres -c "GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO $csmdb_user;" 2>/dev/null
echo "$now ($current_user) $1[Complete] $dbname database created user: $csmdb_user" >> $logfile
}

#----------------------------------------------
# Log messaging intro. header
#----------------------------------------------
echo "${line1_out}"
echo "[Start   ] Welcome to CSM database automation script."
LogMsg "[Start   ] Welcome to CSM database automation script."
LogMsg "${line2_log}"
echo "[Info    ] Log directory: $logfile"

#----------------------------------------------
# Usage function (help menu options)
#----------------------------------------------
function usage () {
echo "-----------------------------------------------------------------------------------------------------------------"
echo "[Info ] $BASENAME : CSM database creation script with additional features"
echo "[Usage] $BASENAME : [OPTION]... [DBNAME]... [OPTION]"
echo "-----------------------------------------------------------------------------------------------------------------"
echo "[Options]"
echo "-----------------------|-----------|-----------------------------------------------------------------------------"
echo "  Argument             | DB Name   | Description                                                                 "
echo "-----------------------|-----------|-----------------------------------------------------------------------------"
echo "  -x, --nodata         | [DEFAULT] | creates database with tables and does not pre populate table data           "
echo "                       | [db_name] | this can also be used with the -f --force, -n --newdb option when           "
echo "                       |           | recreating a DB. This should follow the specified DB name                   "
echo "  -d, --delete         | [db_name] | totally removes the database from the system                                "
echo "  -e, --eliminatetables| [db_name] | drops CSM tables from the database                                          "
echo "  -f, --force          | [db_name] | drops the existing tables in the DB, recreates and populates with table data"
echo "  -n, --newdb          | [db_name] | creates a new database with tables and populated data                       "
echo "  -r, --removetabledata| [db_name] | removes data from all database tables                                       "
echo "  -h, --help           |           | help                                                                        "
echo "-----------------------|-----------|-----------------------------------------------------------------------------"
echo "[Examples]"
echo "-----------------------------------------------------------------------------------------------------------------"
echo "  [DEFAULT] $BASENAME                         |          |"
echo "  [DEFAULT] $BASENAME -x, --nodata            |          |"
echo "            $BASENAME -d, --delete            | [DBNAME] |"
echo "            $BASENAME -e, --eliminatetables   | [DBNAME] |"
echo "            $BASENAME -f, --force             | [DBNAME] |"
echo "            $BASENAME -f, --force             | [DBNAME] | -x, --nodata"
echo "            $BASENAME -n, --newdb             | [DBNAME] |"
echo "            $BASENAME -n, --newdb             | [DBNAME] | -x, --nodata"
echo "            $BASENAME -r, --removetabledata   | [DBNAME] |"
echo "            $BASENAME -h, --help              |          |"
echo "-----------------------------------------------------------------------------------------------------------------"
}

#---------------------
# Default flags
#---------------------
createdb="no"
dropdb="no"
eliminate="no"
force="no"
newdb="no"
populate="yes"
removedata="no"
exclude_data="no"
dbname=$DEFAULT_DB

#----------------------------------------------------
# long options to short along with fixed length
#----------------------------------------------------
reset=true
for arg in "$@"
do
    if [ -n "$reset" ]; then
      unset reset
      set --      # this resets the "$@" array
    fi
    case "$arg" in
        --delete)               set -- "$@" -d ;;
        -delete)                usage && exit 0 ;;
        --eliminatetables)      set -- "$@" -e ;;
        -eliminatetables)       usage && exit 0 ;;
        --force)                set -- "$@" -f ;;
        -force)                 usage && exit 0 ;;
        --newdb)                set -- "$@" -n ;;
        -newdb)                 usage && exit 0 ;;
        --removetabledata)      set -- "$@" -r ;;
        -removetabledata)       usage && exit 0 ;;
        --nodata)               set -- "$@" -x ;;
        -nodata)                usage && exit 0 ;;
        --help)                 set -- "$@" -h ;;
        -help)                  usage && exit 0 ;;
        -d|-e|-f|-n|-r|-x|-h)   set -- "$@" "$arg" ;;
        #-*)                    usage && exit 0 ;;
        -*)                     usage 
                                LogMsg "[Info    ] Script execution: $BASENAME -h, --help"
                                LogMsg "[Info    ] Help menu query executed"
                                LogMsg "${line2_log}"
                                LogMsg "[End     ] Exiting csm_db_script.sh script"
                                echo "[Info  ] No arguments were passed in (Please choose appropriate option from usage list -h, --help)"
                                echo "${line3_log}" >> $logfile
                                exit 0 ;;
       #------ pass through anything else ------#
       *)                       set -- "$@" "$arg" ;;
    esac
done

#--------------------------------------------
# now we can drop into the short getopts
#--------------------------------------------
while getopts "d:n:e:xf:r:h" opt; do
    case $opt in
        d)
            #------ Drops the specified db completely ------#
            dropdb="yes"
            dbname=$OPTARG ;;
        e)
            #------ Eliminates existing csm db tables ------#
            eliminate="yes"
            dbname=$OPTARG ;;
        f)
            #------ This will overwrite the current db it already exists ------#
            force="yes"
            dbname=$OPTARG ;;
        n)
            #------ Create a new db with specified database name ------#
            newdb="yes"
            dbname=$OPTARG ;;
        r)
            #------ Removes the data from the specific tables ------#
            removedata="yes"
            dbname=$OPTARG ;;
        x)
            #------ Create a db without data user db ------#
            exclude_data="yes"
            populate="no"
            if [ $# -eq 1 ]; then
                createdb="yes"
            fi ;;
        h|*)
            usage
            LogMsg "[Info    ] Script execution: $BASENAME -h, --help"
            LogMsg "[Info    ] Help menu query executed"
            LogMsg "${line2_log}"
            LogMsg "[End     ] Exiting csm_db_script.sh script"
            echo "${line3_log}" >> $logfile
            exit 0 ;;
        \?)
            usage && exit 0 ;;
    esac
done

#----------------------------------------------------------
# Built in checks
#----------------------------------------------------------
# checks to see if no arguments are passed into the script 
#----------------------------------------------------------
if [ $# -eq 0 ]; then
    dbname=$DEFAULT_DB
    createdb="yes"
fi

#---------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting with creating a new database
#---------------------------------------------------------------------------------
if [[ ($newdb == "yes") ]]; then
    
    #------ Check combinations ------#
    if [[ ($force == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "${line1_out}"
        exit 0
    fi
fi

#-------------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting with creating the default database
#-------------------------------------------------------------------------------------
if [[ ($createdb == "yes") ]]; then
    
    #------ Check combinations ------#
    if [[ ($force == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($newdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "${line1_out}"
        exit 0
    fi
fi

#---------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting with removing a database
#---------------------------------------------------------------------------------
if [[ ($dropdb == "yes") ]]; then
    
    #------ Check combinations ------#
    if [[ ($force == "yes") || ($removedata == "yes") || ($newdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "${line1_out}"
        exit 0
    fi
fi

#---------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting with removing data from tables
#---------------------------------------------------------------------------------
if [[ ($removedata == "yes") ]]; then
    
    #------ Check combinations ------#
    if [[ ($force == "yes") || ($newdb == "yes") || ($dropdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "${line1_out}"
        exit 0
    fi
fi

#---------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting with force database
#---------------------------------------------------------------------------------
if [[ ($force == "yes") ]]; then
    
    #------ Check combinations ------#
    if [[ ($newdb == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "${line1_out}"
        exit 0
    fi
fi

#---------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting with eliminating tables
#---------------------------------------------------------------------------------
if [[ ($eliminate == "yes") ]]; then
    
    #------ Check combinations ------#
    if [[ ($newdb == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($createdb == "yes") || ($force == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "${line1_out}"
        exit 0
    fi
fi

#------------------------------------------------------------------------------------------------------
# Checks to see if it is a new database or force overwrite then do not populate the data within tables
#------------------------------------------------------------------------------------------------------
if [[ ($newdb == "yes") || ($force == "yes") ]]; then
    
    #------ Check combinations ------#
    if [ $exclude_data == "yes" ]; then
        populate="no"
    fi
fi

#----------------------------------------------------------------
# Check the status of the PostgreSQl server
#----------------------------------------------------------------

systemctl status postgresql > /dev/null
    
#------ Check return code ------#
if [ $? -ne 0 ]; then
    echo "[Error   ] The PostgreSQL server: Is currently inactive and needs to be started"
    echo "[Info    ] If there is an issue please run: systemctl status postgresql for more info."
    LogMsg "[Error   ] The PostgreSQL server: Is currently inactive and needs to be started"
    LogMsg "[Info    ] If there is an issue please run: systemctl status postgresql for more info."
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting csm_db_script.sh script"
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
fi

#---------------------------------------------------------------------------------
# Check if postgresql exists already and root user
#---------------------------------------------------------------------------------
root_query=`psql -qtA $db_username -c "SELECT usename FROM pg_catalog.pg_user where usename = 'root';" 2>&1`
    
    #------ Check return code ------#
    if [[ $? -ne 0 ]]; then
        echo "[Error   ] $root_query"
        echo "$root_query" |& awk '/^psql:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "[Info    ] Postgresql may not be configured correctly. Please check configuration settings."
        LogMsg "[Info    ] Postgresql may not be configured correctly. Please check configuration settings."
        LogMsg "${line2_log}"
        LogMsg "[End     ] Exiting csm_db_script.sh script"
        echo "${line1_out}"
        echo "${line3_log}" >> $logfile
        exit 0
    fi

#---------------------------------------------------------------------------------
# Check if postgresql exists already and DB name
#---------------------------------------------------------------------------------
string2="$now1 ($current_user) [Info    ] DB Names:"
    
    psql -lqt | cut -d \| -f 1 | grep -qw $dbname 2>>/dev/null
        
        #------ Check return code ------#
        if [ $? -ne 127 ]; then
            db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(datname,' | ') from pg_database;
EOF`
            echo "$string2 $db_query" | sed "s/.\{60\}|/&\n$string2 /g" >> $logfile
            echo "[Info    ] PostgreSQL is installed"
            LogMsg "[Info    ] PostgreSQL is installed"
            LogMsg "${line2_log}"
        else
            echo "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
            LogMsg "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
            LogMsg "${line2_log}"
            LogMsg "[End     ] Exiting csm_db_script.sh script"
            echo "${line1_out}"
            LogMsg "${line3_log}"
            exit 1
        fi

#--------------------------------------
# Check if database exists already
#--------------------------------------
db_exists="no"

psql -lqt | cut -d \| -f 1 | grep -qw $dbname

    #------ Check return code ------#
    if [ $? -eq 0 ]; then
        db_exists="yes"
    fi

    #------- End it if the input argument requires an existing database ------#
    if [ $db_exists == "no" ]; then
        
        #------ Database does not exist ------#
        LogMsg "[Info    ] $dbname database does not exist."
        
        #------ Check return code ------#
        if [[ ($removedata == "yes") || ($dropdb == "yes") || ($force == "yes") || ($eliminate == "yes") ]]; then
                echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
                echo "${line1_out}"
                LogMsg "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
                LogMsg "[Info    ] Database does not exist"
                LogMsg "${line2_log}"
                LogMsg "[End     ] Exiting csm_db_script.sh script"
                echo "${line3_log}" >> $logfile
                exit 0
        fi
    else
        #------ Database exists ------#
        if [[ ($newdb == "yes") || ($createdb == "yes") ]]; then
                LogMsg "[Error   ] Database already exists for: $dbname"
                echo "[Error   ] Cannot perform action because the $dbname database already exists. Exiting."
                echo "${line1_out}"
                LogMsg "[Error   ] Cannot perform action because the $dbname database already exists. Exiting."
                LogMsg "[Info    ] Database already exists."
                LogMsg "${line2_log}"
                LogMsg "[End     ] Exiting csm_db_script.sh script"
                echo "${line3_log}" >> $logfile
                exit 0
        fi
    fi

#---------------------------------
# Drop the entire database
#---------------------------------------------------------------------------
# return code added to ensure it was successful or failed during this step
#---------------------------------------------------------------------------
return_code=0

if [ $dropdb == "yes" ]; then
    echo "[Warning ] This will drop $dbname database including all tables and data. Do you want to continue [y/n]?"
    LogMsg "[Info    ] $dbname database drop process begin."
    
    #------ Reads in users response ------#
    read -s -n 1 drop_database
    case "$drop_database" in
        [yY][eE][sS]|[yY])
        echo "[Info    ] User response: $drop_database"
        LogMsg "[Info    ] User response: $drop_database"
            
            #------ Checks the current connections ------#
            connections_count=`psql -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname';"`
            
            #------ Connection count check ------#
            if [ $connections_count -gt 0 ]; then
                LogMsg "${line2_log}"
                LogMsg "[Error   ] $dbname will not be dropped because of existing connection(s) to the database."
                
                #------ Information related to the DB connections ------#
                db_conn_info=`psql -t -A -U $db_username -c "select to_char(now(),'YYYY-MM-DD HH12:MI:SS') || ' ($current_user) [Info    ] Current Connection | User: ' || usename || ' ', ' Datebase: ' || datname, ' Connection(s): ' || count(*), ' Duration: ' || now() - backend_start as Duration from pg_stat_activity WHERE datname='$dbname' group by usename, datname, Duration order by datname, Duration desc;" &>> $logfile`
                
                #------ User connected to the DB ------#
                connections=`psql -t -U $db_username -c "select distinct usename from pg_stat_activity WHERE datname='$dbname';"`
                
                #------ Checks the index array ------#
                if [ ${#connections[@]} -gt 0 ]; then
                    echo "[Error   ] $dbname will not be dropped because of existing connection(s) to the database."
                    
                    #------ Loops through in case of multiple connections ------#
                    for i in ${connections[@]}; do
                        connection_count=`psql -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname' and usename='$i';"`
                        shopt -s extglob
                        connection_count="${connection_count##*( )}"
                        connection_count="${connection_count%%*( )}"
                        echo "[Error   ] User: $i has $connection_count connection(s)"
                    done
                    echo "[Info    ] See log file for connection details"
                    echo "${line1_out}"
                fi
            else
                #------ This drops the DB if no connections are present ------#
                dropdb_no_conns=`dropdb $dbname -U $csmdb_user 2>&1`
                
                #------ Checks return code ------#
                if [ $? -eq 0  ]; then
                    echo "[Complete] $dbname database deleted"
                    LogMsg "[Complete] $dbname database deleted"
                    echo "${line1_out}"
                else
                    echo "[Error   ] Database delete failed for $dbname"
                    LogMsg "[Error   ] Database delete failed for $dbname"
                    echo "$dropdb_no_conns" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                    echo "[Info    ] Please see log file for specific DB error message(s)"
                    echo "$dropdb_no_conns" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                    echo "${line1_out}"
                    return_code=1
                fi
            fi
        ;;
        *)
            #------ Case if DB option "No" was passed in by user ------#
            echo "[Info    ] User response: $drop_database"
            LogMsg "[Info    ] User response: $drop_database"
            echo "[Info    ] Database not deleted"
            LogMsg "[Info    ] Database not deleted"
            echo "${line1_out}"
        ;;
    esac
    #------ End of process ------#
    LogMsg "[Info    ] $dbname database drop process end."
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting csm_db_script.sh script"
    echo "${line3_log}" >> $logfile
    exit $return_code
fi

#-----------------------------------------------------------------------------------------------------
# Drop csm db tables (Used if in another DB such as XCat. This will only drop CSM related tables)
#-----------------------------------------------------------------------------------------------------
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0

if [ $eliminate == "yes" ]; then
    echo "[Warning ] This will remove all CSM related tables from the DB including data, triggers, and functions. Do you want to continue [y/n]?:"
    LogMsg "[Info    ] $dbname database drop csm tables/functions/triggers process begin."
    
    #------ Reads in users response ------#
    read -s -n 1 eliminate_csm_tables
    
    #------ Check response value passed in ------#
    if [ "$eliminate_csm_tables" = "y" ]; then
        echo "[Info    ] User response: $eliminate_csm_tables"
        LogMsg "[Info    ] User response: $eliminate_csm_tables"
    
        #------ Drop DB table ------#
        drop_csm_tables=`psql -q -U $csmdb_user -d $dbname -f $drop_tables_file 2&>1`
        
        #------ Check return code ------#
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname csm tables and triggers dropped"
            LogMsg "[Complete] $dbname csm tables and triggers dropped"
            
            #------ Drop functions ------#
            drop_csm_func_trig=`psql -q -U $csmdb_user -d $dbname -f $drop_functions_file 2>&1`
            
            #------ Check return code ------#
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname csm functions dropped"
                LogMsg "[Complete] $dbname csm functions dropped"
            else
                echo "[Error   ] Csm functions drop failed for $dbname"
                LogMsg "[Error   ] Csm functions drop failed for $dbname"
                echo "[Error   ] Db Message: $drop_csm_func_trig" | awk '{print $0; exit}'
                echo "$drop_csm_func_trig" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0; exit}' >>"${logfile}"
                echo "${line1_out}"
                return_code=1
            fi
        else
            #------ Error drop functions ------#
            echo "[Error   ] Csm table and trigger drop failed for $dbname"
            LogMsg "[Error   ] Csm table and trigger drop failed for $dbname"
            echo "[Error   ] Db Message: $drop_csm_tables" | awk '{print $0; exit}'
            echo "$drop_csm_tables" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0; exit}' >>"${logfile}"
            echo "${line1_out}"
            return_code=1
        fi
        
        #------ Check return code ------#
        if [ $return_code -ne 0 ]; then
            echo "[Error   ] dropping CSM tables"
            LogMsg "[Error   ] dropping CSM tables"
            #LogMsg "${line2_log}"
            #LogMsg "[End     ] Exiting csm_db_script.sh script"
            echo "${line1_out}"
        fi
    else
        #------ Case if DB option "No" was passed in by user ------#
        echo "[Info    ] User response: $eliminate_csm_tables"
        LogMsg "[Info    ] User response: $eliminate_csm_tables"
        echo "[Info    ] Database not deleted"
        LogMsg "[Info    ] Database not deleted"
    fi
    #------ End of process ------#
    LogMsg "[Info    ] $dbname database drop csm tables/functions/triggers process end."
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting csm_db_script.sh script"
    echo "${line3_log}" >> $logfile
    echo "${line1_out}"
    exit $return_code
fi

#-----------------------------------------------
# Remove all data from the database tables
#-------------------------------------------------------------------------
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0

if [ $removedata == "yes" ]; then
    echo "[Info    ] This will remove all CSM related table data (excluding csm_db_schema_version and csm_db_schema_version_history tables)."
    echo "[Warning ] Do you want to continue [y/n]?:"
    LogMsg "[Info    ] $dbname database remove all data from the database tables begin."
    
    #------ Reads in users response ------#
    read -s -n 1 eliminate_csm_table_data
    
    #------ Check response value passed in ------#
    if [ "$eliminate_csm_table_data" = "y" ]; then
        echo "[Info    ] User response: $eliminate_csm_table_data"
        LogMsg "[Info    ] User response: $eliminate_csm_table_data"
    
        #------ Removes all data from tables except csm_db_schema_version table ------#
        db_remove_data=`psql -q -U $csmdb_user -d $dbname -f $delete_data_file 2>&1`
        
        #------ Check return code ------#
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database data deleted process complete."
            LogMsg "[Complete] $dbname database data deleted from all tables excluding csm_db_schema_version and csm_db_schema_version_history tables"
        else
            echo "[Error   ] Database data delete from all tables failed for $dbname"
            LogMsg "[Error   ] Database data delete from all tables failed for $dbname"
            echo "[Error   ] Db Message: $db_remove_data" | awk '{print $0; exit}'
            echo "$db_remove_data" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0; exit}' >>"${logfile}"
            echo "${line1_out}"
            return_code=1
        fi
        
        #------ Check return code ------#
        if [ $return_code -ne 0 ]; then
            echo "[Error   ] Error removing database data"
            LogMsg "[Error   ] Error removing database data"
        fi
    else
        #------ Case if DB option "No" was passed in by user ------#
        echo "[Info    ] User response: $eliminate_csm_table_data"
        LogMsg "[Info    ] User response: $eliminate_csm_table_data"
        echo "[Info    ] Database not deleted"
        LogMsg "[Info    ] Database not deleted"
    fi
    #------ End of process ------#
    LogMsg "[Info    ] $dbname database remove all data from the database tables end."
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting csm_db_script.sh script"
    echo "${line3_log}" >> $logfile
    echo "${line1_out}"
    exit $return_code
fi

#------------------------------
# Create a new database 
#-------------------------------------------------------------------------
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0

if [[ ($newdb == "yes") || ($createdb == "yes") ]]; then
    LogMsg "[Info    ] $dbname database creation process begin."
    
    #------ Checks to see if the DB user alread exists and if not then create the user with privileges ------#
    if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
        echo "[Info    ] $dbname database user: $csmdb_user already exists"
        LogMsg "[Info    ] $dbname database user: $csmdb_user already exists"
    else
        create_db_user
        echo "[Complete] database created user: $csmdb_user"
    fi
    
    #------ Create the DB with ownership ------#
    cdbwo=`psql -q -U $db_username -c "create database $dbname OWNER $csmdb_user" 2>&1`
    
    #------ Check return code ------#
    if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database created."
        LogMsg "[Complete] $dbname database created."
        
        #------ Create tables,views,etc ------#
        cndbtv=`psql -qtA -U $csmdb_user -d $dbname -f $create_tables_file 2>&1`
        
        #------ Check return code ------#
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database tables created."
            LogMsg "[Complete] $dbname database tables created."
            
            #------ Create triggers and functions ------#
            cndbtf=`psql -qtA -U $csmdb_user -d $dbname -f $create_triggers_file 2>&1`
            
            #------ Check return code ------#
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname database functions and triggers created."
                LogMsg "[Complete] $dbname database functions and triggers created."
                
                #------ Loops through the csv files that are currently available and places data into the specified db_tables ------#
                if [[ $populate == "yes" ]]; then
                    
                    #------ This sets the counter for the loop ------#
                    i=0
                    for i in "${!table_data[@]}"
                    do
                        table_name=$i
                        table_csv=${table_data[$i]}
                        
                        #------ Load prepopulated table data ------#
                        cndblcsv1="$now1 ($current_user) [Info    ] DB Message:"
                        cndblcsv=`psql -qtA -U $csmdb_user -d $dbname -c "\copy $table_name from '$cur_path/$table_csv' with csv header;" 2>&1`
                        
                        #------ Check return code ------#
                        if [ $? -eq 0  ]; then
                            echo "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                            LogMsg "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                        else
                            LogMsg "${line2_log}"
                            echo "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            LogMsg "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            echo "$cndblcsv" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                            echo "[Info    ] Please see log file for specific DB error message(s)"
                            echo "$cndblcsv" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                            echo "${line1_out}"
                            return_code=1
                        fi
                        ((i=i+1))
                    done
                    
                    #------ Initializes the DB schema version (Copies from csv file) ------#
                    schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                    shopt -s extglob
                    schema_version="${schema_version##*( )}"
                    schema_version="${schema_version%%*( )}"
                    echo "[Info    ] $dbname DB schema version ($schema_version)"
                    LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                    echo "${line1_out}"
                else
                    echo "[Info    ] $dbname skipping data load process"
                    LogMsg "[Info    ] $dbname skipping data load process"
                    
                    #------ Create DB schema version from csv file ------#
                    cndbwsv_csv=`psql -q -U $csmdb_user -d $dbname -c "\copy $csm_db_schema_version_table from '$cur_path/$csm_db_schema_version_data' with csv header;" 2>&1`
                    
                    #------ Check return code ------#
                    if [ $? -eq 0  ]; then
                        echo "[Complete] $dbname initialized $csm_db_schema_version_table data"
                        LogMsg "[Complete] $dbname initialized $csm_db_schema_version_table data"
                        schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                        shopt -s extglob
                        schema_version="${schema_version##*( )}"
                        schema_version="${schema_version%%*( )}"
                        
                        #------ Displays to screen + log message the schema version ------#
                        echo "[Info    ] $dbname DB schema version ($schema_version)"
                        LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                        echo "${line1_out}"
                    else
                        echo "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                        LogMsg "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                        echo "$cndbwsv_csv" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                        echo "[Info    ] Please see log file for specific DB error message(s)"
                        echo "$cndbwsv_csv" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                        LogMsg "${line2_log}"
                        LogMsg "[End     ] Exiting csm_db_script.sh script"
                        echo "${line1_out}"
                        return_code=1
                    fi
                fi
            else
                #------ Error create functions ------#
                LogMsg "${line2_log}"
                echo "[Error   ] Database functions and triggers creates failed for $dbname"
                LogMsg "[Error   ] Database functions and triggers creates failed for $dbname"
                echo "[Error   ] Db Message: $cndbtf" | awk '{print $0; exit}'
                echo "$cndbtf" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0; exit}' >>"${logfile}"
                echo "${line1_out}"
                return_code=1
            fi
        else
            #------ Error DB create tables ------#
            LogMsg "${line2_log}"
            echo "[Error   ] Database table creates failed for $dbname"
            LogMsg "[Error   ] Database table creates failed for $dbname"
            echo "[Error   ] Db Message: $cndbtv" | awk '{print $0; exit}'
            echo "$cndbtv" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0; exit}' >>"${logfile}"
            echo "${line1_out}"
            return_code=1
        fi
        
        #------ Check return code ------#
        if [ $return_code -ne 0 ]; then
            echo "[Error   ] creating database"
            LogMsg "[Error   ] creating database"
            echo "[Info    ] $dbname dropping database due to error"
            LogMsg "${line2_log}"
            LogMsg "[Info    ] $dbname dropping database due to error"
            echo "${line1_out}"
            
            #------ Drop DB if the process has failed ------#
            cndbdropdb=`dropdb $dbname -U $csmdb_user 2>&1`
            
            #------ Check return code ------#
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname database deleted"
                LogMsg "[Complete] $dbname database deleted"
                echo "${line1_out}"
            else
                echo "[Error   ] Database delete failed for $dbname"
                LogMsg "[Error   ] Database delete failed for $dbname"
                echo "$cndbdropdb" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                echo "[Info    ] Please see log file for specific DB error message(s)"
            echo "$cndbdropdb" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                echo "${line1_out}"
            fi
        fi
    else
        #------ Error handling for failed DB create ------#
        echo "[Error   ] Database create failed for $dbname"
        LogMsg "[Error   ] Database create failed for $dbname"
        echo "$cdbwo"
        echo "$cdbwo" |& awk '/^psql:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "${line1_out}"
        return_code=1
    fi
    #------ End of process ------#
    LogMsg "${line2_log}"
    LogMsg "[End     ] $dbname database creation process end."
    echo "${line3_log}" >> $logfile
    exit $return_code
fi

#--------------------------------------------------------------------------
# Drop existing database tables, functions and triggers then recreate them
#--------------------------------------------------------------------------
# return code added to ensure it was successful or failed during this step
#--------------------------------------------------------------------------
return_code=0

#------ If force option is selected then the script will execute this section ------#
if [[ $force == "yes" ]]; then
    LogMsg "[Info    ] $dbname database force/creation process begin."
    
    #------ Checks to see if the DB user alread exists and if not then create the user ------#
    if psql $csmdb_user -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
        echo "[Info    ] $dbname database user: $csmdb_user already exists"
        LogMsg "[Info    ] $dbname database user: $csmdb_user already exists"
    else
        create_db_user
        echo "[Complete] database created user: $csmdb_user"
    fi
    
    #------ Checks to see if file exists ------#
    declare -A files
    
    #------ Assign files to index array to be checked ------#
    files[0]=$cur_path/$drop_tables_file
    files[1]=$cur_path/$drop_functions_file
    files[2]=$cur_path/$create_tables_file
    files[3]=$cur_path/$create_triggers_file
    files[4]=$cur_path/csm_ras_type_data.csv
    files[5]=$cur_path/$csm_db_schema_version_data

    #------ Loop through the specific files to see if they exist ------#
    for f in "${files[@]}" ; do
        if [ ! -f $f ]; then
            echo "[Error   ] Cannot perform action because the $f does not exist."
            LogMsg "[Error   ] Cannot perform action because the $f does not exist."
            LogMsg "${line2_log}"
            LogMsg "[End     ] Exiting $0 script"
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
            exit 0
        fi
    done 

    #------ Drop all DB tables ------#
    dbf_drop_tabs=`psql -q -U $csmdb_user -d $dbname -f $drop_tables_file 2>&1`
    
    #------ Check return code ------#
    if [ $? -eq 0  ]; then 
        echo "[Complete] $dbname database tables and triggers dropped"
        LogMsg "[Complete] $dbname database tables and triggers dropped"
        
        #------ Drop all DB functions ------#
        dbf_dropfuncs=`psql -q -U $csmdb_user -d $dbname -f $drop_functions_file 2>&1`
        
        #------ Check return code ------#
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname csm functions dropped"
            LogMsg "[Complete] $dbname csm functions dropped"
            
            #------ Create DB tables ------#
            dbf_ctabs=`psql -q -U $csmdb_user -d $dbname -f $create_tables_file 2>&1`
            
            #------ Check return code ------#
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname database tables recreated."
                LogMsg "[Complete] $dbname database tables recreated."
                
                #------ Create DB triggers and functions ------#
                dbf_ctfs=`psql -q -U $csmdb_user -d $dbname -f $create_triggers_file 2>&1`
                
                #------ Check return code ------#
                if [ $? -eq 0  ]; then
                    echo "[Complete] $dbname database functions and triggers recreated."
                    LogMsg "[Complete] $dbname database functions and triggers recreated."
                    
                    #------ Loops through the csv files that are currently available and places data into the specified db_tables ------#
                    if [ $populate == "yes" ]; then
                        
                        #------ This sets the counter for the loop ------#
                        i=0
                        for i in "${!table_data[@]}";
                        do
                            table_name=$i
                            table_csv=(${table_data[$i]})
                            
                            #------ Load prepopulated table data ------#
                            dbf_loadtabdata=`psql -q -U $csmdb_user -d $dbname -c "\copy $table_name from '$cur_path/$table_csv' with csv header;" 2>&1`
                            
                            #------ Check return code ------#
                            if [ $? -eq 0  ]; then
                                echo "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                                LogMsg "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                            else
                                echo "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                                LogMsg "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                                echo "$dbf_loadtabdata" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                                echo "[Info    ] Please see log file for specific DB error message(s)"
                                echo "$dbf_loadtabdata" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                                echo "${line1_out}"
                                return_code=1
                            fi
                            ((i=i+1))
                        done
                        
                        #------ Initializes the DB schema version (Copies from csv file) ------#
                        schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                        shopt -s extglob
                        schema_version="${schema_version##*( )}"
                        schema_version="${schema_version%%*( )}"
                        echo "[Info    ] $dbname DB schema version ($schema_version)"
                        LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                        echo "${line1_out}"
                    else
                        echo "[Info    ] $dbname skipping data load process."
                        LogMsg "[Info    ] $dbname skipping data load process."
                        
                        #------ Create DB schema version from csv file ------#
                        dbf_dbsv_csv=`psql -q -U $csmdb_user -d $dbname -c "\copy $csm_db_schema_version_table from '$cur_path/$csm_db_schema_version_data' with csv header;" 2>&1`
                        
                        #------ Check return code ------#
                        if [ $? -eq 0  ]; then
                            echo "[Complete] $dbname table data loaded successfully into csm_db_schema_version"
                            LogMsg "[Complete] $dbname table data loaded successfully into csm_db_schema_version"
                            schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                            shopt -s extglob
                            schema_version="${schema_version##*( )}"
                            schema_version="${schema_version%%*( )}"
                            
                            #------ Displays to screen + log message the schema version ------#
                            echo "[Info    ] $dbname DB schema version ($schema_version)"
                            LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                            echo "${line1_out}"
                        else
                            echo "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                            LogMsg "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                            echo "$dbf_dbsv_csv" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                            echo "[Info    ] Please see log file for specific DB error message(s)"
                            echo "$dbf_dbsv_csv" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                            echo "${line1_out}"
                            return_code=1
                        fi
                    fi
                else
                    #------ Error handling for failed DB triggers and functions ------#
                    echo "[Error   ] Database functions and triggers creates failed for $dbname"
                    LogMsg "[Error   ] Database functions and triggers creates failed for $dbname"
                    echo "$dbf_ctfs" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                    echo "[Info    ] Please see log file for specific DB error message(s)"
                    echo "$dbf_ctfs" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                    echo "${line1_out}"
                    return_code=1
                fi
            else
                #------ Error handling for failed DB tables create ------#
                echo "[Error   ] Database table creates failed for $dbname"
                LogMsg "[Error    ]Database table creates failed for $dbname"
                echo "$dbf_ctabs" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                echo "[Info    ] Please see log file for specific DB error message(s)"
                echo "$dbf_ctabs" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                echo "${line1_out}"
                return_code=1
            fi
        else
            #------ Error handling for failed DB triggers and functions drop ------#
            echo "[Error   ] Csm functions and triggers drop failed for $dbname"
            LogMsg "[Error    ]Csm functions and triggers drop failed for $dbname"
            echo "$dbf_dropfuncs" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
            echo "[Info    ] Please see log file for specific DB error message(s)"
            echo "$dbf_dropfuncs" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
            echo "${line1_out}"
            return_code=1
        fi
    else
        #------ Error handling for failed DB table drop ------#
        echo "[Error   ] Table drop failed for $dbname"
        LogMsg "[Error    ]Table drop failed for $dbname"
        echo "$dbf_drop_tabs" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
        echo "[Info    ] Please see log file for specific DB error message(s)"
        echo "$dbf_drop_tabs" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "${line1_out}"
        return_code=1
    fi
    
    #------ Error handling for failed DB create ------#
    if [ $return_code -ne 0 ]; then
        echo "[Error   ] creating database"
        LogMsg "[Error    ] creating database"
        #LogMsg "${line2_log}"
        #LogMsg "[End     ] Exiting csm_db_script.sh script"
        echo "${line1_out}"
    fi
    
    #------ End of process ------#
    LogMsg "[Info    ] $dbname database force/creation process end."
    LogMsg "${line2_log}"
    LogMsg "[End     ] Exiting csm_db_script.sh script"
    echo "${line3_log}" >> $logfile
    exit $return_code
fi

#--------------------------------------------------------------------------
# Default action create a new database
#--------------------------------------------------------------------------
# return code added to ensure it was successful or failed during this step
#--------------------------------------------------------------------------
return_code=0

LogMsg "[Info    ] $dbname default database creation process begin."

#------ Checks to see if the DB user alread exists and if not then create the user with privileges ------#
if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
    echo "[Info    ] $dbname database user: $csmdb_user already exists"
    LogMsg "[Info    ] $dbname database user: $csmdb_user already exists"
else
    create_db_user
    echo "[Complete] database created user: $csmdb_user"
fi

#------ Create the DB with ownership ------#
def_cdbwo=`psql -q -U $db_username -c "create database $dbname OWNER $csmdb_user" 2>&1`

#------ Check return code ------#
if [ $? -eq 0  ]; then
    echo "[Complete] $dbname database created."
    LogMsg "[Complete] $dbname database created."
    
    #------ Create tables,views,etc ------#
    def_ctv=`psql -q -U $csmdb_user -d $dbname -f $create_tables_file 2>&1`
    
    #------ Check return code ------#
    if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database tables created"
        LogMsg "[Complete] $dbname database tables created"
        
        #------ Create DB triggers and functions ------#
        def_ctf=`psql -q -U $csmdb_user -d $dbname -f $create_triggers_file 2>&1`
        
        #------ Check return code ------#
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database functions and triggers recreated."
            LogMsg "[Complete] $dbname database functions and triggers recreated."
                    
                #------ Loops through the csv files that are currently available and places data into the specified db_tables ------#
                if [[ $populate == "yes" ]]; then
                    
                    #------ This sets the counter for the loop ------#
                    i=0
                    for i in "${!table_data[@]}"
                    do
                        table_name=$i
                        table_csv=${table_data[$i]}
                        
                        #------ Load prepopulated table data ------#
                        def_pltd=`psql -qtA -U $csmdb_user -d $dbname -c "\copy $table_name from '$cur_path/$table_csv' with csv header;" 2>&1`
                        
                        #------ Check return code ------#
                        if [ $? -eq 0  ]; then
                            echo "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                            LogMsg "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                        else
                            echo "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            LogMsg "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            echo "$def_pltd" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                            echo "[Info    ] Please see log file for specific DB error message(s)"
                            echo "$def_pltd" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                            LogMsg "${line2_log}"
                            LogMsg "[End     ] Exiting csm_db_script.sh script"
                            echo "${line1_out}"
                            return_code=1
                        fi
                        ((i=i+1))
                    done
                    
                    #------ Initializes the DB schema version (Copies from csv file) ------#
                    schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                    shopt -s extglob
                    schema_version="${schema_version##*( )}"
                    schema_version="${schema_version%%*( )}"
                    echo "[Info    ] $dbname DB schema version ($schema_version)"
                    LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                    echo "${line1_out}"
                else
                    echo "[Info    ] $dbname skipping data load process"
                    LogMsg "[Info    ] $dbname skipping data load process"
                    
                    #------ Create DB schema version from csv file ------#
                    def_dbsv=`psql -q -U $csmdb_user -d $dbname -c "\copy $csm_db_schema_version_table from '$cur_path/$csm_db_schema_version_data' with csv header;" 2>&1`
                    
                    #------ Check return code ------#
                    if [ $? -eq 0  ]; then
                        echo "[Complete] $dbname table data initialized ${table_name[$i]}"
                        LogMsg "[Complete] $dbname table data initialized ${table_name[$i]}"
                    else
                        echo "[Error   ] Table data initialization failed for $dbname ${table_name[$i]}"
                        LogMsg "[Error   ] Table data initialization failed for $dbname ${table_name[$i]}"
                        echo "$def_dbsv" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
                        echo "[Info    ] Please see log file for specific DB error message(s)"
                        echo "$def_dbsv" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
                        LogMsg "${line2_log}"
                        LogMsg "[End     ] Exiting csm_db_script.sh script"
                        echo "${line1_out}"
                        return_code=1
                    fi
                fi
            else
            #------ Error handling for failed DB triggers and functions ------#
            echo "[Error   ] Database functions and triggers creates failed for $dbname"
            LogMsg "[Error   ] Database functions and triggers creates failed for $dbname"
            echo "$def_ctf" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
            echo "[Info    ] Please see log file for specific DB error message(s)"
            echo "$def_ctf" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
            LogMsg "${line2_log}"
            LogMsg "[End     ] Exiting csm_db_script.sh script"
            echo "${line1_out}"
            return_code=1
        fi
    else
        #------ Error handling for failed DB tables create ------#
        echo "[Error   ] Database table creates failed for $dbname default"
        LogMsg "[Error    ] Database table creates failed for $dbname default"
        echo "$def_ctv" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
        echo "[Info    ] Please see log file for specific DB error message(s)"
        echo "$def_ctv" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        #LogMsg "${line2_log}"
        #LogMsg "[End     ] Exiting csm_db_script.sh script"
        echo "${line1_out}"
        return_code=1
    fi
    
    #------ Check return code ------#
    if [ $return_code -ne 0 ]; then
        echo "[Error   ] creating database"
        LogMsg "[Error   ] Error creating database"
        echo "[Info    ] $dbname dropping database due to error"
        LogMsg "[Info    ] $dbname dropping database due to error"
        echo "${line1_out}"
        
        #------ Drop DB if the process has failed ------#
        def_dropdb=`dropdb $dbname -U $csmdb_user 2>&1`
        
        #------ Check return code ------#
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database deleted"
            LogMsg "[Complete] $dbname database deleted"
        else
            echo "[Error   ] Database delete failed for $dbname"
            LogMsg "[Error   ] Database delete failed for $dbname"
            echo "$def_dropdb" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
            echo "[Info    ] Please see log file for specific DB error message(s)"
            echo "$def_dropdb" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
            echo "${line1_out}"
        fi
    fi
else
    #------ Error handling for failed DB create ------#
    echo "[Error   ] Database create failed for $dbname"
    LogMsg "[Error   ] Database create failed for $dbname"
    echo "$def_cdbwo" | awk '{print "'"[Error   ] DB Message: "'"$0; exit}'
    echo "[Info    ] Please see log file for specific DB error message(s)"
    echo "$def_cdbwo" | awk '{print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
    echo "${line1_out}"
    return_code=1
fi
#------ End of process ------#
LogMsg "[Info    ] $dbname database default/creation process end."
LogMsg "${line2_log}"
LogMsg "[End     ] Exiting csm_db_script.sh script"
echo "${line3_log}" >> $logfile
exit $return_code
