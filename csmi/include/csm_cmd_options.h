/*================================================================================

    csmi/include/csm_cmd_options.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

// All CSM API commandlines should have the same name as the API they implement.
// Example: csm_node_attributes_update
//
// Commands should use a standardized set of options, to be defined in this file.
//
// We will use getopt_long, with each flag having a long and short option.
// Example: --noderange,-n long and short option to specify node/noderange
// 
// All options should have a flag, no options should be position dependant.
// Good: csm_command -n node1 -s "some string"
//       csm_command -s "some string" -n node1
// Bad:  csm_command node1 -s "some string"
//
// All CSM API commandline programs should be shipped to /opt/ibm/csm/bin or /opt/ibm/csm/sbin 
//
// As much as possible, CSM API commandline programs should work in a similar fashion to xCAT
// commands, for administrator ease of use.
//
// All CSM API commandline programs should implement a standardized output option
// intended for parsing by user programs.
// Possible options include YAML format, xCAT stanza format, CSV

// REQUIRED FOR ALL COMMAND LINES ============================

// help option: --help, -h
// Required for all commands
#define CSM_OPT_HELP_LONG "help"
#define CSM_OPT_HELP_SHORT 'h'

// verbose option: --verbose, -v
// Required for all commands
#define CSM_OPT_VERBOSE_LONG "verbose"
#define CSM_OPT_VERBOSE_SHORT 'v'

//==========================================

//1
// feature_1 option: --feature_1, -1
//node update
#define CSM_OPT_FEATURE_1_LONG "feature_1"
#define CSM_OPT_FEATURE_1_SHORT '1'

//2
// feature_2 option: --feature_2, -2
//node update
#define CSM_OPT_FEATURE_2_LONG "feature_2"
#define CSM_OPT_FEATURE_2_SHORT '2'

//3
// feature_3 option: --feature_3, -3
//node update
#define CSM_OPT_FEATURE_3_LONG "feature_3"
#define CSM_OPT_FEATURE_3_SHORT '3'

//3
// feature_4 option: --feature_4, -4
//node update
#define CSM_OPT_FEATURE_4_LONG "feature_4"
#define CSM_OPT_FEATURE_4_SHORT '4'

//A
// allocation_id option: --allocation_id, -a
#define CSM_OPT_ALLOCATION_ID_LONG "allocation_id"
#define CSM_OPT_ALLOCATION_ID_SHORT 'a'

//B
// network_bandwith option: --network_bandwith, -b
// used by allocation_step_end
#define CSM_OPT_NETWORK_BANDWITH_LONG "network_bandwith"
#define CSM_OPT_NETWORK_BANDWITH_SHORT 'b'
// begin_time option: --begin_time, -b
// used by diag_result_create
#define CSM_OPT_BEGIN_TIME_LONG "begin_time"
#define CSM_OPT_BEGIN_TIME_SHORT 'B'

//C
// compute_nodes option: --compute_nodes, -c
#define CSM_OPT_COMPUTE_NODES_LONG "compute_nodes"
#define CSM_OPT_COMPUTE_NODES_SHORT 'c'
// cpu_stats option: --cpu_stats, -C
// used by allocation_step_end
#define CSM_OPT_CPU_STATS_LONG "cpu_stats"
#define CSM_OPT_CPU_STATS_SHORT 'C'
// cmd_line option: --cmd_line, -C
// used by diag_run_begin
//#define CSM_OPT_CMD_LINE_LONG "cmd_line"
//#define CSM_OPT_CMD_LINE_SHORT 'C'

//D
// diag_status option: --diag_status, -d
#define CSM_OPT_DIAG_STATUS_LONG "diag_status"
#define CSM_OPT_DIAG_STATUS_SHORT 'd'

//E
// environment_variable option: --environment_variable, -j
#define CSM_OPT_ENVIRONMENT_VARIABLE_LONG "environment_variable"
#define CSM_OPT_ENVIRONMENT_VARIABLE_SHORT 'j'
// exit_status option: --exit_status, -E
#define CSM_OPT_EXIT_STATUS_VARIABLE_LONG "exit_status"
#define CSM_OPT_EXIT_STATUS_VARIABLE_SHORT 'E'

//F
// option: --file, -f
#define CSM_OPT_FILE_LONG "file"
#define CSM_OPT_FILE_SHORT "f"

//G
// num_gpus option: --num_gpus, -g
#define CSM_OPT_NUM_GPUS_LONG "num_gpus"
#define CSM_OPT_NUM_GPUS_SHORT 'g'
// gpu_stats option: --gpu_stats, -g
#define CSM_OPT_GPU_STATS_LONG "gpu_stats"
#define CSM_OPT_GPU_STATS_SHORT 'G'

//I
// seq_id option: --seq_id, -i
#define CSM_OPT_SED_ID_LONG "seq_id"
#define CSM_OPT_SED_ID_SHORT 'i'
// io_stats option: --io_stats, -I
// used by allocation_step_end
#define CSM_OPT_IO_STATS_LONG "io_stats"
#define CSM_OPT_IO_STATS_SHORT 'I'
// inserted_ras option: --inserted_ras, -I
// used by diag_run_end
//#define CSM_OPT_INSERTED_RAS_LONG "inserted_ras"
//#define CSM_OPT_INSERTED_RAS_SHORT 'I'

//J
// primary job id option: --primary_job_id, -j
#define CSM_OPT_PRIMARY_JOB_ID_LONG "primary_job_id"
#define CSM_OPT_PRIMARY_JOB_ID_SHORT 'j'
// secondary job id option: --secondary_job_id, -J
#define CSM_OPT_SECONDARY_JOB_ID_LONG "secondary_job_id"
#define CSM_OPT_SECONDARY_JOB_ID_SHORT 'J'

//L
// level_gpu_usage option: --level_gpu_usage, -l
#define CSM_OPT_LEVEL_GPU_USAGE_LONG "level_gpu_usage"
#define CSM_OPT_LEVEL_GPU_USAGE_SHORT 'l'
// log_file option: --log_file, -L
//used by diag_result_create
#define CSM_OPT_LOG_FILE_LONG "log_file"
#define CSM_OPT_LOG_FILE_SHORT 'L'
// log_dir option: --log_dir, -L
//used by diag_result_create
//#define CSM_OPT_LOG_DIR_LONG "log_dir"
//#define CSM_OPT_LOG_DIR_SHORT 'L'

//M
// num_memory option: --num_memory, -m
#define CSM_OPT_NUM_MEMORY_LONG "num_memory"
#define CSM_OPT_NUM_MEMORY_SHORT 'm'
// memory_stats option: --memory_stats, -M
// used by allocation_step_end
#define CSM_OPT_MEMORY_STATS_LONG "memory_stats"
#define CSM_OPT_MEMORY_STATS_SHORT 'M'
// max_memory option: --max_memory, -m
// used by allocation_step_end
//#define CSM_OPT_MAX_MEMORY_LONG "max_memory"
//#define CSM_OPT_MAX_MEMORY_SHORT 'M'
// max_swap option: --max_swap, -m
// used by allocation_step_end
//#define CSM_OPT_MAX_SWAP_LONG "max_swap"
//#define CSM_OPT_MAX_SWAP_SHORT 'M'

//N
// node_range option: --node_range, -n
#define CSM_OPT_NODE_RANGE_LONG "node_range"
#define CSM_OPT_NODE_RANGE_SHORT 'n'
// num_nodes option: --num_nodes, -N
#define CSM_OPT_NUM_NODES_LONG "num_nodes"
#define CSM_OPT_NUM_NODES_SHORT 'N'
// node_names option: --node_names, -n
// used by node_resources_query
//#define CSM_OPT_NODE_NAMES_LONG "node_names"
//#define CSM_OPT_NODE_NAMES_SHORT 'n'
// node_name option: --node_name, -n 
// used by diag_result_create and node query and query history and node update
//#define CSM_OPT_NODE_NAMES_LONG "node_name"
//#define CSM_OPT_NODE_NAMES_SHORT 'n'

//P
// num_processors option: --num_processors, -p
#define CSM_OPT_NUM_PROCESSORS_LONG "num_processors"
#define CSM_OPT_NUM_PROCESSORS_SHORT 'p'
// physical_frame_location option: --physical_frame_location, -p
//used by node update
#define CSM_OPT_PHYSICAL_FRAME_LOCATION_LONG "physical_frame_location"
#define CSM_OPT_PHYSICAL_FRAME_LOCATION_SHORT 'p'
//used by node update
#define CSM_OPT_PHYSICAL_U_LOCATION_LONG "physical_u_location"
#define CSM_OPT_PHYSICAL_U_LOCATION_SHORT 'p'

//R

// ready option: --ready, -r
//used by node update
#define CSM_OPT_READY_LONG "ready"
#define CSM_OPT_READY_SHORT 'R'
// option: --role, -r
//used by daemon
#define CSM_OPT_ROLE_LONG "role"
#define CSM_OPT_ROLE_SHORT "r"
// arguments option: --arguments, -r
// #define CSM_OPT_ARGUMENTS_LONG "arguments"
// #define CSM_OPT_ARGUMENTS_SHORT 'r'
// option: --run_id, -r
//used by diag_run_query and query details and run end
//#define CSM_OPT_RUN_ID_LONG "run_id"
//#define CSM_OPT_RUN_ID_SHORT "r"

//S
// state option: --state, -s
//used by node udpate
#define CSM_OPT_STATE_LONG "state"
#define CSM_OPT_STATE_SHORT 's'
// step_id option: --step_id, -S
#define CSM_OPT_STEP_ID_LONG "step_id"
#define CSM_OPT_STEP_ID_SHORT 'S'
// total_s_time option: --total_s_time, -S
//used in step end
// #define CSM_OPT_TOTAL_S_TIME_LONG "total_s_time"
// #define CSM_OPT_TOTAL_S_TIME_SHORT 'S'
// serial_number option: --serial_number, -S
//used in diag_result_create
// #define CSM_OPT_TOTAL_S_TIME_LONG "serial_number"
// #define CSM_OPT_TOTAL_S_TIME_SHORT 'S'
//used in diag_result_create and run end
// #define CSM_OPT_STATUS_LONG "status"
// #define CSM_OPT_STATUS_SHORT 'S'

//T
// type option: --type, -t
#define CSM_OPT_TYPE_LONG "type"
#define CSM_OPT_TYPE_SHORT 't'
// num_tasks option: --num_tasks, -T
#define CSM_OPT_NUM_TASKS_LONG "num_tasks"
#define CSM_OPT_NUM_TASKS_SHORT 'T'
// test_name option: --test_name, -t
//used in diag_result_create
// #define CSM_OPT_TEST_NAME_LONG "test_name"
// #define CSM_OPT_TEST_NAME_SHORT 't'

//U
// user_name option: --user_name, -u
#define CSM_OPT_USER_NAME_LONG "user_name"
#define CSM_OPT_USER_NAME_SHORT 'u'
// username option: --user_id, -U
#define CSM_OPT_USER_ID_LONG "user_id"
#define CSM_OPT_USER_ID_SHORT 'U'
// total_u_time option: --total_u_time, -U
// used by allocation_step_end
// #define CSM_OPT_TOTAL_U_TIME_LONG "total_u_time"
// #define CSM_OPT_TOTAL_U_TIME_SHORT 'u'
// total_num_threads option: --total_num_threads, -T
// used by allocation_step_end
// #define CSM_OPT_TOTAL_NUM_THREADS_LONG "total_num_threads"
// #define CSM_OPT_TOTAL_NUM_THREADS_SHORT 'T'

//W
// working_directory option: --working_directory, -w
#define CSM_OPT_WORKING_DIRECTORY_LONG "working_directory"
#define CSM_OPT_WORKING_DIRECTORY_SHORT 'w'

//X
// executable option: --executable, -e
#define CSM_OPT_EXECUTABLE_LONG "executable"
#define CSM_OPT_EXECUTABLE_SHORT 'e'