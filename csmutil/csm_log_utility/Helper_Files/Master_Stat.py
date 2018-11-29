import os
import errno
import subprocess
import sys
import numpy as np
from datetime import datetime

import time

sys.path.insert(0, './Helper_Files')

import Log_Object as LO
import subprocess
from subprocess import Popen, PIPE
from shlex import split

time_format = '%Y-%m-%d %H:%M:%S.%f'
# start_datetime = datetime.strptime('1000-01-01' + ' ' + '00:00:00.0000', time_format)
# end_datetime   = datetime.strptime('9999-01-01' + ' ' + '00:00:00.0000', time_format)

print_padding = 115
total_errors = 0



def compute_CSM_Master_stats(filename, start_datetime, end_datetime, order_by, reverse_order):
    opened_file = Pre_Process(filename) #open(filename, 'r') 
    dictionary = dict()         # Dictionary key uses Api operation ID
    Api_Statistics = dict()     # Api_Statistics key uses Api name
    report_file_path = './Reports/Master_Reports/Report_'+ filename[14:] +'.txt'
    if not os.path.exists(os.path.dirname(report_file_path)):
        try:
            os.makedirs(os.path.dirname(report_file_path))
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

    report_file = open(report_file_path, 'w')

#       2018-06-06 10:25:04.905532     csmapi::info     | CSM_CMD_node_resources_query[315229106]; Client Sent; PID: 48489; UID:0; GID:0
#       2018-06-06 10:25:04.905677     csmapi::info     | [315229106]; csm_node_resources_query start
#       2018-06-06 10:25:04.908051     csmapi::info     | [315229106]; csm_node_resources_query end
#       2018-06-06 10:25:04.908184     csmapi::info     | CSM_CMD_node_resources_query[315229106]; Client Recv; PID: 48489; UID:0; GID:0

    for line in opened_file :
        if '|' in line:
            Log_Line = LO.Log(line)
            if Log_Line.DateTime >= start_datetime and Log_Line.DateTime <= end_datetime:
                if 'start' in line and 'csmapi::' in line and 'Allocation' not in line:
                    start_line = line.split()
                    start_key = start_line[4]
                   
                    if start_key not in dictionary.keys():     # Api ID does not exist, add to dictionary
                        dictionary[start_key] = Log_Line             
                    else:                                      # Api ID exists, calculate time and remove
                        end_line = dictionary[start_key].line.split()
                        if end_line[5] != start_line[5] or end_line[6] == 'start':      # Api ID collision. Mismatch of operations
                            del dictionary[start_key]
                            report_file.write( collision_error(start_key, start_line, end_line))
                        else:                                                           # Matching operations and ID
                            run_time = calculate_time_difference(Log_Line.DateTime, dictionary[start_key].DateTime, time_format)
                            del dictionary[start_key]
                            if     start_line[5] in Api_Statistics.keys():
                                Api_Statistics[start_line[5]].append(run_time)
                            else:
                                Api_Statistics[start_line[5]] = [run_time]
                    
                elif 'end' in line and 'csmapi::' in line and 'Allocation' not in line:
                    end_line = line.split()
                    end_key = end_line[4]
                    
                    if end_key not in dictionary.keys():        # Api ID does not exist, add to dictionary
                        dictionary[end_key] = Log_Line
                    else:                                       # Api ID exists, calculate time and remove
                        start_line = dictionary[end_key].line.split()
                        if end_line[5] != start_line[5] or start_line[6] == 'end':      # Api ID collision. Mismatch of operations
                            del dictionary[end_key]
                            report_file.write( collision_error(end_key, start_line, end_line))
                        else:                                                           # Matching operations and ID
                            run_time = calculate_time_difference(dictionary[end_key].DateTime, Log_Line.DateTime, time_format)
                            del dictionary[end_key]
                            if  end_line[5] in Api_Statistics.keys():
                                Api_Statistics[end_line[5]].append(run_time)
                            else:
                                Api_Statistics[end_line[5]] = [run_time]
            else:
                break
    end_stats = calculate_statistics(filename, Api_Statistics, order_by, reverse_order)
    report_file.write(end_stats)
    report_file.close()

def calculate_time_difference(start_time, end_time, time_format):
    runtime = end_time - start_time
    return runtime.total_seconds()

def collision_error(Api_Id, start_api, end_api):
    # print "Error: Api ID collision: " + Api_Id
    # error_file.write("Error: Api ID collision: " + Api_Id)
    # print "Start Api: " + ' '.join(start_api)
    # error_file.write("Start Api: " + ' '.join(start_api))
    # print "End   Api: " + ' '.join(end_api)
    # error_file.write("End   Api: " + ' '.join(end_api))
    error = "Error: Api ID collision: " + Api_Id + '\n'  + "Start Api: " + ' '.join(start_api) + '\n' + "End   Api: " + ' '.join(end_api) + '\n\n'
    global total_errors 
    total_errors += 1
    return error

def calculate_statistics(filename, Api_Statistics, order_by, reverse_order):
    print ("Master Log").center(print_padding, '+')
    print filename.center(print_padding, '-')
    file_opened = open(filename, 'r')
    file_lines = file_opened.readlines()
    file_start = file_lines[0].split("csm")[0].strip()
    file_end   = file_lines[-1].split("csm")[0].strip()
    file_time = ("File Start: {0} and File End: {1}".format(file_start, file_end)).center(print_padding, '-')
    print '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std")
    
    total_calls = 0

    # Dictionary to hold all apis with full stats.
    dic_API_Stats = {}

    stats = ("Master Log").center(print_padding, '+') + '\n' + filename.center(print_padding, '-') + '\n' + file_time +'\n' + '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std") + '\n'
    for key in Api_Statistics.keys():
        #apis with stats
        dic_API_Stats[key] = [key]
        dic_API_Stats[key].append(len(Api_Statistics[key]))
        dic_API_Stats[key].append(np.mean(Api_Statistics[key]))
        dic_API_Stats[key].append(np.median(Api_Statistics[key]))
        dic_API_Stats[key].append(min(Api_Statistics[key]))
        dic_API_Stats[key].append(max(Api_Statistics[key]))
        dic_API_Stats[key].append(np.std(Api_Statistics[key]))

        total_calls = total_calls + len(Api_Statistics[key])
    
    for key, value in sorted(dic_API_Stats.iteritems(), key=lambda (k,v): (v[order_by],k), reverse = reverse_order):
        stat_line = '{:50s} {:10d}  {:3.6f}  {:3.6f}  {:3.6f}  {:3.6f}  {:3.6f}'.format(value[0], value[1], value[2], value [3], value [4], value [5], value[6])
        print stat_line
        stats = stats + '\n' + stat_line

    print "Total Calls:  " + str(total_calls)
    print "Total Errors: " + str(total_errors) + '\n'  
    return stats + '\n' + "Total Calls:  " + str(total_calls) + '\n' + "Total Errors: " + str(total_errors) + '\n' 

def Pre_Process(filename):
    p1 = Popen(split("grep \'start\|end\' " + filename ), stdout=PIPE, stderr=subprocess.PIPE)
    p2 = Popen(split("grep -v \'Allocation\'"), stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
    return p2.split('\n')

# if len(sys.argv) == 1:
#     pass
# elif len(sys.argv) == 3:
#     start_datetime = datetime.strptime(sys.argv[1] + ' ' + sys.argv[2], '%Y-%m-%d %H:%M:%S.%f')
# elif len(sys.argv) == 5:
#     start_datetime = datetime.strptime(sys.argv[1] + ' ' + sys.argv[2], '%Y-%m-%d %H:%M:%S.%f')
#     end_datetime = datetime.strptime(sys.argv[3] + ' ' + sys.argv[4], '%Y-%m-%d %H:%M:%S.%f')
# else:
#     print "Incorrect Number of Arguments. Please use one of the following"
#     print "python api_statistics.py"
#     print "python api_statistics.py <Start Date YYYY-MM-DD> <Start Time HH:MM:SS>"
#     print "python api_statistics.py <Start Date YYYY-MM-DD> <Start Time HH:MM:SS> <End Date YYYY-MM-DD> <End Time HH:MM:SS>"
# print "Search from:  %s to %s" % (str(start_datetime), str(end_datetime))

# compute_CSM_Master_stats('csm_resources.txt')
# compute_CSM_Master_stats('csm_master_LL.log')
# compute_CSM_Master_stats('csm_master.log.old.5')

# start_time = time.time()
# compute_CSM_Master_stats('csm_master_OR.log.old.1')
# elapsed_time = time.time() - start_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master.log.old.2')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master.log.old.3')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master.log.old.4')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master.log.old.5')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time

# start_time = time.time()
# compute_CSM_Master_stats('csm_master_LL.log.old.1')
# elapsed_time = time.time() - start_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master_LL.log.old.2')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master_LL.log.old.3')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master_LL.log.old.4')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time
# compute_CSM_Master_stats('csm_master_LL.log.old.5')
# elapsed_time = time.time() - elapsed_time
# print elapsed_time