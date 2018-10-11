import os
import errno
import subprocess
import sys
import numpy as np
from datetime import datetime

sys.path.insert(0, './Helper_Files')

import subprocess
from subprocess import Popen, PIPE
from shlex import split

import Log_Object as LO

time_format = '%Y-%m-%d %H:%M:%S.%f'
# start_datetime = datetime.strptime('1000-01-01' + ' ' + '00:00:00.0000', time_format)
# end_datetime   = datetime.strptime('9999-01-01' + ' ' + '00:00:00.0000', time_format)

print_padding = 115
total_errors = 0

def compute_CSM_Compute_stats(filename, start_datetime, end_datetime):
    opened_file = Pre_Process(filename) #open((filename), 'r')                   
    dictionary = dict()                 # Dictionary key uses Api operation ID
    Api_Statistics = dict()             # Api_Statistics key uses Api name
    report_file_path = './Reports/Compute_Reports/Report_'+ filename[15:] +'.txt'
    if not os.path.exists(os.path.dirname(report_file_path)):
        try:
            os.makedirs(os.path.dirname(report_file_path))
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

    report_file = open(report_file_path, 'w')

    for line in opened_file :
        if '|' in line:
            Log_Line = LO.Log(line)
            if Log_Line.DateTime >= start_datetime and Log_Line.DateTime <= end_datetime:   # Looking for only start and end time of Api call
                if 'start' in line and 'csmapi::' in line and 'Allocation' not in line:
                    start_line = line.split()
                    start_key = start_line[4]
                    if start_key not in dictionary.keys():       # Api ID does not exist, add to dictionary
                        dictionary[start_key] = Log_Line
                    else:                                        # Api ID exists, calculate time and remove
                        end_line = dictionary[start_key]
                        if end_line[5] != start_line[5] or end_line[6] == 'start':   # Api ID collision. Mismatch of operations
                            del dictionary[start_key]
                            report_file.write(collision_error(start_key, start_line, end_line))
                        else:                                                        # Matching operations and ID    
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
                        if end_line[5] != start_line[5] or start_line[6] == 'end':   # Api ID collision. Mismatch of operations
                            del dictionary[end_key]
                            report_file.write(collision_error(end_key, start_line, end_line))
                        else:                                                        # Matching operations and ID
                            run_time = calculate_time_difference(dictionary[end_key].DateTime, Log_Line.DateTime, time_format)
                            del dictionary[end_key]
                            if  end_line[5] in Api_Statistics.keys():
                                Api_Statistics[end_line[5]].append(run_time)
                            else:
                                Api_Statistics[end_line[5]] = [run_time]
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
    print ("Compute Log").center(print_padding, '+')
    print filename[2:].center(print_padding, '-')
    file_opened = open(filename, 'r')
    file_lines = file_opened.readlines()
    file_start = file_lines[0].split("csm")[0][9:].strip()
    file_end   = file_lines[-1].split("csm")[0][9:].strip()
    file_time = ("File Start: {0} and File End: {1}".format(file_start, file_end)).center(print_padding, '-')
    
    print '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std")
    # print Api_Statistics.keys()
    total_calls = 0
    stats = ("Compute Log").center(print_padding, '+') + '\n' + filename[2:].center(print_padding, '-') + '\n' + file_time  + '\n' + '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std") + '\n'
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
    p1 = Popen(split("grep \'start\|end\' " + filename ), stdout=PIPE, stderr=subprocess.PIPE)
    p2 = Popen(split("grep -v \'Allocation\'"), stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
    return p2.split('\n')
