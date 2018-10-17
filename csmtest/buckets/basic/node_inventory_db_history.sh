# Basic Bucket for Node Inventory Collection DB History Table Testing

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find config file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exiting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/node_inventory_db_history.log
TEMP_LOG=${LOG_PATH}/buckets/basic/node_inventory_db_history_tmp.log
SBIN_PATH=/opt/ibm/csm/sbin

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exiting."
fi

# The list of the node inventory active tables, indexed by table name
declare -AA tables
tables["csm_node"]="csm_node"
tables["csm_dimm"]="csm_dimm"
tables["csm_gpu"]="csm_gpu"
tables["csm_hca"]="csm_hca"
tables["csm_ssd"]="csm_ssd"
tables["csm_processor_socket"]="csm_processor_socket"

# Create a mapping from active table name to associated history table name
declare -AA history_tables
history_tables["csm_node"]="csm_node_history"
history_tables["csm_dimm"]="csm_dimm_history"
history_tables["csm_gpu"]="csm_gpu_history"
history_tables["csm_hca"]="csm_hca_history"
history_tables["csm_ssd"]="csm_ssd_history"
history_tables["csm_processor_socket"]="csm_processor_socket_history"

# Normal inventory history table behavior is to create an entry in the history table 
# whenever a value in the active table changes.
# However, there are some columns that are exceptions to this rule.
# If any of these exceptions change value, correct behavior is to NOT create an entry in the history table.
# In the case of non-inventory fields in the csm_node table, they should create history entries, 
# but are not covered by these inventory tests.
declare -AA exception_columns
exception_columns["csm_node"]="node_name collection_time update_time state comment secondary_agg primary_agg" 
exception_columns["csm_node"]+=" feature_1 feature_2 feature_3 feature_4 hard_power_cap physical_u_location physical_frame_location"
exception_columns["csm_dimm"]="node_name serial_number"
exception_columns["csm_gpu"]="node_name gpu_id"
exception_columns["csm_hca"]="node_name"
exception_columns["csm_ssd"]="node_name update_time wear_lifespan_used wear_total_bytes_written wear_total_bytes_read wear_percent_spares_remaining"
exception_columns["csm_processor_socket"]="node_name serial_number"

# Uncommenting out any line below will remove checking of that table from the tests
#unset -v 'tables[csm_node]' 'history_tables[csm_node]' 'exception_columns[csm_node]' 
#unset -v 'tables[csm_dimm]' 'history_tables[csm_dimm]' 'exception_columns[csm_dimm]' 
#unset -v 'tables[csm_gpu]'  'history_tables[csm_gpu]' 'exception_columns[csm_gpu]' 
#unset -v 'tables[csm_hca]'  'history_tables[csm_hca]' 'exception_columns[csm_hca]' 
#unset -v 'tables[csm_ssd]'  'history_tables[csm_ssd]' 'exception_columns[csm_ssd]' 
#unset -v 'tables[csm_processor_socket]' 'history_tables[csm_processor_socket]' 'exception_columns[csm_processor_socket]' 

check_return_exit () {
        if [ $1 -ne $2 ]
        then
                echo "FAILED" >> ${LOG}
                exit 1
        else
                echo "PASS" >> ${LOG}
        fi
}

# Input = return code from command.  This will flag the test case as failed on a non-zero return code
check_return_flag () {
        if [ $1 -ne 0 ]
        then
                FLAGS+="\n$2"
                echo "FAILED" >> ${LOG}
        else
                echo "PASS" >> ${LOG}
        fi
}

# Input = set of strings expected in command output.  This will return 1 if any of the input strings are not found, 0 if they are all found
check_all_output () {
        for arg in "$@"
        do
                cat ${TEMP_LOG} | grep "${arg}"
                if [ $? -ne 0 ]
                then
                        return 1
                fi
        done
        return 0
}


# DEVEL - DELETE LOG if it exists
if [ -e ${LOG} ]
then
   # Delete pre-existing log
   rm -f ${LOG}
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "          Starting Node Inventory DB History Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Confirm that SINGLE_COMPUTE has been set
if [ -z ${SINGLE_COMPUTE+x} ]; then 
   echo "SINGLE_COMPUTE is not set, exitting." >> ${LOG}
   exit 1
else 
   echo "SINGLE_COMPUTE is set to '$SINGLE_COMPUTE'" >> ${LOG}
fi

echo "Configured DB tables for testing: ${tables[@]}" >> ${LOG}

echo "------------------------------------------------------------" >> ${LOG}

# Function to read a set of data from the DB before the test is run
# Returned data will be set in global arrays named:
# active_rows_before
# history_rows_before
# All arrays indexed by names in "tables" array 
declare -AA active_rows_before
declare -AA history_rows_before
read_db_before()
{
   # Generate a series of queries, one row expected per query
   local tbl
   local psql_input=""
   for tbl in "${tables[@]}"
   do
      # Get the count of rows in the active table
      psql_input+="select COUNT(*) from $tbl where node_name='$SINGLE_COMPUTE';"
      
      # Get the count of rows in the history table
      psql_input+="select COUNT(*) from ${history_tables[$tbl]} where node_name='$SINGLE_COMPUTE';"
   done

   # Execute all queries in a single call to psql, get the tuples only (-t flag)
   read -r -a psql_output <<< $(echo "$psql_input" | /bin/psql -At -U postgres csmdb)
   
   # Use for debugging the output, if necessary
   #for i in ${psql_output[@]} ; do
   #   echo "$i"
   #done

   # Copy the output queries into the appropriate variables
   local i=0
   for tbl in "${tables[@]}" ; do
      active_rows_before[$tbl]=${psql_output[$i]}
      (( i++ ))
      history_rows_before[$tbl]=${psql_output[$i]}
      (( i++ ))
   done
}

# Function to read a set of data from the DB after the test is run
# Returned data will be set in global arrays named:
# active_rows_after
# history_rows_after
# All arrays indexed by names in "tables" array 
declare -AA active_rows_after
declare -AA history_rows_after
read_db_after()
{
   # Generate a series of queries, one row expected per query
   local tbl
   local psql_input=""
   for tbl in "${tables[@]}"
   do
      # Get the count of rows in the active table
      psql_input+="select COUNT(*) from $tbl where node_name='$SINGLE_COMPUTE';"
      
      # Get the count of rows in the history table
      psql_input+="select COUNT(*) from ${history_tables[$tbl]} where node_name='$SINGLE_COMPUTE';"
   done

   # Execute all queries in a single call to psql, get the tuples only (-t flag)
   read -r -a psql_output <<< $(echo "$psql_input" | /bin/psql -At -U postgres csmdb)
   
   # Use for debugging the output, if necessary
   #for i in ${psql_output[@]} ; do
   #   echo "$i"
   #done

   # Copy the output queries into the appropriate variables
   local i=0
   for tbl in "${tables[@]}" ; do
      active_rows_after[$tbl]=${psql_output[$i]}
      (( i++ ))
      history_rows_after[$tbl]=${psql_output[$i]}
      (( i++ ))
   done
}

# Function to check if a particular column is "normal"
# "normal" columns should cause new history table entries to be created when they are updated
# Takes two parameters:
# The active table name and the column name to check
is_normal_column()
{
   if [ "$#" -ne 2 ]; then
     echo "is_normal_column(): incorrect number of arguments, pass 2 arguments: table name and column name" 1> ${TEMP_LOG} 2>&1
     return 1
   fi
  
   local tbl=$1
   local check_column=$2
   
   local rc=0
   local column
   for column in $(echo "${exception_columns[$tbl]}" | tr " " "\n"); do
      #echo "$column"

      if [ "$check_column" = "$column" ] ; then
         (( rc=1 ))
      fi
   done

   return $rc
}

# Function to check if a particular column is an exception column
# exception columns should not cause new history table entries to be created when they are updated
# Takes two parameters:
# The active table name and the column name to check
is_exception_column()
{
  # Just invert the logic of is_normal_column
  if is_normal_column $1 $2; then
     return 1
  else
     return 0
  fi
}

# Function to check that all database tables are empty
check_all_tables_empty()
{
   local tbl
   for tbl in "${tables[@]}"
   do
      # Check active table count
      if [ "${active_rows_after[$tbl]}" -ne "0" ] ; then
         echo "check_all_tables_empty(): active_rows_after for $tbl is ${active_rows_after[$tbl]}, 0 was expected!" 1>> ${TEMP_LOG} 2>&1
         return 1
      fi      

      # Check history table count
      if [ "${history_rows_after[$tbl]}" -ne "0" ] ; then
         echo "check_all_tables_empty(): history_rows_after for $tbl is ${history_rows_after[$tbl]}, 0 was expected!" 1>> ${TEMP_LOG} 2>&1
         return 1
      fi
   done

   return 0
}   

# Function to check that all active database tables are empty
check_all_active_tables_empty()
{
   if [ "$#" -ne 1 ]; then
     echo "check_all_active_tables_empty(): incorrect number of arguments, pass 1 argument: name of current running test." 1>> ${TEMP_LOG} 2>&1
     return 1
   fi
  
   local rc=0 
   local tbl
   for tbl in "${tables[@]}"
   do
      # Check active table count
      if [ "${active_rows_after[$tbl]}" -ne "0" ] ; then
         FLAGS+="\n$1: $tbl table should be empty but unexpectedly has rows populated."
         rc=1 
      fi      
   done

   return $rc 
}   

# Function to check that all database tables have at least one row 
# Function requires one parameter, the name of the current test case
check_all_tables_populated()
{
   if [ "$#" -ne 1 ]; then
     echo "check_all_tables_populated(): incorrect number of arguments, pass 1 argument: name of current running test." 1>> ${TEMP_LOG} 2>&1
     return 1
   fi

   local rc=0
   local tbl
   for tbl in "${tables[@]}"
   do
      # Check active table count
      if [ "${active_rows_after[$tbl]}" -eq "0" ] ; then
         FLAGS+="\n$1: 0 rows populated in $tbl table."
         rc=1
      else      
         # Check history table count if the active table was populated
         if [ "${history_rows_after[$tbl]}" -eq "0" ] ; then
            FLAGS+="\n$1: 0 rows populated in ${history_tables[$tbl]} table."
            rc=1
         fi
      fi
   done

   return $rc
}   

# Function to check that no new history data has appeared during the test in a specific table 
# Function requires two parameters, the name of the current test case and the name of the table to check
check_table_no_new_history()
{
   if [ "$#" -ne 2 ]; then
     echo "check_table_no_new_history(): incorrect number of arguments, pass 2 arguments: name of current running test and name of the table." 1>> ${TEMP_LOG} 2>&1
     return 1
   fi

   local tbl=$2
   # Check no new rows have appeared in the history tables during the test 
   if [ "${history_rows_after[$tbl]}" -gt "${history_rows_before[$tbl]}" ] ; then
      FLAGS+="\n$1: New rows incorrectly populated in ${history_tables[$tbl]} table (${history_rows_before[$tbl]} -> ${history_rows_after[$tbl]})."
      return 1
   else
      return 0
   fi
}   

# Function to check that no new history data has appeared during the test 
# Function requires one parameter, the name of the current test case
check_all_tables_no_new_history()
{
   if [ "$#" -ne 1 ]; then
     echo "check_all_tables_no_new_history(): incorrect number of arguments, pass 1 argument: name of current running test." 1>> ${TEMP_LOG} 2>&1
     return 1
   fi

   local rc=0
   local tbl
   for tbl in "${tables[@]}"
   do
      # Check no new rows have appeared in the history tables during the test 
      if [ "${history_rows_after[$tbl]}" -gt "${history_rows_before[$tbl]}" ] ; then
         FLAGS+="\n$1: New rows incorrectly populated in ${history_tables[$tbl]} table (${history_rows_before[$tbl]} -> ${history_rows_after[$tbl]})."
         rc=1
      fi
   done

   return $rc
}   

# Function to check that new history data has appeared during the test in a specific table 
# Function requires two parameters, the name of the current test case and the name of the table to check
check_table_new_history()
{
   if [ "$#" -ne 2 ]; then
     echo "check_table_new_history(): incorrect number of arguments, pass 2 arguments: name of current running test and name of the table." 1>> ${TEMP_LOG} 2>&1
     return 1
   fi

   local tbl=$2
   # Check new rows have appeared in the history tables during the test 
   if [ "${history_rows_after[$tbl]}" -le "${history_rows_before[$tbl]}" ] ; then
      FLAGS+="\n$1: No new rows populated in ${history_tables[$tbl]} table (${history_rows_before[$tbl]} -> ${history_rows_after[$tbl]})."
      return 1
   else
      return 0
   fi
}   

# Function to check that new history data has appeared during the test 
# Function requires one parameter, the name of the current test case
check_all_tables_new_history()
{
   if [ "$#" -ne 1 ]; then
     echo "check_all_tables_new_history(): incorrect number of arguments, pass 1 argument: name of current running test." 1>> ${TEMP_LOG} 2>&1
     return 1
   fi

   local rc=0
   local tbl
   for tbl in "${tables[@]}"
   do
      # Check no new rows have appeared in the history tables during the test 
      if [ "${history_rows_after[$tbl]}" -le "${history_rows_before[$tbl]}" ] ; then
         FLAGS+="\n$1: No new rows populated in ${history_tables[$tbl]} table (${history_rows_before[$tbl]} -> ${history_rows_after[$tbl]})."
         rc=1
      fi
   done

   return $rc
}   

##################################################################################################################
# Test Case 0: Clean table data; confirm all tables are empty
##################################################################################################################
echo "Test Case 0: Clean table data; confirm all tables are empty" >> ${LOG}
read_db_before

xdsh $SINGLE_COMPUTE "systemctl stop csmd-compute" 1>> ${TEMP_LOG} 2>&1
${CSM_PATH}/csm_node_delete -n "$SINGLE_COMPUTE" 1>> ${TEMP_LOG} 2>&1
   
for tbl in "${tables[@]}"
do
   /bin/psql -U postgres csmdb -c "DELETE FROM ${history_tables[$tbl]} where node_name='$SINGLE_COMPUTE';" 1>> ${TEMP_LOG} 2>&1
done

read_db_after

check_all_tables_empty
check_return_exit $? 0

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}

##################################################################################################################
# Test Case 1: Start the compute daemon; confirm inventory entries are created in the active and history tables 
##################################################################################################################
echo "Test Case 1: Start the compute daemon; confirm inventory entries are created in the active and history tables." >> ${LOG}
read_db_before

xdsh $SINGLE_COMPUTE "systemctl start csmd-compute" 1>> ${TEMP_LOG} 2>&1
sleep 1

read_db_after

rc=0
check_all_tables_populated "Test Case 1"
(( rc=rc+$? ))

check_all_tables_new_history "Test Case 1"
(( rc=rc+$? ))

check_return_flag $rc "Test Case 1: Not all tables were populated with inventory data."

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}

#################################################################################################################
# Test Case 2: Change the compute node state; confirm no entries are created in the history tables 
#################################################################################################################
echo "Test Case 2: Change the compute node state; confirm no entries are created in the history tables." >> ${LOG}
read_db_before

${CSM_PATH}/csm_node_attributes_update -n "$SINGLE_COMPUTE" -s "IN_SERVICE" 1>> ${TEMP_LOG} 2>&1
sleep 1

read_db_after
check_all_tables_no_new_history "Test Case 2"
check_return_flag $? "Test Case 2: New history entries were created incorrectly."

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}

#################################################################################################################
# Test Case 3: Restart the compute daemon; confirm no entries are created in the history tables 
#################################################################################################################
echo "Test Case 3: Restart the compute daemon; confirm no entries are created in the history tables." >> ${LOG}
read_db_before

xdsh $SINGLE_COMPUTE "systemctl restart csmd-compute" 1>> ${TEMP_LOG} 2>&1
sleep 1

read_db_after
check_all_tables_no_new_history "Test Case 3"
check_return_flag $? "Test Case 3: New history entries were created incorrectly."

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}

#################################################################################################################
# Test Case 4: Stop the compute daemon and delete it from the DB; confirm entries are created in the history tables 
#################################################################################################################
echo "Test Case 4: Stop the compute daemon and delete it from the DB; confirm entries are created in the history tables." >> ${LOG}
read_db_before

xdsh $SINGLE_COMPUTE "systemctl stop csmd-compute" 1>> ${TEMP_LOG} 2>&1
sleep 1
${CSM_PATH}/csm_node_delete -n "$SINGLE_COMPUTE" 1>> ${TEMP_LOG} 2>&1

read_db_after

rc=0
check_all_active_tables_empty "Test Case 4"
(( rc=rc+$? ))

check_all_tables_new_history "Test Case 4"
(( rc=rc+$? ))

check_return_flag $rc "Test Case 4: Active tables are not empty or new history entries were not created."

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}

#################################################################################################################
# Test Case 5: Start the compute daemon; confirm active tables are populated and new history entries are created 
#################################################################################################################
echo "Test Case 5: Start the compute daemon; confirm active tables are populated and new history entries are created." >> ${LOG}
read_db_before

xdsh $SINGLE_COMPUTE "systemctl start csmd-compute" 1>> ${TEMP_LOG} 2>&1
sleep 1

read_db_after

rc=0
check_all_tables_populated "Test Case 5"
(( rc=rc+$? ))

check_all_tables_new_history "Test Case 5"
(( rc=rc+$? ))

check_return_flag $rc "Test Case 5: Not all tables were populated with inventory data or new history entries were not created."

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}

#################################################################################################################
# Test Case 6: Change individual inventory fields and recollect inventory, confirm expected history is created
#################################################################################################################
echo "Test Case 6: Change individual inventory fields and recollect inventory, confirm expected history is created." >> ${LOG}

# This test is two sets of double loops
# The outer two loops loop through all tables, all columns and modify the value for each "normal" column, one by one
# Inventory is then recollected by restarting the compute daemon
# The inner two loops then loop through the history tables and confirm that only the expected columns have changed

# Loop through all of the node inventory history tables
rc=0
for i_tbl in "${tables[@]}" ; do
   history_table=${history_tables[$i_tbl]}

   #echo "$i_tbl $history_table"

   # Get the columns associated with the current table and create an index of column names
   unset -v 'columns[@]'
   unset -v 'column_index[@]'
   
   declare -A columns
   declare -A column_index
  
   i=0 
   for col in $(/bin/psql -U postgres csmdb -c "select * from $i_tbl limit 0;" | head -n 1 | tr "|" "\n")
   do
      columns["$col"]="$col"
      column_index["$col"]=$i
      (( i++ ))
   done

   for i_col in "${columns[@]}" ; do
      #echo "$i_tbl, $i_col, ${column_index[$i_col]}" 
   
#      if is_normal_column $i_tbl $i_col ; then
#         echo "1 $i_tbl, $i_col is normal"
#      else
#         echo "1 $i_tbl, $i_col is an exception"
#      fi
      
#      if is_exception_column $i_tbl $i_col ; then
#         echo "2 $i_tbl, $i_col is an exception"
#      else
#         echo "2 $i_tbl, $i_col is normal"
#      fi

      if is_normal_column $i_tbl $i_col ; then
         # Only try to modify the normal columns
         # Use a value that is valid for both text and integer columns, timestamp columns are all exception columns 
         test_value="7777777" 
         echo "Test Case 6: changing $i_tbl,$i_col to $test_value" 1>> ${TEMP_LOG} 2>&1

         #######################################################################
         # Prepare test
         #######################################################################
         /bin/psql -U postgres csmdb -c "UPDATE $i_tbl SET $i_col='$test_value' where node_name='$SINGLE_COMPUTE';" 1>> ${TEMP_LOG} 2>&1

         #######################################################################
         # Query DB contents before test
         #######################################################################
         # Read active_rows_before, history_rows_before
         read_db_before

         #######################################################################
         # Run test
         #######################################################################
         xdsh $SINGLE_COMPUTE "systemctl restart csmd-compute"
         sleep 1
         #echo "$i_tbl: ${active_row_values_before[$i_tbl]}"
         #OLDIFS=$IFS
         #IFS=","
         #for value in ${active_row_values_before[$i_tbl]} ; do
         #   echo "$i_tbl $value"
         #done
         #IFS=$OLDIFS

         #######################################################################
         # Query DB contents after test
         #######################################################################
         # Read active_rows_after, history_rows_after
         read_db_after

         #for tbl in "${tables[@]}"
         #do
         #   echo "$tbl: ${active_rows_before[$tbl]} ${active_rows_after[$tbl]} ${history_rows_before[$tbl]} ${history_rows_after[$tbl]}"
         #done
   

         #######################################################################
         # Was test successful?
         #######################################################################
         for o_tbl in "${tables[@]}" ; do
            if [ "$o_tbl" = "$i_tbl" ] ; then
               # We made a change to this active table, so we expect an updated history table
               check_table_new_history "Test Case 6" $o_tbl
               tbl_rc=$?
               (( rc=rc+$tbl_rc ))
               ((tbl_rc)) && echo "check_table_new_history returned $tbl_rc" 1>> ${TEMP_LOG} 2>&1

               if [ "${history_rows_after[$o_tbl]}" -gt "${history_rows_before[$o_tbl]}" ] ; then
                  :
                  #for each column2
                     #if column2=column
                        #if is_normal(column)
                           #if (history_value_before != history_value_before)
                           #   : Good case
                           #else
                           #   FLAG test failed, changed value in active table did not trigger changed value in history table
                        #else
                        
                     #else
               fi     # history_rows_after -gt history_rows_before
            else
               # o_tbl != i_tbl, no history change expected
               check_table_no_new_history "Test Case 6" $o_tbl
               tbl_rc=$?
               (( rc=rc+$tbl_rc ))
               ((tbl_rc)) && echo "check_table_no_new_history returned $tbl_rc" 1>> ${TEMP_LOG} 2>&1
            fi        # o_tbl = i_tbl
         done      # o_tbl loop

      fi        # is_normal_column
   done      # i_col loop 
done      # i_tbl loop
            
check_return_flag $rc "Test Case 6: History table entries unexpectedly created or unexpectedly missing."

rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "             Node Inventory DB History Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
