#!/bin/bash
#================================================================================
#   
#    csm_history_wrapper_archive_script_template.sh
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
#   usage:         Archive history related tables
#   version:       1.0
#   created:       04-10-2017
#   last modified: 03-15-2017
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
logname="csm_db_archive_script.log"
cd "${BASH_SOURCE%/*}" || exit
dbname=$DEFAULT_DB
now=$(date '+%Y-%m-%d.%H.%M.%S.%N')

script_name="csm_history_wrapper_archive_script_template.sh"
#echo "------------------------------------------------------------------------------"
#echo "[Script name:   ]  $script_name"

#-------------------------------------------------------------------------------
# Current user connected
#-------------------------------------------------------------------------------

current_user=`id -u -n`
db_username="postgres"
now1=$(date '+%Y-%m-%d %H:%M:%S')

#----------------------------------------------------------------
# These are the variables for the avg processing
#----------------------------------------------------------------

count=0
total=0
average="0"

#----------------------------------------------------------------
# Below checks the arguments passed in on the command line
#----------------------------------------------------------------

    if [ "$#" -ne 4 ]; then
        echo "------------------------------------------------------------------------------------------------------------------------"
        echo "[Error  ] illegal # of import arguments"
        echo "[Info   ] Data_dir is where the archive files will be written"
        echo "[Example] [./csm_history_wrapper_archive_script_template.sh] [dbname] [archive_counter] [history_table_name] [/data_dir/]"
        echo "------------------------------------------------------------------------------------------------------------------------"
        exit 1
    fi

    dbname=$1
    archive_counter=$2
    table_name1=$3
    data_dir=$4
    cur_path=$data_dir
    logpath=$data_dir #<----- This file will live in "/var/log/ibm/csm/db"

#----------------------------------------------------------------
# Below makes the directory if it does not exist
#----------------------------------------------------------------

    if [[ ! -e $data_dir ]]; then
        mkdir -p $data_dir
        if [ $? -ne 0 ]; then
            echo "make directory failed for: $data_dir"
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
#    if [ -d "$logpath" ]; then
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
     echo "$LogTime ($current_user) ($table_name1.arc ) $1" >> $logfile
     }

     LogMsg "[Start ] Welcome to CSM datatbase:"
     LogMsg "------------------------------------------------------------------------------------"

#----------------------------------------------------------------
# Check if postgresql exists already
#----------------------------------------------------------------

string1="$now1 ($current_user) ($table_name1.arc ) [Info  ] DB Names:"
psql -l 2>>/dev/null $logfile

#if [ $? -eq 0 ]; then
if [ $? -ne 127 ]; then       #<------------This is the error return code
db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
\set ON_ERROR_STOP true
select string_agg(datname,' | ') from pg_database;
EOF`
    echo "$string1 $db_query" | sed "s/.\{80\}|/&\n$string1 /g" >> $logfile
    LogMsg "[Info  ] PostgreSQL is installed"
#   LogMsg "---------------------------------------------------------------------------------------"
else
    echo "-----------------------------------------------------------------------------------------"
    echo "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    echo "-----------------------------------------------------------------------------------------"
    LogMsg "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    LogMsg "---------------------------------------------------------------------------------------"
    exit 1
fi

        LogMsg "[Info  ] csm_history_wrapper_archive_script_template.sh"
        LogMsg "------------------------------------------------------------------------------------"

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
        echo "[Info    ] Please provide a valid DB that exists on the system (hint: psql -l)."
        echo "-------------------------------------------------------------------------------------------------------------"
        LogMsg "[Error ] Cannot perform action because the $dbname database does not exist. Exiting."
        LogMsg "[End   ] Please provide a valid DB that exists on the system (hint: psql -l)."
        LogMsg "------------------------------------------------------------------------------------"
        exit 1
    fi

#-----------------------------------------------
# csm_table_name & csm_table_data_array
#-----------------------------------------------

declare -A avg_data

#----------------------------------------------------------------
# This should be in the order of each of the child history scripts
#----------------------------------------------------------------

    table_name=()
    table_name+=(${3}                  )

#----------------------------------------------------------------
# All the raw combined timing results before trimming
#----------------------------------------------------------------

    all_results="$data_dir/$table_name1_archive_results.$now.timings"

#----------------------------------------------------------------
# These are the individual history tables being archived
#----------------------------------------------------------------
    
    ./csm_history_table_archive_template.sh $dbname $archive_counter $table_name1 $data_dir 2>&1 >>"$all_results" | tee -a "$all_results" | \
        awk '/^ERROR:.*$/ { print "'"$(date '+%Y-%m-%d.%H:%M:%S') ($current_user) ($table_name1.arc ) [Error ] $table_name1: "'" $0 }' | tee -a >>"${logfile}"

#-------------------------------------------------------------------------------------------------------------------
# Waits for the process to finish before calculating and trimming the results
#-------------------------------------------------------------------------------------------------------------------

    wait        

#----------------------------------------------------------------
# Create the archive count array from external file
#----------------------------------------------------------------
# 1. create array
# 2. read a line in
# 3. create an index on table name and append line to the array
# 4. increment table name (index for each pass)
#----------------------------------------------------------------

z=0
d=0

declare -A archive_array

if [ -f "$data_dir/$table_name1.count" ]; then
    for file in $( ls -1 $data_dir/$table_name1.count*)
        do
            archive_array[${table_name[z]}]=$(cat $file)
            ((z++))
        done
else
    echo "-------------------------------------------------------------------------------------------------------------"
    echo "[Error: ] The directory ($data_dir$table_name1) was invalid."
    LogMsg "[Error ] The directory ($data_dir$table_name1) was invalid."
    echo "[Error: ] Or the table: $table_name1 is not a valid archiving table."
    LogMsg "[Error ] Or the table: $table_name1 is not a valid archiving table."
    echo "[Info:  ] Please check the log file: $data_dir$logname for detailed info."
    LogMsg "[Info  ] Please check the log file: $data_dir$logname for detailed info."
    LogMsg "[End   ] Exiting: $table_name1 archive process"
    echo "-------------------------------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    exit 0
fi

#-------------------------------------------------------------------------------------------------------------------
# This calculates and trims the results to a csv file
#-------------------------------------------------------------------------------------------------------------------

    j=0
    echo "------------------------------------------------------------------------------"

    for i in $(grep real $all_results | awk '{ print substr($2,3,5) }'); do
        total=$(echo $total+$i | bc | awk '{printf "%.3f\n", $0}')
#       echo "$i" >> "${data_dir}/${trim_timing_file}"
        avg_data[${table_name[j]}]="$i"
        ((count++))
        ((j++))
    done

    average=$(echo "scale=6; $total / $count" | bc | awk '{printf "%.3f\n", $0}')
#   echo $average > "${data_dir}/${avg_results}"

echo "  Table                        |  Time       |  Archive Count                 "          
echo "-------------------------------|-------------|--------------------------------"

LogMsg "------------------------------------------------------------------------------------"

for ((j=0; j<${#table_name[*]}; j++));
do
    table=${table_name[j]}
    table_avg=${avg_data[$table]}
    archive_count=${archive_array[$table]}
    printf '%-0s %-29s %-13s %0s\n' "" "$table" "|  $table_avg" "|   $archive_count"
    LogMsg "[Info  ] Tbl name: $table | Tbl time: $table_avg | Arc ct: $archive_count"
done

      echo "------------------------------------------------------------------------------"
      echo " Date/Time:                    |  $now"
      echo " DB Name:                      |  $dbname"
      echo " DB User:                      |  $current_user"
      echo " archive_counter:              |  $archive_counter"
      echo " Total time:                   |  $total"
      echo " Average time:                 |  $average"
      echo "------------------------------------------------------------------------------"

      LogMsg "------------------------------------------------------------------------------------"
      LogMsg "[Info  ] Total Time:   $total"
      LogMsg "[Info  ] Average Time: $average"
      LogMsg "------------------------------------------------------------------------------------"
      LogMsg "[End   ] Complete: $table_name1 archive process"
      echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile


#----------------------------------------------------------------
# This removes all .timing files left over
#----------------------------------------------------------------

    rm ${all_results}
    rm ${data_dir}$table_name1.count*
#----------------------------------------------------------------
