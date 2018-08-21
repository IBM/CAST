#================================================================================
#   
#    csm_db_script.sh
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
#   usage:              ./csm_db_script.sh <----- to create the csm_db
#   current_version:    10.14
#   create:             12-14-2015
#   last modified:      08-08-2018
#================================================================================

export PGOPTIONS='--client-min-messages=warning'

OPTERR=0
DEFAULT_DB="csmdb"
#logpath="/var/log/ibm/csm/db"
logpath=`pwd` #<------ change this when pushing to the repo.
logname="csm_db_script.log"
cd "${BASH_SOURCE%/*}" || exit
cur_path=`pwd`
BASENAME=`basename "$0"`

#==============================================
# CSM_database_files
#==============================================
create_tables_file="csm_create_tables.sql"
drop_tables_file="csm_drop_tables.sql"
delete_data_file="csm_delete_data.sql"
create_triggers_file="csm_create_triggers.sql"
drop_functions_file="csm_drop_functions.sql"

#==============================================
# CSM_table_name & CSM_table_data_array
#==============================================
# declare associative array
#==============================================
declare -A table_data

#===========================================================
# CSM_defined_tables
#===========================================================
csm_db_schema_version_table="csm_db_schema_version"
csm_db_schema_version_data="csm_db_schema_version_data.csv"

#================================================================
# Assign elements to the array
#================================================================
table_data[csm_ras_type]=csm_ras_type_data.csv
table_data[csm_db_schema_version]=csm_db_schema_version_data.csv
# table_data[table3]=table3.csv

#==============================================
# Current user connected
#==============================================
current_user=`id -u -n`
db_username="postgres"
csmdb_user="csmdb"
now1=$(date '+%Y-%m-%d %H:%M:%S')

#==============================================================
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files
# to /tmp directory.
#==============================================================
#if [ -d "$logpath" ]; then
if [ -w "$logpath" ]; then
    logdir="$logpath"
else
    logdir="/tmp"
fi
logfile="${logdir}/${logname}"

#==============================================
# Log Message
#==============================================
function LogMsg () {
now=$(date '+%Y-%m-%d %H:%M:%S')
echo "$now ($current_user) $1" >> $logfile
}

#==============================================
# Create User with privileges
#==============================================
function create_db_user () {
now=$(date '+%Y-%m-%d %H:%M:%S')
psql -t -q -U postgres -d postgres -c "CREATE USER $csmdb_user;" 2>/dev/null
psql -t -q -U postgres -d postgres -c "GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO $csmdb_user;" 2>/dev/null
echo "$now ($current_user) $1[Complete] $dbname database created user: $csmdb_user" >> $logfile
}


#================================
# Log messaging intro. header
#================================
echo "-----------------------------------------------------------------------------------------------------------------"
echo "[Start   ] Welcome to CSM database automation script."
LogMsg "[Start   ] Welcome to CSM database automation script."

#=====================================
# Usage function (help menu options)
#=====================================
function usage () {
echo "================================================================================================================="
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
echo "================================================================================================================="
}

#=====================
# Default flags
#=====================
createdb="no"
dropdb="no"
eliminate="no"
force="no"
newdb="no"
populate="yes"
removedata="no"
exclude_data="no"
dbname=$DEFAULT_DB

#====================================================
# long options to short along with fixed length
#====================================================
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
                                LogMsg "[End     ] Help menu query executed"
                                echo "[Info  ] No arguments were passed in (Please choose appropriate option from usage list -h, --help)"
                                echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
                                exit 0 ;;
       #============================================
       # pass through anything else
       #============================================
       *)                       set -- "$@" "$arg" ;;
    esac
done

#============================================
# now we can drop into the short getopts
#============================================
while getopts "d:n:e:xf:r:h" opt; do
    case $opt in
        d)
            #===================================
            # Drops the specified db completely
            #===================================
            dropdb="yes"
            dbname=$OPTARG ;;
        e)
            #===================================
            # Eliminates existing csm db tables
            #===================================
            eliminate="yes"
            dbname=$OPTARG ;;
        f)
            #===================================
            # This will overwrite the current
            # db it already exists
            #===================================
            force="yes"
            dbname=$OPTARG ;;
        n)
            #===================================
            # Create a new db with specified
            # database name
            #===================================
            newdb="yes"
            dbname=$OPTARG ;;
        r)
            #===================================
            # Removes the data from the specific
            # tables
            #===================================
            removedata="yes"
            dbname=$OPTARG ;;
        x)
            #===================================
            # Create a db without data user db
            #===================================
            exclude_data="yes"
            populate="no"
            if [ $# -eq 1 ]; then
                createdb="yes"
            fi ;;
        h|*)
            #usage && exit 0 ;;
            usage
            LogMsg "[Info    ] Script execution: $BASENAME -h, --help"
            LogMsg "[End     ] Help menu query executed"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            exit 0 ;;
        \?)
            usage && exit 0 ;;
    esac
done
#shift $((OPTIND-1))

#==========================================================
#exit #Remove after done testing indent: command not found
#==========================================================

#==========================================================
# Built in checks
#==========================================================
# checks to see if no arguments are passed into the script 
#----------------------------------------------------------
if [ $# -eq 0 ]; then
    dbname=$DEFAULT_DB
    createdb="yes"
fi

#=================================================================================
# Checks to see if input arguments are conflicting with creating a new database
#=================================================================================
if [[ ($newdb == "yes") ]]; then
    if [[ ($force == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "-----------------------------------------------------------------------------------------------------------------"
        exit 0
    fi
fi
#======================================================================================
# Checks to see if input arguments are conflicting with creating the default database
#======================================================================================
if [[ ($createdb == "yes") ]]; then
    if [[ ($force == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($newdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "-----------------------------------------------------------------------------------------------------------------"
        exit 0
    fi
fi
#==============================================================================
# Checks to see if input arguments are conflicting with removing a database
#==============================================================================
if [[ ($dropdb == "yes") ]]; then
    if [[ ($force == "yes") || ($removedata == "yes") || ($newdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "-----------------------------------------------------------------------------------------------------------------"
        exit 0
    fi
fi
#===================================================================================
# Checks to see if input arguments are conflicting with removing data from tables
#===================================================================================
if [[ ($removedata == "yes") ]]; then
    if [[ ($force == "yes") || ($newdb == "yes") || ($dropdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "-----------------------------------------------------------------------------------------------------------------"
        exit 0
    fi
fi
#==========================================================================
# Checks to see if input arguments are conflicting with force database
#==========================================================================
if [[ ($force == "yes") ]]; then
    if [[ ($newdb == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($createdb == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "-----------------------------------------------------------------------------------------------------------------"
        exit 0
    fi
fi
#=============================================================================
# Checks to see if input arguments are conflicting with eliminating tables
#=============================================================================
if [[ ($eliminate == "yes") ]]; then
    if [[ ($newdb == "yes") || ($removedata == "yes") || ($dropdb == "yes") || ($createdb == "yes") || ($force == "yes") ]]; then
        echo "[Error   ] arguments conflict"
        usage
        echo "-----------------------------------------------------------------------------------------------------------------"
        exit 0
    fi
fi
#========================================================================================================
# Checks to see if it is a new database or force overwrite then do not populate the data within tables
#========================================================================================================
if [[ ($newdb == "yes") || ($force == "yes") ]]; then
    if [ $exclude_data == "yes" ]; then
        populate="no"
    fi
fi

#=======================================
# Check if postgresql exists already
#=======================================

string1="$now1 ($current_user) [Info    ] DB Names:"

psql -l 2>>/dev/null $logfile

if [ $? -ne 127 ]; then
db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
\set ON_ERROR_STOP true
select string_agg(datname,' | ') from pg_database;
EOF`
    #LogMsg "[Info    ] DB Names: $db_query"
    echo "$string1 $db_query" | sed "s/.\{80\}|/&\n$string1 /g" >> $logfile
    echo "[Info    ] PostgreSQL is installed"
    LogMsg "[Info    ] PostgreSQL is installed"
    #LogMsg "---------------------------------------------------------------------------------------"
else
    echo "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    LogMsg "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    echo "-----------------------------------------------------------------------------------------------------------------"
    LogMsg "---------------------------------------------------------------------------------------"
    exit 1
fi

#======================================
# Check if database exists already
#======================================
db_exists="no"
psql -lqt | cut -d \| -f 1 | grep -qw $dbname
if [ $? -eq 0 ]; then
    db_exists="yes"
fi

#==================================================================
# End it if the input argument requires an existing database
#==================================================================
if [ $db_exists == "no" ]; then
    #==========================
    # Database does not exist
    #==========================
    LogMsg "[Info    ] $dbname database does not exist."
    if [[ ($removedata == "yes") || ($dropdb == "yes") || ($force == "yes") || ($eliminate == "yes") ]]; then
            echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
            echo "-----------------------------------------------------------------------------------------------------------------"
            LogMsg "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
            LogMsg "[End     ] Database does not exist"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            #LogMsg "---------------------------------------------------------------------------------------"
            exit 0
    fi
else
    #==========================
    # Database exists
    #==========================
    if [[ ($newdb == "yes") || ($createdb == "yes") ]]; then
            LogMsg "[Error   ] Database already exists for: $dbname"
            echo "[Error   ] Cannot perform action because the $dbname database already exists. Exiting."
            echo "-----------------------------------------------------------------------------------------------------------------"
            LogMsg "[Error   ] Cannot perform action because the $dbname database already exists. Exiting."
            LogMsg "[End     ] Database already exists."
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            #LogMsg "---------------------------------------------------------------------------------------"
            exit 0
    fi
fi

#=================================
# Drop the entire database
#=================================
# return code added to ensure it was successful or failed during this step
#---------------------------------------------------------------------------------------------------
return_code=0
if [ $dropdb == "yes" ]; then
    echo "[Warning ] This will drop $dbname database including all tables and data. Do you want to continue [y/n]?"
    LogMsg "[Info    ] $dbname database drop process begin."
    read -s -n 1 drop_database
    case "$drop_database" in
        [yY][eE][sS]|[yY])
        echo "[Info    ] User response: $drop_database"
        LogMsg "[Info    ] User response: $drop_database"
            #===============================
            # Checks the current connections
            #===============================
            connections_count=`psql -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname';"`
            if [ $connections_count -gt 0 ]; then
                LogMsg "[Error   ] $dbname will not be dropped because of existing connection(s) to the database."
                psql -t -A -U $db_username -c "select to_char(now(),'YYYY-MM-DD HH12:MI:SS') || ' ($current_user) [Info    ] Current Connection | User: ' || usename || ' ',
                ' Datebase: ' || datname, ' Connection(s): ' || count(*), ' Duration: ' || now() - backend_start as Duration
                from pg_stat_activity WHERE datname='$dbname' group by usename, datname, Duration order by datname, Duration desc;" &>> $logfile
                connections=`psql -t -U $db_username -c "select distinct usename from pg_stat_activity WHERE datname='$dbname';"`
                if [ ${#connections[@]} -gt 0 ]; then
                    echo "[Error   ] $dbname will not be dropped because of existing connection(s) to the database."
                    #==============================================
                    # Loops through in case of multiple connections
                    #==============================================
                    for i in ${connections[@]}; do
                        connection_count=`psql -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname' and usename='$i';"`
                        shopt -s extglob
                        connection_count="${connection_count##*( )}"
                        connection_count="${connection_count%%*( )}"
                        echo "[Error   ] User: $i has $connection_count connection(s)"
                    done
                    echo "[Info    ] See log file for connection details"
                    echo "-----------------------------------------------------------------------------------------------------------------"
                fi
            else
                #==================================================
                # This drops the DB if no connections are present
                #==================================================
                psql -q -U $db_username -c "drop database $dbname" &>> $logfile
                if [ $? -eq 0  ]; then
                    echo "[Complete] $dbname database deleted"
                    LogMsg "[Complete] $dbname database deleted"
                    echo "-----------------------------------------------------------------------------------------------------------------"
                else
                    echo "[Error   ] Database delete failed for $dbname"
                    LogMsg "[Error   ] Database delete failed for $dbname"
                    echo "-----------------------------------------------------------------------------------------------------------------"
                    return_code=1
                fi
            fi
        ;;
        *)
            echo "[Info    ] User response: $drop_database"
            LogMsg "[Info    ] User response: $drop_database"
            echo "[Error   ] Database not deleted"
            LogMsg "[Error   ] Database not deleted"
            echo "-----------------------------------------------------------------------------------------------------------------"
        ;;
    esac
    LogMsg "[End     ] $dbname database drop process end."
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    #LogMsg "---------------------------------------------------------------------------------------"
    exit $return_code
fi

#=====================================================================================================
# Drop csm db tables (Used if in another DB such as XCat. This will only drop CSM related tables)
#=====================================================================================================
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0
if [ $eliminate == "yes" ]; then
    LogMsg "[Info    ] $dbname database drop csm tables/functions/triggers process begin."
    #=====================
    # Drop DB table
    #=====================
    psql -q -U $csmdb_user -d $dbname -f $drop_tables_file &>> $logfile
    if [ $? -eq 0  ]; then
        echo "[Complete] $dbname csm tables and triggers dropped"
        LogMsg "[Complete] $dbname csm tables and triggers dropped"
        #=======================
        # Drop functions
        #======================
        psql -q -U $csmdb_user -d $dbname -f $drop_functions_file &>> $logfile
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname csm functions dropped"
            LogMsg "[Complete] $dbname csm functions dropped"
        else
            echo "[Error   ] Csm functions drop failed for $dbname"
            LogMsg "[Error   ] Csm functions drop failed for $dbname"
            echo "-----------------------------------------------------------------------------------------------------------------"
            return_code=1
        fi
    else
        echo "[Error   ] Csm table and trigger drop failed for $dbname"
        LogMsg "[Error   ] Csm table and trigger drop failed for $dbname"
        echo "-----------------------------------------------------------------------------------------------------------------"
        return_code=1
    fi
    if [ $return_code -ne 0 ]; then
        echo "[Error   ] dropping CSM tables"
        LogMsg "[Error   ] dropping CSM tables"
        echo "-----------------------------------------------------------------------------------------------------------------"
    fi
    LogMsg "[End     ] $dbname database drop csm tables/functions/triggers process end."
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    #LogMsg "---------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------"
    exit $return_code
fi
#===============================================
# Remove all data from the database tables
#===============================================
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0
if [ $removedata == "yes" ]; then
    LogMsg "[Info    ] $dbname database remove all data from the database tables begin."
    #=====================================
    # Removes all data from tables except
    # csm_db_schema_version table
    #=====================================
    psql -q -U $csmdb_user -d $dbname -f $delete_data_file 2>>/dev/null
    if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database data deleted from all tables excluding csm_db_schema_version and csm_db_schema_version_history tables"
        LogMsg "[Complete] $dbname database data deleted from all tables excluding csm_db_schema_version and csm_db_schema_history tables"
        echo "-----------------------------------------------------------------------------------------------------------------"
    else
        echo "[Error   ] Database data delete from all tables failed for $dbname"
        LogMsg "[Error   ] Database data delete from all tables failed for $dbname"
        echo "-----------------------------------------------------------------------------------------------------------------"
        return_code=1
    fi
    if [ $return_code -ne 0 ]; then
        echo "[Error   ] Error removing database data"
        LogMsg "[Error   ] Error removing database data"
        echo "-----------------------------------------------------------------------------------------------------------------"
    fi
    LogMsg "[End     ] $dbname database remove all data from the database tables end."
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    #LogMsg "---------------------------------------------------------------------------------------"
    exit $return_code
fi

#=============================
# Create a new database 
#=============================
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0
if [[ ($newdb == "yes") || ($createdb == "yes") ]]; then
    LogMsg "[Info    ] $dbname database creation process begin."
    #=============================================
    # Checks to see if the DB user alread exists
    # and if not then create the user.
    #=============================================
    if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
        echo "[Info    ] $dbname database user: $csmdb_user already exists"
        LogMsg "[Info    ] $dbname database user: $csmdb_user already exists"
    else
        create_db_user
        echo "[Complete] database created user: $csmdb_user"
    fi
    #=======================================================
    # Create the DB with ownership
    # First connection is with su postgres
    # There after each process is created by the created
    # DB user with privileges.
    #=======================================================
    psql -q -U $db_username -c "create database $dbname OWNER $csmdb_user" 2>/dev/null #&>> $logfile
    if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database created."
        LogMsg "[Complete] $dbname database created."
        #=========================
        # Create tables,views,etc
        #=========================
        psql -q -U $csmdb_user -d $dbname -f $create_tables_file &>/dev/null
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database tables created."
            LogMsg "[Complete] $dbname database tables created."
            #===============================
            # Create triggers and functions
            #===============================
            psql -q -U $csmdb_user -d $dbname -f $create_triggers_file &>> $logfile
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname database functions and triggers created."
                LogMsg "[Complete] $dbname database functions and triggers created."
                #=======================================================================================================
                # Loops through the csv files that are currently available and places data into the specified db_tables
                # loop through the array to get the table name and the related csv file
                # ${!table_data[@]} gets the element key i.e table1, table2, table3...
                # ${table_data[$i]} get the value i.e. table1.csv, table2.csv, table3.csv...
                #=======================================================================================================
                if [[ $populate == "yes" ]]; then
                    # This sets the counter for the loop
                    i=0
                    for i in "${!table_data[@]}"
                    do
                        table_name=$i
                        table_csv=${table_data[$i]}
                        #==============================
                        # Load prepopulated table data
                        #==============================
                        psql -q -U $csmdb_user -d $dbname -c "\copy $table_name from '$cur_path/$table_csv' with csv header;" &>> $logfile
                        if [ $? -eq 0  ]; then
                            echo "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                            LogMsg "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                        else
                            echo "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            LogMsg "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            echo "-----------------------------------------------------------------------------------------------------------------"
                            return_code=1
                        fi
                        ((i=i+1))
                    done
                    #===============================================
                    # Initializes the DB schema version
                    # (Copies from csv file - This file is modified
                    # when DB changes have taken place).
                    #===============================================
                    schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                    shopt -s extglob
                    schema_version="${schema_version##*( )}"
                    schema_version="${schema_version%%*( )}"
                    echo "[Info    ] $dbname DB schema version ($schema_version)"
                    LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                    echo "-----------------------------------------------------------------------------------------------------------------"
                else
                    echo "[Info    ] $dbname skipping data load process"
                    LogMsg "[Info    ] $dbname skipping data load process"
                    #=========================================
                    # Create DB schema version from csv file
                    #=========================================
                    psql -q -U $csmdb_user -d $dbname -c "\copy $csm_db_schema_version_table from '$cur_path/$csm_db_schema_version_data' with csv header;" &>> $logfile
                    if [ $? -eq 0  ]; then
                        echo "[Complete] $dbname initialized $csm_db_schema_version_table data"
                        LogMsg "[Complete] $dbname initialized $csm_db_schema_version_table data"
                        schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                        shopt -s extglob
                        schema_version="${schema_version##*( )}"
                        schema_version="${schema_version%%*( )}"
                        #=====================================================
                        # Displays to screen + log message the schema version
                        #=====================================================
                        echo "[Info    ] $dbname DB schema version ($schema_version)"
                        LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                        echo "-----------------------------------------------------------------------------------------------------------------"
                    else
                        echo "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                        LogMsg "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                        echo "-----------------------------------------------------------------------------------------------------------------"
                        return_code=1
                    fi
                fi
            else
                echo "[Error   ] Database functions and triggers creates failed for $dbname"
                LogMsg "[Error   ] Database functions and triggers creates failed for $dbname"
                echo "-----------------------------------------------------------------------------------------------------------------"
                return_code=1
            fi
        else
            echo "[Error   ] Database table creates failed for $dbname"
            LogMsg "[Error   ] Database table creates failed for $dbname"
            echo "-----------------------------------------------------------------------------------------------------------------"
            return_code=1
        fi
        if [ $return_code -ne 0 ]; then
            echo "[Error   ] creating database"
            LogMsg "[Error   ] creating database"
            echo "[Info    ] $dbname dropping database due to error"
            LogMsg "[Info    ] $dbname dropping database due to error"
            echo "-----------------------------------------------------------------------------------------------------------------"
            #====================================
            # Drop DB if the process has failed
            #====================================
            psql -q -U $csmdb_user -c "drop database $dbname" &>> $logfile
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname database deleted"
                LogMsg "[Complete] $dbname database deleted"
                echo "-----------------------------------------------------------------------------------------------------------------"
            else
                echo "[Error   ] Database delete failed for $dbname"
                LogMsg "[Error   ] Database delete failed for $dbname"
                echo "-----------------------------------------------------------------------------------------------------------------"
            fi
        fi
    else
        echo "[Error   ] Database create failed for $dbname"
        LogMsg "[Error   ] Database create failed for $dbname"
        echo "-----------------------------------------------------------------------------------------------------------------"
        return_code=1
    fi
    LogMsg "[End     ] $dbname database creation process end."
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    #LogMsg "---------------------------------------------------------------------------------------"
    exit $return_code
fi

#===================================================================================================
# Drop existing database tables, functions and triggers then recreate them
#===================================================================================================
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0
#=======================================
# If force option is selected then
# the script will execute this section
#=======================================
if [[ $force == "yes" ]]; then
    LogMsg "[Info    ] $dbname database force/creation process begin."
    #=============================================
    # Checks to see if the DB user alread exists
    # and if not then create the user.
    #=============================================
    if psql $csmdb_user -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
        echo "[Info    ] $dbname database user: $csmdb_user already exists"
        LogMsg "[Info    ] $dbname database user: $csmdb_user already exists"
    else
        create_db_user
        echo "[Complete] database created user: $csmdb_user"
    fi
    #=============================
    # Drop all DB tables
    #==============================
    psql -q -U $csmdb_user -d $dbname -f $drop_tables_file &>> $logfile
    if [ $? -eq 0  ]; then 
        echo "[Complete] $dbname database tables and triggers dropped"
        LogMsg "[Complete] $dbname database tables and triggers dropped"
        #=============================
        # Drop all DB functions
        #=============================
        psql -q -U $csmdb_user -d $dbname -f $drop_functions_file &>> $logfile
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname csm functions dropped"
            LogMsg "[Complete] $dbname csm functions dropped"
            #=============================
            # Create DB tables
            #=============================
            psql -q -U $csmdb_user -d $dbname -f $create_tables_file &>> $logfile
            if [ $? -eq 0  ]; then
                echo "[Complete] $dbname database tables recreated."
                LogMsg "[Complete] $dbname database tables recreated."
                #===================================
                # Create DB triggers and functions
                #===================================
                psql -q -U $csmdb_user -d $dbname -f $create_triggers_file &>> $logfile
                if [ $? -eq 0  ]; then
                    echo "[Complete] $dbname database functions and triggers recreated."
                    LogMsg "[Complete] $dbname database functions and triggers recreated."
                    #=======================================================================================================
                    # Loops through the csv files that are currently available and places data into the specified db_tables
                    # loop through the array to get the table name and the related csv file
                    # ${!table_data[@]} gets the element key i.e table1, table2, table3...
                    # ${table_data[$i]} get the value i.e. table1.csv, table2.csv, table3.csv...
                    #=======================================================================================================
                    if [ $populate == "yes" ]; then
                        #====================================
                        # This sets the counter for the loop
                        #====================================
                        i=0
                        for i in "${!table_data[@]}";
                        do
                            table_name=$i
                            table_csv=(${table_data[$i]})
                            #==============================
                            # Load prepopulated table data
                            #==============================
                            psql -q -U $csmdb_user -d $dbname -c "\copy $table_name from '$cur_path/$table_csv' with csv header;" &>> $logfile
                            if [ $? -eq 0  ]; then
                                echo "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                                LogMsg "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                            else
                                echo "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                                LogMsg "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                                echo "-----------------------------------------------------------------------------------------------------------------"
                                return_code=1
                            fi
                            ((i=i+1))
                        done
                        #===============================================
                        # Initializes the DB schema version
                        # (Copies from csv file - This file is modified
                        # when DB changes have taken place).
                        #===============================================
                        schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                        shopt -s extglob
                        schema_version="${schema_version##*( )}"
                        schema_version="${schema_version%%*( )}"
                        echo "[Info    ] $dbname DB schema version ($schema_version)"
                        LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                        echo "-----------------------------------------------------------------------------------------------------------------"
                    else
                        echo "[Info    ] $dbname skipping data load process."
                        LogMsg "[Info    ] $dbname skipping data load process."
                        #=========================================
                        # Create DB schema version from csv file
                        #=========================================
                        psql -q -U $csmdb_user -d $dbname -c "\copy $csm_db_schema_version_table from '$cur_path/$csm_db_schema_version_data' with csv header;" &>> $logfile
                        if [ $? -eq 0  ]; then
                            echo "[Complete] $dbname table data loaded successfully into csm_db_schema_version"
                            LogMsg "[Complete] $dbname table data loaded successfully into csm_db_schema_version"
                            schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                            shopt -s extglob
                            schema_version="${schema_version##*( )}"
                            schema_version="${schema_version%%*( )}"
                            #=====================================================
                            # Displays to screen + log message the schema version
                            #=====================================================
                            echo "[Info    ] $dbname DB schema version ($schema_version)"
                            LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                            echo "-----------------------------------------------------------------------------------------------------------------"
                        else
                            echo "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                            LogMsg "[Error   ] Table schema initialization failed for $dbname $csm_db_schema_version_table"
                            echo "-----------------------------------------------------------------------------------------------------------------"
                            return_code=1
                        fi
                    fi
                else
                    echo "[Error   ] Database functions and triggers creates failed for $dbname"
                    LogMsg "[Error   ] Database functions and triggers creates failed for $dbname"
                    echo "-----------------------------------------------------------------------------------------------------------------"
                    return_code=1
                fi
            else
                echo "[Error   ] Database table creates failed for $dbname"
                LogMsg "[Error    ]Database table creates failed for $dbname"
                echo "-----------------------------------------------------------------------------------------------------------------"
                return_code=1
            fi
        else
            echo "[Error   ] Csm functions and triggers drop failed for $dbname"
            LogMsg "[Error    ]Csm functions and triggers drop failed for $dbname"
            echo "-----------------------------------------------------------------------------------------------------------------"
            return_code=1
        fi
    else
        echo "[Error   ] Table drop failed for $dbname"
        LogMsg "[Error    ]Table drop failed for $dbname"
        echo "-----------------------------------------------------------------------------------------------------------------"
        return_code=1
    fi
    if [ $return_code -ne 0 ]; then
        echo "[Error   ] creating database"
        LogMsg "[Error    ] creating database"
        echo "-----------------------------------------------------------------------------------------------------------------"
    fi
    LogMsg "[End     ] $dbname database force/creation process end."
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    #LogMsg "---------------------------------------------------------------------------------------"
    exit $return_code
fi

#===================================================================================================
# Default action create a new database
#===================================================================================================
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0
LogMsg "[Info    ] $dbname default database creation process begin."
#=============================================================================
# Checks to see if the DB user alread exists
# and if not then create the user.
#=============================================================================
if psql $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw $csmdb_user; then
    echo "[Info    ] $dbname database user: $csmdb_user already exists"
    LogMsg "[Info    ] $dbname database user: $csmdb_user already exists"
else
    create_db_user
    echo "[Complete] database created user: $csmdb_user"
fi
#=======================================================
# Create the DB with ownership
# First connection is with su postgres
# There after each process is created by the created
# DB user with privileges.
#=======================================================
psql -q -U $db_username -c "create database $dbname OWNER $csmdb_user" 2>/dev/null #&>> $logfile
#psql -q -U $db_username -c "create database $dbname"
if [ $? -eq 0  ]; then
    echo "[Complete] $dbname database created."
    LogMsg "[Complete] $dbname database created."
    #=========================
    # Create tables,views,etc
    #=========================
    psql -q -U $csmdb_user -d $dbname -f $create_tables_file &>> $logfile
    if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database tables created"
        LogMsg "[Complete] $dbname database tables created"
        #===================================
        # Create DB triggers and functions
        #===================================
        psql -q -U $csmdb_user -d $dbname -f $create_triggers_file &>> $logfile
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database functions and triggers recreated."
            LogMsg "[Complete] $dbname database functions and triggers recreated."
                    #=======================================================================================================
                    # Loops through the csv files that are currently available and places data into the specified db_tables
                    # loop through the array to get the table name and the related csv file
                    # ${!table_data[@]} gets the element key i.e table1, table2, table3...
                    # ${table_data[$i]} get the value i.e. table1.csv, table2.csv, table3.csv...
                    #=======================================================================================================
                if [[ $populate == "yes" ]]; then
                    #====================================
                    # This sets the counter for the loop
                    #====================================
                    i=0
                    for i in "${!table_data[@]}"
                    do
                        table_name=$i
                        table_csv=${table_data[$i]}
                        #==============================
                        # Load prepopulated table data
                        #==============================
                        psql -q -U $csmdb_user -d $dbname -c "\copy $table_name from '$cur_path/$table_csv' with csv header;" &>> $logfile
                        if [ $? -eq 0  ]; then
                            echo "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                            LogMsg "[Complete] $dbname table data loaded successfully into ${table_name[$i]}"
                        else
                            echo "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            LogMsg "[Error   ] Table data load failed for $dbname ${table_name[$i]}"
                            echo "-----------------------------------------------------------------------------------------------------------------"
                            return_code=1
                        fi
                        ((i=i+1))
                    done
                    #===============================================
                    # Initializes the DB schema version
                    # (Copies from csv file - This file is modified
                    # when DB changes have taken place).
                    #===============================================
                    schema_version=`psql -t -U $csmdb_user -d $dbname -c "select version from csm_db_schema_version;"`
                    shopt -s extglob
                    schema_version="${schema_version##*( )}"
                    schema_version="${schema_version%%*( )}"
                    echo "[Info    ] $dbname DB schema version ($schema_version)"
                    LogMsg "[Info    ] $dbname DB schema version ($schema_version)"
                    echo "-----------------------------------------------------------------------------------------------------------------"
                else
                    echo "[Info    ] $dbname skipping data load process"
                    LogMsg "[Info    ] $dbname skipping data load process"
                    #=========================================
                    # Create DB schema version from csv file
                    #=========================================
                    psql -q -U $csmdb_user -d $dbname -c "\copy $csm_db_schema_version_table from '$cur_path/$csm_db_schema_version_data' with csv header;" &>> $logfile
                    if [ $? -eq 0  ]; then
                        echo "[Complete] $dbname table data initialized ${table_name[$i]}"
                        LogMsg "[Complete] $dbname table data initialized ${table_name[$i]}"
                    else
                        echo "[Error   ] Table data initialization failed for $dbname ${table_name[$i]}"
                        LogMsg "[Error   ] Table data initialization failed for $dbname ${table_name[$i]}"
                        echo "-----------------------------------------------------------------------------------------------------------------"
                        return_code=1
                    fi
                fi
            else
            echo "[Error   ] Database functions and triggers creates failed for $dbname"
            LogMsg "[Error   ] Database functions and triggers creates failed for $dbname"
            echo "-----------------------------------------------------------------------------------------------------------------"
            return_code=1
        fi
    else
        echo "[Error   ] Database table creates failed for $dbname"
        LogMsg "[Error    ] Database table creates failed for $dbname"
        echo "-----------------------------------------------------------------------------------------------------------------"
        return_code=1
    fi
    if [ $return_code -ne 0 ]; then
        echo "[Error   ] creating database"
        LogMsg "[Error   ] Error creating database"
        echo "[Info    ] $dbname dropping database due to error"
        LogMsg "[Info    ] $dbname dropping database due to error"
        echo "-----------------------------------------------------------------------------------------------------------------"
        #====================================
        # Drop DB if the process has faileld
        #====================================
        psql -q -U $csmdb_user -c "drop database $dbname" &>> $logfile
        if [ $? -eq 0  ]; then
            echo "[Complete] $dbname database deleted"
            LogMsg "[Complete] $dbname database deleted"
        else
            echo "[Error   ] Database delete failed for $dbname"
            LogMsg "[Error   ] Database delete failed for $dbname"
            echo "-----------------------------------------------------------------------------------------------------------------"
        fi
    fi
else
    echo "[Error   ] Database create failed for $dbname"
    LogMsg "[Error   ] Database create failed for $dbname"
    echo "-----------------------------------------------------------------------------------------------------------------"
    return_code=1
fi
LogMsg "[End     ] $dbname database default/creation process end."
#LogMsg "---------------------------------------------------------------------------------------"
echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
exit $return_code
