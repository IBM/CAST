import os
import errno
import subprocess
import sys
import numpy as np
from datetime import datetime

sys.path.insert(0, './Helper_Files')

import Log_Object as LO
import subprocess
from subprocess import Popen, PIPE
import shlex

time_format = '%Y-%m-%d %H:%M:%S.%f'
# start_datetime = datetime.strptime('1000-01-01' + ' ' + '00:00:00.0000', time_format)
# end_datetime   = datetime.strptime('9999-01-01' + ' ' + '00:00:00.0000', time_format)

print_padding = 115
total_errors = 0

def compute_CSM_Utility_stats(filename, start_datetime, end_datetime):
    opened_file = Pre_Process(filename) #open(filename, 'r')                   
    dictionary = dict()                 # Dictionary key uses Api operation ID
    Api_Statistics = dict()             # Api_Statistics key uses Api name
    report_file_path = './Reports/Utility_Reports/Report_'+ filename[15:] +'.txt'
    if not os.path.exists(os.path.dirname(report_file_path)):
        try:
            os.makedirs(os.path.dirname(report_file_path))
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

    report_file = open(report_file_path, 'w')

#[UTL]2018-05-30 16:56:30.728978     csmapi::info     | CSM_CMD_node_resources_query_all[1216090507]; Client Sent; PID: 14064; UID:0; GID:0

    for line in opened_file :
        if '|' in line:
            Log_Line = LO.Log(line)
            if Log_Line.DateTime >= start_datetime and Log_Line.DateTime <= end_datetime:   # Looking for only start and end time of Api call
                if 'csmapi' in line:
                    start_line = line.split()
                    start_key = start_line[4].split('[')[1]
                    if start_key not in dictionary.keys():       # Api ID does not exist, add to dictionary
                        dictionary[start_key] = Log_Line
                    else:                                        # Api ID exists, calculate time and remove
                        end_line = dictionary[start_key].line.split()
                        if end_line[4].split('[')[0] != start_line[4].split('[')[0]:   # Api ID collision. Mismatch of operations
                            del dictionary[start_key]
                            report_file.write(collision_error(start_key, start_line, end_line))
                        else:                                                        # Matching operations and ID    
                            run_time = calculate_time_difference(dictionary[start_key].DateTime, Log_Line.DateTime, time_format)
                            del dictionary[start_key]
                            start_api = start_line[4].split('[')[0]
                            if  start_api in Api_Statistics.keys():
                                Api_Statistics[start_api].append(run_time)
                            else:
                                Api_Statistics[start_api] = [run_time]
            else:
                break
    end_stats = calculate_statistics(filename, Api_Statistics)
    report_file.write(end_stats)
    report_file.close()

def calculate_time_difference(start_time, end_time, time_format):
    runtime = end_time - start_time
    return runtime.total_seconds()

def collision_error(Api_Id, start_api, end_api):
    error = "Error: Api ID collision: " + Api_Id + '\n'  + "Start Api: " + ' '.join(start_api) + '\n' + "End   Api: " + ' '.join(end_api) + '\n\n'
    global total_errors 
    total_errors += 1
    return error

def calculate_statistics(filename, Api_Statistics):
    print ("Utility Log").center(print_padding, '+')
    print filename[2:].center(print_padding, '-')
    print '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std")
    # print Api_Statistics.keys()
    total_calls = 0
    stats = ("Utility Log").center(print_padding, '+') + '\n' + filename[2:].center(print_padding, '-') + '\n' + '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std") + '\n'
    for key in Api_Statistics.keys():
        stat_line = '{:50s} {:10d}  {:3.6f}  {:3.6f}  {:3.6f}  {:3.6f}  {:3.6f}'.format(
            key, 
            len(Api_Statistics[key]),
            np.mean(Api_Statistics[key]),
            np.median(Api_Statistics[key]),
            min(Api_Statistics[key]),
            max(Api_Statistics[key]),
            np.std(Api_Statistics[key]))
        total_calls = total_calls + len(Api_Statistics[key])
        print stat_line
        stats = stats + '\n' + stat_line
    print "Total Calls:  " + str(total_calls)
    print "Total Errors: " + str(total_errors) + '\n'  
    return stats + '\n' + "Total Calls:  " + str(total_calls) + '\n' + "Total Errors: " + str(total_errors) + '\n' 

def Pre_Process(filename):
    p1 = Popen(shlex.split("grep \'csmapi\' " + filename ), stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
    return p1.split('\n')