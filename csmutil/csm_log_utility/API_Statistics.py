import os
import errno
import subprocess
import sys
import time
import multiprocessing
import numpy as np
from datetime import datetime

import argparse

sys.path.insert(0, './Helper_Files')

import Log_Object as LO
from Aggregator_Stat import compute_CSM_Aggregator_stats
from Compute_Stat import compute_CSM_Compute_stats
from Master_Stat import compute_CSM_Master_stats
from Utility_Stat import compute_CSM_Utility_stats


# Variables
time_format = '%Y-%m-%d %H:%M:%S.%f'
start_datetime = datetime.strptime('1000-01-01' + ' ' + '00:00:00.0000', time_format)
end_datetime   = datetime.strptime('9999-01-01' + ' ' + '00:00:00.0000', time_format)
# How to order the output.
order_by = 0;
reverse_order = False;


logs_path = ""

parser = argparse.ArgumentParser(
    description='''A tool for parsing daemon logs for API statistics.''')
parser.add_argument( '-p', metavar='path', dest='directory_path', default=None,
    help='The directory path to where the logs are located. Defaults to: \'/var/log/ibm/csm\'')
parser.add_argument( '-s', metavar='start', dest='start_arg', default=None,
    help='start of search range. Defaults to: \'1000-01-01 00:00:00.0000\'')
parser.add_argument( '-e', metavar='end', dest='end_arg', default=None,
    help='end of search range. Defaults to: \'9999-01-01 00:00:00.0000\'')
parser.add_argument( '-o', metavar='order', dest='order_arg', default=0,
    help='order the results by a field. Defaults to alphabetical by API name. Valid values: 0 = alphabetical, 1 = Frequency, 2 = Mean, 3 = Max, 4 = Min, 5 = Std')
parser.add_argument( '-r', metavar='reverse', dest='reverse_arg', default=0,
    help='reverse the order of the data. Defaults to 0. Set to 1 to turn on.')
args = parser.parse_args()

#Default log location for CSM logs, unless a user supplies a path
if(args.directory_path):
    logs_path = args.directory_path
else:
    logs_path = "/var/log/ibm/csm"   

#---------- Takes in user Command Line Arguments----------
# Format: YYYY-MM-DD HH:MM::SS
# python api_statistics.py                                                  - looks through entire log file
# python api_statistics.py <Start Date> <Start Time>                        - looks from Start Date/Time to end of log file
# python api_statistics.py <Start Date> <Start Time> <End Date> <End Time>  - looks from Start Date/Time to End Date/Time

if(args.start_arg):
    start_datetime = datetime.strptime(args.start_arg, '%Y-%m-%d %H:%M:%S.%f')
if(args.end_arg):
    end_datetime = datetime.strptime(args.end_arg, '%Y-%m-%d %H:%M:%S.%f')
if(args.order_arg):
    order_by = int(args.order_arg)
    if(order_by > 5 or order_by < 0):
        order_by = 0
if(int(args.reverse_arg) == 1):
    reverse_order = True


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
print "Search from:  %s to %s" % (str(start_datetime), str(end_datetime))



sys.path.insert(0, logs_path)


def handle_file(filename):
    if "compute" in filename and ".log" in filename:
        start = time.time()
        compute_CSM_Compute_stats(filename,start_datetime,end_datetime, order_by, reverse_order)
        print 'Run Time: ' + str(time.time() - start) + '\n'
    elif "aggregator" in filename and ".log" in filename:
        start = time.time()
        compute_CSM_Aggregator_stats(filename,start_datetime, end_datetime, order_by, reverse_order)
        print 'Run Time: ' + str(time.time() - start) + '\n'
    elif "master" in filename and ".log" in filename:
        start = time.time()
        compute_CSM_Master_stats(filename,start_datetime,end_datetime, order_by, reverse_order)
        print 'Run Time: ' + str(time.time() - start) + '\n'
    elif "utility" in filename and ".log" in filename:
        start = time.time()
        compute_CSM_Utility_stats(filename,start_datetime,end_datetime, order_by, reverse_order)
        print 'Run Time: ' + str(time.time() - start) + '\n'



#---------- Get all the log files within this directory and its subdirectories ---------
directory = logs_path # Starts with current api_statistics.py directory location provided or default
file_list = list()
subdirs = [x[0] for x in os.walk(directory)]
for subdirectory in subdirs:
    files = os.walk(subdirectory).next()[2]
    if (len(files) > 0):
        total_errors = 0
        for file in files:
            filename = subdirectory + "/" + file
            if ("compute" in filename or "aggregator" in filename or "master" in filename or "utility" in filename) and ".log" in filename and 'Error' not in filename and '.txt' not in filename:
            # if 'log.old.5' in filename and 'txt' not in filename:
            	file_list.append(filename)

print file_list

#---------- Parallelize with each process handling a different file ----------
jobs = [ multiprocessing.Process(target=handle_file, args=(i,)) for i in file_list]
for j in jobs:
    j.start()
for j in jobs:
    j.join()


