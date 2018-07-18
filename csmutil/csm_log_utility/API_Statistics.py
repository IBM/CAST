import os
import errno
import subprocess
import sys
import time
import multiprocessing
import numpy as np
from datetime import datetime


sys.path.insert(0, './Helper_Files')
sys.path.insert(0,'./Logs')

import Log_Object as LO
from Aggregator_Stat import compute_CSM_Aggregator_stats
from Computer_Stat import compute_CSM_Compute_stats
from Master_Stat import compute_CSM_Master_stats
from Utility_Stat import compute_CSM_Utility_stats

time_format = '%Y-%m-%d %H:%M:%S.%f'
start_datetime = datetime.strptime('1000-01-01' + ' ' + '00:00:00.0000', time_format)
end_datetime   = datetime.strptime('9999-01-01' + ' ' + '00:00:00.0000', time_format)

def handle_file(filename):
    if "compute" in filename and ".log" in filename:
        print 'Compute: ' + filename
        start = time.time()
        compute_CSM_Compute_stats(filename,start_datetime,end_datetime)
        print 'Run Time: ' + str(time.time() - start) + '\n'
    elif "aggregator" in filename and ".log" in filename:
        print 'Aggregate: ' + filename
        start = time.time()
        compute_CSM_Aggregator_stats(filename,start_datetime, end_datetime)
        print 'Run Time: ' + str(time.time() - start) + '\n'
    elif "master" in filename and ".log" in filename:
        print 'Master: ' + filename
        start = time.time()
        compute_CSM_Master_stats(filename,start_datetime,end_datetime)
        print 'Run Time: ' + str(time.time() - start) + '\n'
    elif "utility" in filename and ".log" in filename:
        start = time.time()
        # pass
        print 'Utility: ' + filename
        compute_CSM_Utility_stats(filename,start_datetime,end_datetime)
        print 'Run Time: ' + str(time.time() - start) + '\n'


#---------- Takes in user Command Line Arguments----------
# Format: YYYY-MM-DD HH:MM::SS
# python api_statistics.py                                                  - looks through entire log file
# python api_statistics.py <Start Date> <Start Time>                        - looks from Start Date/Time to end of log file
# python api_statistics.py <Start Date> <Start Time> <End Date> <End Time>  - looks from Start Date/Time to End Date/Time
if len(sys.argv) == 1:
    pass
elif len(sys.argv) == 3:
    start_datetime = datetime.strptime(sys.argv[1] + ' ' + sys.argv[2], '%Y-%m-%d %H:%M:%S.%f')
elif len(sys.argv) == 5:
    start_datetime = datetime.strptime(sys.argv[1] + ' ' + sys.argv[2], '%Y-%m-%d %H:%M:%S.%f')
    end_datetime = datetime.strptime(sys.argv[3] + ' ' + sys.argv[4], '%Y-%m-%d %H:%M:%S.%f')
else:
    print "Incorrect Number of Arguments. Please use one of the following"
    print "python api_statistics.py"
    print "python api_statistics.py <Start Date YYYY-MM-DD> <Start Time HH:MM:SS>"
    print "python api_statistics.py <Start Date YYYY-MM-DD> <Start Time HH:MM:SS> <End Date YYYY-MM-DD> <End Time HH:MM:SS>"
print "Search from:  %s to %s" % (str(start_datetime), str(end_datetime))
#---------- Get all the log files within this directory and its subdirectories ---------
directory = "." # Starts with current api_statistics.py directory location
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


