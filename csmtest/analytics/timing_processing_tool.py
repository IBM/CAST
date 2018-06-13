#================================================================================
#   
#    analytics/timing_processing_tool.py
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

#!/usr/bin/env python

import sys
import os

log_file = "/test/big_data/allocation.log"

# ----------------------------------------------------------------
# find_line will go through file line by line.  If the line
# contains the target string, find_line will add it to the output
# array.  Once the file has been fully traversed, the output array
# is returned
# ----------------------------------------------------------------
def find_line (target, file):
	output = []
	for line in open(file, "r"):
		if target in line:
			line = line.strip()
			output.append(line)
	return output
# ----------------------------------------------------------------
# parse_line will go through an array by index.  It will split
# each string in the array in to 3 parts delimited by "]".  The 
# string in index 3 of each split string (the time value) will be
# stripped of "[]" characters and added to the output array.  Once
# the input array has been fully traversed, the output array is
# returned
# ----------------------------------------------------------------
def parse_line (input):
	output = []
	for line in input:
		parts = line.split("]", 3)
		output.append(parts[3].strip("[]"))
	return output

# ----------------------------------------------------------------
# average will sum up the elements in a list and return that sum
# divided by the length of the list
# ----------------------------------------------------------------
def average (list):
	list_sum = 0
	result = 0
	# translate strings in list to float values
	temp = map(float, list)
	list_sum = sum(temp)
	result = list_sum / len(temp)
	return result

buckets = "[ALLOCATION]"
cases_apis = ["[A][CREATE]",
"[A][DELETE]", 
"[B][CREATE]", 
"[B][DELETE]", 
"[C][CREATE]",
"[C][QUERY]",
"[C][QUERY_ALL]", 
"[C][RESOURCES_QUERY]", 
"[C][DELETE]",
"[D][CREATE]",
"[D][UPDATE_STATE_RUNNING]",
"[D][UPDATE_STATE_STAGING_OUT]",
"[D][DELETE]",
"[D][UPDATE_HISTORY]",
"[E][STEP_BEGIN]",
"[E][QUERY_DETAILS]",
"[E][STEP_QUERY]",
"[E][STEP_QUERY_DETAILS]",
"[E][STEP_QUERY_ACTIVE]",
"[E][STEP_END]"]

target_list = []
for str in cases_apis:
	full_str = buckets + str
	target_list.append(full_str)

for target in target_list:
	result_list = find_line(target, log_file)
	result_list = parse_line(result_list)
	result = average(result_list)
	print("Average for %s: %s" % (target, result))
