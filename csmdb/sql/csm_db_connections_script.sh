#!/bin//bash
#--------------------------------------------------------------------------------
#
#    csm_db_connections_script.sh
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
#--------------------------------------------------------------------------------

#--------------------------------------------------------------------------------
#   usage:              ./csm_db_connections_script.sh  <----- to kill db sessions
#   current_version:    1.8
#   create:             09-08-2017
#   last modified:      12-05-2018
#   Comments:           Execute this script as "postgres" user
#                       (user who runs postmaster)
#--------------------------------------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'

#------------------------------------------------
# INITIALIZE ENVIRONMENT
# Set up the environmental variables
#------------------------------------------------

OPTERR=0
DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
logname="csm_db_connections_script.log"
cd "${BASH_SOURCE%/*}" || exit
#cur_path=`pwd`

line1_out="------------------------------------------------------------------------------------------------------------------------"
line2_log="----------------------------------------------------------------------------------------------------"
line3_log="---------------------------------------------------------------------------------------------------------------------------"
line4_out="------------------------------------------------------------------------------------------------------------------------"

#------------------------------------------------
# Current user connected
#------------------------------------------------

current_user=`id -u -n`
db_username="postgres"
dbname="postgres"
now1=$(date '+%Y-%m-%d %H:%M:%S')

KILL="kill -TERM"
BASENAME=`basename "$0"`

#------------------------------------------------
# Log Message
#------------------------------------------------
# This checks the existence of the default log
# directory. If the default doesn't exist it will
# write the log files to /tmp directory.
# The current version will only display results
# to the screen.
#------------------------------------------------

#------------------------------------------------
# This checks the existence of the default 
# log directory.  If the default doesn't exist
# it will write the log files to /tmp directory
#------------------------------------------------

if [ -w "$logpath" ]; then
    logdir="$logpath"
else
    logdir="/tmp"
fi
logfile="${logdir}/${logname}"

#------------------------------------------------
# Log Message function
#------------------------------------------------

function LogMsg () {
now=$(date '+%Y-%m-%d %H:%M:%S')
echo "$now ($current_user) $1" >> $logfile
}

#------------------------------------------------
# Log messaging intro. header
#------------------------------------------------

echo "${line1_out}"
echo "[Start ] Welcome to CSM datatbase connections script."
echo "${line1_out}"
echo "[Info  ] $logfile"
LogMsg "[Start ] Welcome to CSM datatbase connections script."
LogMsg "${line2_log}"

#------------------------------------------------
# Long options to short along with fixed length
#------------------------------------------------

reset=true
for arg in "$@"
do
    if [ -n "$reset" ]; then
      unset reset
      set --      # this resets the "$@" array
    fi
    case "$arg" in
        --list)                     set -- "$@" -l ;;
        -list)                      usage && exit 0 ;;
        --kill)                     set -- "$@" -k ;;
        -kill)                      usage && exit 0 ;;
        --force)                    set -- "$@" -f ;;
        -force)                     usage && exit 0 ;;
        --user)                     set -- "$@" -u ;;
        -user)                      usage && exit 0 ;;
        --pid)                      set -- "$@" -p ;;
        -pid)                       usage && exit 0 ;;
        --help)                     set -- "$@" -h ;;
        -help)                      usage && exit 0 ;;
        -l|-k|-f|-u|-p|-h)          set -- "$@" "$arg" ;;
        -*)                         usage 2>>/dev/null &&
                                    exit 0 ;;
       # pass through anything else
       *)                           set -- "$@" "$arg" ;;
    esac
done

#------------------------------------------------
# Now we can drop into the short getopts
#------------------------------------------------
# Also checks the existence of the user and pid
# If neither of these are available then an
# error message will prompt and will be logged.
#------------------------------------------------

while [ "$#" -gt 0 ]
do
    case "$1" in
        -h|-\?)
                usage=t
                break
                ;;
        -l)
                OPT="list"
                ;;
        -k)
                OPT="kill"
                ;;
        -f)
                force=t
                ;;
        -u)
                if [ -z "$2" ]; then
                        echo "[Error ] Please specify user name"
                        LogMsg "[Error ] Please specify user name"
                        echo "${line1_out}"
                        echo "${line3_log}" >> $logfile
                        exit 1
                else
                        user="$2"
                        if psql -t -U $db_username -d $dbname -c "select usename FROM pg_stat_activity GROUP BY usename" | cut -d \| -f 1 | grep -qw $user; then
                            echo "[Info  ] DB user: $user is connected"
                            LogMsg "[Info  ] DB user: $user is connected"
                        else
                            echo "[Error ] DB user: $user is not connected or is invalid"
                            echo "${line1_out}"
                            LogMsg "[Error ] DB user: $user is not connected or is invalid"
                            LogMsg "${line2_log}"
                            LogMsg "[End   ] Postgres DB kill (-k, --kill and or -u, --user) query executed"
                            echo "${line3_log}" >> $logfile
                        exit 1
                        fi
                fi
                shift;;
        -p)
                if [ -z $2 ]; then
                        echo "[Error ] Please specify pid"
                        echo "${line1_out}"
                        echo "${line3_log}" >> $logfile
                        exit 1
                else
                        pid="$2"
                        if psql -t -U $db_username -d $dbname -c "select pid FROM pg_stat_activity GROUP BY pid" | cut -d \| -f 1 | grep -qw $pid; then
                            echo "[Info  ] DB PID: $pid is connected"
                            LogMsg "[Info  ] Script execution: $BASENAME -p, --pid"
                            LogMsg "[Info  ] DB PID: $pid is connected"
                        else
                            echo "[Error ] DB PID: $pid is not connected or is invalid"
                            echo "${line1_out}"
                            LogMsg "[Error ] DB PID: $pid is not connected or is invalid"
                            LogMsg "${line2_log}"
                            LogMsg "[End   ] Postgres DB kill (-k, --kill and or -u, --user) query executed"
                            echo "${line3_log}" >> $logfile
                        exit 1
                        fi
                fi
                shift;;
         *)
                if [ "$#" -eq "0" ]; then
                        echo "$BASENAME: invalid option: $2" 1>&2
                        echo "Try '$BASENAME --help' for more information." 1>&2
                        LogMsg "[Info  ] Script execution: $BASENAME -h, --help"
                        LogMsg "[Info  ] Wrong arguments were passed in (Please choose appropriate option from usage list -h, --help)"
                        LogMsg "${line2_log}"
                        LogMsg "[End   ] Help menu query executed"
                        echo "${line3_log}" >> $logfile
                        exit 1
                fi
                ;;
    esac
    shift;
done

function usage () {
#if [ "$usage" ]; then
        echo "------------------------------------------------------------------------------------------------------------------------"
        echo "[Info ] $BASENAME : List/Kill database user sessions"
        #echo "-----------------------------------------------------------------------------------------------------------------------"
        echo "[Usage] $BASENAME : [OPTION]... [USER]"
        echo "------------------------------------------------------------------------------------------------------------------------"
        echo "[Options]"
        echo "----------------|-------------------------------------------------------------------------------------------------------"
        echo "  Argument      | Description"
        echo "----------------|-------------------------------------------------------------------------------------------------------"
        echo "   -l, --list   | list database sessions"
        echo "   -k, --kill   | kill/terminate database sessions"
        echo "   -f, --force  | force kill (do not ask for confirmation,"
        echo "                | use in conjunction with -k option)"
        echo "   -u, --user   | specify database user name"
        echo "   -p, --pid    | specify database user process id (pid)"
        echo "   -h, --help   | help menu"
        echo "----------------|-------------------------------------------------------------------------------------------------------"
        echo "[Examples]"
        echo "------------------------------------------------------------------------------------------------------------------------"
        echo "   $BASENAME -l, --list                       | list all session(s)"
        echo "   $BASENAME -l, --list -u, --user [USERNAME] | list user session(s) "
        echo "   $BASENAME -k, --kill                       | kill all session(s)"
        echo "   $BASENAME -k, --kill -f, --force           | force kill all session(s)"
        echo "   $BASENAME -k, --kill -u, --user [USERNAME] | kill user session(s)"
        echo "   $BASENAME -k, --kill -p, --pid  [PIDNUMBER]| kill user session with a specific pid"
        echo "------------------------------------------------------------------------------------------------------------------------"
        #exit 0
#fi
}

#--------------------------------------------------
# Check if postgresql exists already and root user
#--------------------------------------------------
string1="$now1 ($current_user) [Info  ] DB Users:"
    psql -U $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw root
        if [ $? -ne 0 ]; then
            db_user_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(usename,' | ') from pg_user;
EOF`
            echo "$string1 $db_user_query" | sed "s/.\{60\}|/&\n$string1 /g" >> $logfile
            echo "[Error ] Postgresql may not be configured correctly. Please check configuration settings."
            LogMsg "[Error ] Postgresql may not be configured correctly. Please check configuration settings."
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
            exit 0
        fi

#------------------------------------------------
# Check if postgresql exists already and DB name
#------------------------------------------------
string2="$now1 ($current_user) [Info  ] DB Names:"
    psql -lqt | cut -d \| -f 1 | grep -qw $dbname 2>>/dev/null
        if [ $? -eq 0 ]; then       #<------------This is the error return code
            db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(datname,' | ') from pg_database;
EOF`
            echo "$string2 $db_query" | sed "s/.\{60\}|/&\n$string2 /g" >> $logfile
            LogMsg "${line2_log}"
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

#------------------------------------------------
# Checks to see if no arguments are passed in
#------------------------------------------------

PSQLC="psql -U $db_username -d $dbname -c "
PSQLTC="psql -t -U $db_username $dbname -A -c "
ALLUSERS=`psql -t -U $db_user $dbname -A -c "select usename from pg_stat_activity"` 
USERLIST=`psql -t -U $db_user $dbname -A -c "select usename from pg_stat_activity where usename='$user'"`

if [ "$OPT" = "list" ]; then
        UCTR=`$PSQLTC "select count(*) from pg_stat_activity" `
        echo "${line4_out}"
        printf '%-1s %-20s %-20s %-18s\n'"" "[Info  ] "" Database Session" "  | (all_users)"":" " $UCTR"
        LogMsg "[Info  ] Database Session | (all_users): $UCTR"
        if [[ -z "$user" ]]; then
        echo "${line1_out}"
        LogMsg "${line2_log}"
        printf '%-1s %-1s %-1s %-8s %-14s %-18s %-10s %0s\n'"" "$now1" "($current_user)" "[Info  ]" "PID " "| Usename" "| Datname" "|  Count " "| Duration" >> $logfile
        LogMsg "${line2_log}"
        connectioninfo=()
        psql \
        -U $db_username \
        -c "select pid, usename, datname, COUNT(*) AS count, now() - backend_start as duration
        FROM pg_stat_activity GROUP BY pid, usename, datname, duration ORDER BY usename, datname ASC" \
        --single-transaction \
        --set AUTOCOMMIT=off \
        --set ON_ERROR_STOP=on \
        --no-align \
        -t \
        --field-separator ' ' \
        --quiet \
        -d $dbname | while read -a Record ; do
        
        pid=${Record[0]}
        usename=${Record[1]}
        datname=${Record[2]}
        count=${Record[3]}
        duration=${Record[@]:4}
        
        connectioninfo+=("[Info  ] $pid | $usename | $datname | $count | $duration")
        printf '%-1s %-1s %-1s %-8s %-14s %-18s %-10s %0s\n'"" "$now1" "($current_user)" "[Info  ]" "$pid " "| $usename" "| $datname" "|     $count" "| $duration" >> $logfile
        done
        LogMsg "${line2_log}"
        LogMsg "[Info  ] Script execution: $BASENAME -l, --list"
        LogMsg "${line2_log}"
        LogMsg "[End   ] Postgres DB all current user connections query executed"
        echo "${line3_log}" >> $logfile
        fi
        SQL="select pid as "PID", datname as "Database","
        SQL="$SQL usename as "User", now() - backend_start as connection_duration from pg_stat_activity"
        if [ ! -z "$user" ]; then
                SQL="$SQL where usename = '$user'"
                UCTR1=`$PSQLTC "select count(*) from pg_stat_activity where usename = '$user'"`
                printf '%-1s %-20s %-20s %-18s\n'"" "[Info  ] " "Session List" "| ($user)"":" " $UCTR1"
                LogMsg "[Info  ] Session List     | ($user): $UCTR1"
                LogMsg "${line2_log}"
                printf '%-1s %-1s %-1s %-8s %-14s %-18s %-10s %0s\n'"" "$now1" "($current_user)" "[Info  ]" "PID " "| Usename" "| Datname" "|  Count " "| Duration" >> $logfile
                LogMsg "${line2_log}"
                connectioninfo=()
                psql \
                -U $db_username \
                -c "select pid, usename, datname, COUNT(*) AS count, now() - backend_start as duration
                FROM pg_stat_activity where usename = '$user' GROUP BY pid, usename, datname, duration ORDER BY usename, datname ASC" \
                --single-transaction \
                --set AUTOCOMMIT=off \
                --set ON_ERROR_STOP=on \
                --no-align \
                -t \
                --field-separator ' ' \
                --quiet \
                -d $dbname | while read -a Record ; do
                
                pid=${Record[0]}
                usename=${Record[1]}
                datname=${Record[2]}
                count=${Record[3]}
                duration=${Record[@]:4}
                
                connectioninfo+=("[Info ] $pid | $usename | $datname | $count | $duration")
                printf '%-1s %-1s %-1s %-8s %-14s %-18s %-10s %0s\n'"" "$now1" "($current_user)" "[Info ]" "$pid " "| $usename" "| $datname" "|     $count" "| $duration" >> $logfile
                done
                LogMsg "${line2_log}"
                LogMsg "[Info  ] Script execution: $BASENAME -l, --list -u, --user"
                LogMsg "${line2_log}"
                LogMsg "[End   ] Postgres DB current users list for: $user connections query executed"
                echo "${line1_out}"
                echo "${line3_log}" >> $logfile
        fi
        $PSQLC "$SQL"
elif [ "$OPT" = "kill" ]; then
        SQL="select pid from pg_stat_activity"
        if [ "$2" == "$user" ] && [ "$2" == "$pid" ]; then
                LogMsg "[Info  ] Script execution: $BASENAME -k, --kill"
        fi
        if [ ! -z "$user" ]; then
                SQL="$SQL where usename = '$user'"
                LogMsg "[Info  ] Script execution: $BASENAME -k, --kill -u, --user"
        elif [ ! -z "$pid" ]; then
                SQL="$SQL where pid = '$pid'"
                LogMsg "[Info  ] Script execution: $BASENAME -k, --kill -p, --pid"
        fi
        for pid in `$PSQLTC "$SQL" `; do
                if [ "$force" ]; then
                        echo "[Info  ] Killing session (PID:$pid)"
                        $KILL $pid
                        LogMsg "[Info  ] Killing user session: $user (PID:$pid) $KILL $pid" #(need to test this option at some point)
                else
                        echo "[Info  ] Kill database session (PID:$pid) [y/n] ?:"
                        read -s -n 1 confirm
                        if [ "$confirm" = "y" ]; then
                                echo "[Info  ] User response: $confirm"
                                echo "[Info  ] Killing session (PID:$pid)"
                                $KILL $pid
                                if [ ! -z "$user" ]; then
                                    LogMsg "[Info  ] Killing session for user: $user (PID:$pid) User response: ****(YES)**** $KILL $pid"
                                else
                                    LogMsg "[Info  ] Killing session (PID:$pid) User response: ****(YES)**** $KILL $pid"
                                fi
                        else
                                if [ ! -z "$user" ]; then
                                    echo "[Info  ] User response: $confirm"
                                    echo "[Info  ] Not killing session (PID:$pid)"
                                    LogMsg "[Info  ] Killing session for user: $user (PID:$pid) User response: ****(NO)****  not killed" 
                                else
                                    echo "[Info  ] User response: $confirm"
                                    echo "[Info  ] Not killing session (PID:$pid)"
                                    LogMsg "[Info  ] Killing session (PID:$pid) User response: ****(NO)****  not killed"
                                fi
                        fi
                fi
        done
        LogMsg "${line2_log}"
        LogMsg "[End   ] Postgres DB kill query executed"
        echo "${line3_log}" >> $logfile
else
        usage
        if [[ $1 == "-h" ]]; then
            LogMsg "[Info  ] Script execution: $BASENAME -h, --help"
            LogMsg "${line2_log}"
            LogMsg "[End   ] Help menu query executed"
            echo "${line3_log}" >> $logfile
            exit 0
        else
            LogMsg "[Info  ] Script execution: $BASENAME $1 "
            LogMsg "[Info  ] Wrong arguments were passed in (Please choose appropriate option from usage list -h, --help)"
            LogMsg "${line2_log}"
            LogMsg "[End   ] Please choose another option"
            echo "${line3_log}" >> $logfile
            exit 1
        fi
fi
echo "${line4_out}"
