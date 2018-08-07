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

def compute_CSM_Aggregator_stats(filename, start_datetime, end_datetime):
    opened_file = Pre_Process(filename) #open(filename, 'r')                   
    dictionary = dict()                 # Dictionary key uses Api operation ID
    Api_Statistics = dict()             # Api_Statistics key uses Api name
    Api_Statistics['responses'] = []
    report_file_path = './Reports/Aggregator_Reports/Report_'+ filename[18:] +'.txt'
    if not os.path.exists(os.path.dirname(report_file_path)):
        try:
            os.makedirs(os.path.dirname(report_file_path))
        except OSError as exc: # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise

    report_file = open(report_file_path, 'w')

# [AGG]2018-05-31 15:42:32.083870       csmd::info     | AGG-MTC: Expecting 1 responses. Setting timeout to 58000 milliseconds.

    for line in opened_file :
        print 'Outputting line: ' + line
        if '|' in line:
            Log_Line = LO.Log(line)
            if Log_Line.DateTime >= start_datetime and Log_Line.DateTime <= end_datetime:   # Looking for only start and end time of Api call
                if 'AGG-MTC' in line:
                    responses = int(line.split()[8])
                    Api_Statistics['responses'].append(responses)
            else:
                break
    end_stats = calculate_statistics(filename, Api_Statistics)
    report_file.write(end_stats)
    report_file.close()

def calculate_time_difference(start_time, end_time, time_format):
    runtime = end_time - start_time
    return runtime.total_seconds()

def collision_error(Api_Id, start_api, end_api):
    print "Error: Api ID collision: " + Api_Id
    print "Start Api: " + ' '.join(start_api)
    print "End   Api: " + ' '.join(end_api)
    global total_errors 
    total_errors += 1

def calculate_statistics(filename, Api_Statistics):
    print ("Aggregator Log").center(print_padding, '+')
    print filename[2:].center(print_padding, '-')
    print '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std")
    total_calls = 0
    stats = ("Aggregator Log").center(print_padding, '+') + '\n' + filename[2:].center(print_padding, '-') + '\n' + '{:50s} {:10s}  {:8s}  {:8s}  {:8s}  {:8s}  {:8s}'.format('Api Function', "Frequency", "Mean", "Median", "Min", "Max", "Std") + '\n'
    if not Api_Statistics['responses']:
        pass
    else:
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
    p1 = Popen(shlex.split("grep \'AGG-MTC\' " + filename ), stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
    # p2 = Popen(split("grep -v \'Allocation\'"), stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
    return p1.split('\n')