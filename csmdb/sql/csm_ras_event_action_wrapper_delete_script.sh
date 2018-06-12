#!/bin/bash
#================================================================================
#   
#    csm_ras_event_action_wrapper_delete_script.sh
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
#   usage:         Delete related history table data which has been archived
#   version:       1.0
#   create:        04-10-2017
#   last modified: 06-11-2017
#================================================================================

#----------------------------------------------------------------
# Traps any interrupted or terminated sessions
# (see finish function below)
#----------------------------------------------------------------

trap finish SIGINT
trap finish SIGTERM

#------------------------------------------------
# Postgres and other error handling options
#------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'
OPTERR=0

#----------------------------------------------------------------
# Defined variables
#----------------------------------------------------------------
# Command line variables passed in:
# 1. Database Name,
# 2. Minute interval value passed in for deleting history records
#----------------------------------------------------------------

DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
logname="csm_db_delete_script.log"
tmp_logname="$$_csm_db_delete_script.log"
cd "${BASH_SOURCE%/*}" || exit
dbname=$DEFAULT_DB
now=$(date '+%Y-%m-%d.%H.%M.%S')

#-------------------------------------------------------------------------------
# Current user connected
#-------------------------------------------------------------------------------

current_user=`id -u -n`
db_username="postgres"
now1=$(date '+%Y-%m-%d %H:%M:%S')
pid=$BASHPID

#-------------------------------------------------------------------------------
# Environment variables for delete counts
#-------------------------------------------------------------------------------

#------------------------------------------------------------------------------------
script_name="csm_ras_event_action_wrapper_delete_script.sh"
#echo "------------------------------------------------------------------------------"

#----------------------------------------------------------------
#  These are the variables for the avg processing
#----------------------------------------------------------------

count=0
total=0
average="0"

#----------------------------------------------------------------
# Below checks the arguments passed in on the command line
#----------------------------------------------------------------

    if [ "$#" -ne 4 ]; then
        echo "----------------------------------------------------------------------------------------------------------------------"
        echo "[Error  ] illegal # of import arguments"
        echo "[Info   ] Data_dir is where the archive files will be written"
        echo "[Example] [./csm_ras_event_action_wrapper_delete_script.sh] [dbname] [time_mins] [table_name] [data_dir]"
        echo "----------------------------------------------------------------------------------------------------------------------"
        exit 1
    fi

    dbname=$1
    interval_time=$2
    i=$interval_time
    table_name1=$3
    data_dir=$4
    cur_path=$data_dir
    logpath=$data_dir

#----------------------------------------------------------------
# Below makes the directory if it does not exist
# First checks if the "/" is specified at the end of the
# directory given path. If not then it is added
#----------------------------------------------------------------

    if [[ "${data_dir: -1}" != "/" ]]; then
        data_dir="${data_dir}/"
    fi

    if [[ ! -e $data_dir ]]; then
        mkdir -p $data_dir 2>>/dev/null
        if [ $? -ne 0 ]; then
            echo "------------------------------------------------------------------------------------------------------------------------"
            echo "[Error  ] make directory failed for: $data_dir"
            echo "[Info   ] mkdir: cannot create directory ‘$data_dir’: Permission denied"
            echo "[Info   ] please provide a valid writable directory"
            echo "------------------------------------------------------------------------------------------------------------------------" 
            exit 1
        else
            chown postgres:postgres $data_dir
            chmod 755 $data_dir
        fi
    elif [[ ! -d $data_dir ]]; then
        echo "------------------------------------------------------------------------------------------------------------------------"
        echo "$data_dir already exists but is not a directory" 1>&2
        echo "------------------------------------------------------------------------------------------------------------------------"
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
     logfile="${logdir}/${tmp_logname}"

#-------------------------------------------------------------------------------
# Log Message
#-------------------------------------------------------------------------------

     function LogMsg () {
     LogTime=$(date '+%Y-%m-%d %H:%M:%S')
     echo "$LogTime ($pid) ($current_user) ($table_name1.del ) $1" >> $logfile
     }

     LogMsg "[Start ] Welcome to CSM database:"
     LogMsg "---------------------------------------------------------------------------------------"

#-------------------------------------------------------------------------------
# Error Log Message
#-------------------------------------------------------------------------------

    function finish () {

    echo "-------------------------------------------------------------------------------------------------------------"
    LogMsg "---------------------------------------------------------------------------------------"
    echo   "[Info   ] Delete process for $table_name1 has been interrupted or terminated."
    echo   "[Info   ] Please see log file for more details"
    LogMsg "[Info  ] Delete process for $table_name1 has been interrupted or terminated."
    LogMsg "[Info  ] Please see log file for more details"
    LogMsg "[End   ] Exiting csm_history_wrapper_delete_script_template.sh."
    echo "-------------------------------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile

    cat ${data_dir}$tmp_logname >> ${data_dir}$logname
    wait
    rm -rf ${data_dir}$tmp_logname
    exit $?
}

#----------------------------------------------------------------
# Check if postgresql exists already
#----------------------------------------------------------------

string1="$now1 ($pid) ($current_user) ($table_name1.del ) [Info  ] DB Names:"
psql -l 2>>/dev/null $logfile

#if [ $? -eq 0 ]; then
if [ $? -ne 127 ]; then       #<------------This is the error return code
db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
\set ON_ERROR_STOP true
select string_agg(datname,' | ') from pg_database;
EOF`
    echo "$string1 $db_query" | sed "s/.\{40\}|/&\n$string1 /g" >> $logfile
    LogMsg "[Info  ] PostgreSQL is installed"
else
    echo "-----------------------------------------------------------------------------------------"
    echo "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    echo "-----------------------------------------------------------------------------------------"
    LogMsg "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    LogMsg "---------------------------------------------------------------------------------------"
    exit 1
fi

     LogMsg "[Info  ] $table_name1.wrapper_delete_script.sh"
     LogMsg "---------------------------------------------------------------------------------------"

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
    
        LogMsg "[Start ] Database does not exist."
        echo "-------------------------------------------------------------------------------------------------------------"
        echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
        echo "[Info    ] Please provide a valid DB that exists on the system."
        echo "-------------------------------------------------------------------------------------------------------------"
        LogMsg "[Error ] Cannot perform action because the $dbname database does not exist. Exiting."
        LogMsg "[End   ] Please provide a valid DB that exists on the system."
        echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
        exit 1
    fi

#-----------------------------------------------
# csm_table_name & csm_table_data_array
#-----------------------------------------------

declare -A avg_data
declare -A delete_array

#----------------------------------------------------------------
# This should be in the order of each of the child history scripts
#----------------------------------------------------------------

    table_name=()
    table_name+=(${3}                          )

#----------------------------------------------------------------
# All the raw combined timing results before trimming
#----------------------------------------------------------------

    all_results="$data_dir/${pid}_csm_db_ras_delete_results.$now.timings"
    delete_avg_results="delete_avg_results_$count_$now.csv"

#-------------------------------------------------------------------------------------------------------------------
# This is the script that deletes the history tables
#-------------------------------------------------------------------------------------------------------------------
    
    ./csm_ras_event_action_table_delete.sh $dbname $interval_time $table_name1 $data_dir 2>&1 >>"$all_results" | tee -a "$all_results" | \
    awk '/^ERROR:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d.%H:%M:%S') ($pid) ($current_user) ($table_name1.del ) [Error ] "'" $0 }' | tee -a >>"${logfile}" &

#-------------------------------------------------------------------------------------------------------------------
# Waits for the process to finish before calculating and trimming the results
#-------------------------------------------------------------------------------------------------------------------

    wait

#----------------------------------------------------------------
# Create the delete count array from external file
#----------------------------------------------------------------
# 1. create array
# 2. read a line in
# 3. create an index on table name and append line to the array
# 4. increment table name (index for each pass)
#----------------------------------------------------------------

z=0

getArray() {
    delete_array=()
    while IFS= read -r line
    do
       delete_array[${table_name[z]}]="$line"
       ((z++))
    done < "$1"
}

d=0
if [[ -f "${data_dir}${pid}_ras_tmp_delete_count.count" ]]; then
    line=$(head -n 1 "${data_dir}${pid}_ras_tmp_delete_count.count")
    if [ -z "$line" ]; then
        echo "-------------------------------------------------------------------------------------------------------------"
        echo "[Error: ] The directory ($data_dir$table_name1) was invalid."
        LogMsg "[Error ] The directory ($data_dir$table_name1) was invalid."
        echo "[Error: ] Or the table: $table_name1 is not a valid deletion table."
        LogMsg "[Error ] Or the table: $table_name1 is not a valid deletion table."
        echo "[Info:  ] Please check the log file: $data_dir$logname for detailed info."
        LogMsg "[Info  ] Please check the log file: $data_dir$logname for detailed info."
        LogMsg "[Info  ] Exiting CSM datatbase: ${table_name1}_wrapper_delete_script.sh"
        #echo "-------------------------------------------------------------------------------------------------------------"
        rm ${all_results}
        rm ${data_dir}${pid}_ras_tmp_delete_count.count
        finish
        exit 0
    else    
        for file in $( ls -1 $data_dir/${pid}_ras_tmp_delete_count.count)
        do
        getArray "$file"
        done
    fi
fi
#-------------------------------------------------------------------------------------------------------------------
# This calculates and trims the results to a csv file and sets array index for table names
#-------------------------------------------------------------------------------------------------------------------

    j=0
    for i in $(grep real $all_results | awk '{ print substr($2,3,5) }'); do
        total=$(echo $total+$i | bc | awk '{printf "%.3f\n", $0}')
#        echo "$i" >> "${data_dir}/${trim_timing_file}"
        avg_data[${table_name[j]}]="$i"
        ((count++))
        ((j++))
    done

    average=$(echo "scale=6; $total / $count" | bc | awk '{printf "%.3f\n", $0}')
#    echo $average > "${data_dir}/${delete_avg_results}"

#----------------------------------------------------------------
# Headers to be displayed for the loop below
#----------------------------------------------------------------

echo "------------------------------------------------------------------------------"
echo "  Table                        | Time        |  Delete Count                  "
echo "-------------------------------|-------------|--------------------------------"

#----------------------------------------------------------------
# This loop combines both table name and delete count 
#----------------------------------------------------------------

    for ((j=0; j<${#table_name[*]}; j++));
    do
        table=${table_name[j]}
        table_avg=${avg_data[$table]}
        delete_count=${delete_array[$table]}
        printf '%-0s %-29s %-13s %0s\n' "" "$table" "|  $table_avg" "|   $delete_count"
        LogMsg "[Info  ] Tbl name: $table | Tbl time: $table_avg | Del count: $delete_count"
    done

#----------------------------------------------------------------
# Displays the results to the screen 
#----------------------------------------------------------------

        echo "------------------------------------------------------------------------------"
        echo " Date/Time:                    |  $now"
        echo " DB Name:                      |  $dbname"
        echo " DB User:                      |  $current_user"
        echo " Interval time:                |  $interval_time Min(s)."
        echo " Total time:                   |  $total"
        echo " Average time:                 |  $average"
        echo "------------------------------------------------------------------------------"
       
        LogMsg "---------------------------------------------------------------------------------------"
        LogMsg "[Info  ] Total Time:   $total"
        LogMsg "[Info  ] Average Time: $average"
        LogMsg "---------------------------------------------------------------------------------------"
        LogMsg "[End   ] Complete CSM datatbase: ${table_name1}_wrapper_delete_script.sh"
#       LogMsg "---------------------------------------------------------------------------------------"
        echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile

#----------------------------------------------------------------
# This removes all .timing & .count files left over
#----------------------------------------------------------------

    rm ${all_results}
    rm ${data_dir}${pid}_ras_tmp_delete_count.count

#----------------------------------------------------------------

#----------------------------------------------------------------
# Temp file to master log file and clean up
#----------------------------------------------------------------

cat ${data_dir}$tmp_logname >> ${data_dir}$logname
wait
rm ${data_dir}$tmp_logname
