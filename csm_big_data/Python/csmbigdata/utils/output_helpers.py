#!/bin/python
# encoding: utf-8
#================================================================================
#
#   output_helpers.py
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================
'''
.. module::csmbigdata.utils.output_helpers
    :platform: Linux
    :synopsis: A collection of standardized functions to assist in formatting data.
 .. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
from __future__ import print_function
import sys

from dateutil import tz

import logging.config
logger=logging.getLogger(__name__)

# Formatting constants.
DIV_0 = '=' * 80
DIV_1 = '-' * 80
DIV_2 = '=' * 40
DIV_3 = '@' * 90

'''The tool wide format for timestamps.'''
TIME_FORMAT = "%Y-%m-%d %H:%M:%S"

''' The timezone of the invoking machine.'''
TIME_ZONE    = tz.tzlocal()

def eprint(*args, **kwargs):
    ''' Alias to a version of the print function that redirects to stderr. '''
    print(*args, file=sys.stderr, **kwargs)

def csv_row( row ):
    '''Creates a csv row for a table.

    :param list row: A list of items to convert into a comma separated string.
    
    :returns string: The list as a string with its values separated by commas.
    '''
    return ",".join( '"{}"'.format(str(field)) for field in row)

def make_csv_file(header, rows, output_file, output_to_screen=False, screen_max_rows=30):
    ''' Makes a csv file in the file location specified by the user.

    :param list header: A list of header string values. If the header is set to None, 
        the `rows` will be the only contents of the file.
    :param list<list> rows: A list of rows(list) that make up the body of the file. Cannot
        be set to None.
    :param boolean output_to_screen: 
        :True: Display the first `screen_max_rows` rows and the header of the csv file to 
            the screen as stdout.
        :False: Do not display any of the file contents to the screen.
    :param int screen_max_rows: The maximum number of rows to output to the screen.

    :returns 0: The file was created successfully.
    :returns 1: The file was unable to be created.
    '''
    logger.debug("Enter make_csv_file")
    
    # EARLY RETURN if file was not set and rows were not found
    if rows is None or output_file is None:
        logger.error("Could not make the csv file")  

        if rows is None:
            logger.error("No rows were supplied for the csv file")
        if output_file is None:
            logger.error("Output file was not specified")
        

        logger.debug("Exit make_csv_file")
        return 1
    csv_file = open(output_file, 'w')
    
    # EARLY Return if the file couldn't be opened for writing.
    if csv_file is None:
        logger.error("Could not open '" + output_file + "'")
        logger.debug("Exit make_csv_file")
        return 1
    eprint("Building csv file: " + output_file)

    # Make the header string.
    if header is not None:
        header_str = csv_row(header)
        csv_file.write(header_str + "\n" )
        if output_to_screen:
            print("\n\t" + header_str)
            eprint("\t" + DIV_0)
    
    # Output the rows to the file and/or screen.
    row_count=0
    for row in rows:
        row_str = csv_row(row)
        csv_file.write(row_str + "\n")
        
        row_count+=1
        if output_to_screen and row_count < screen_max_rows:
            print("\t" + row_str)
    
    # Write the accounting to the screen.
    if output_to_screen:
        eprint("\t... {0} rows omitted.".format(max(0,row_count-screen_max_rows)))
    print('\nWrote {0} rows to csv file "{1}"'.format(row_count, output_file))

    # Close the file handle
    csv_file.close()
    
    logger.debug("Exit make_csv_file")
    return 0

def format_timestamp(time, set_tz_loc=False):
    ''' Formats the timestamp for printing. If the timestamp can't be formatted, 
    or is a string returns 'Timestamp-None'

    :param datetime time: The datetime object to format.
    :param boolean tz_loc: If set the timestamp will be given the local timezone.
    :returns string: A formatted timestamp string. If the time could not be formatted, 
        'TIMESTAMP-NONE' is returned instead.
    '''
    logger.debug("Enter format_timestamp")
    # EARLY RETURN if time is None.
    if time is None or type(time) is str:
        return '''TIMESTAMP-NONE'''
    
    # If desired set the timestamp to be local time.
    if set_tz_loc:
        # If the time has no timestamp give it one,
        # Else coerce the timestamp to local time.
        if time.tzinfo is None:
            time = time.rplace(tzinfo=TIME_ZONE)
        else:
            time = time.astimezone(TIME_ZONE)

    logger.debug("Exit format_timestamp")
    return time.strftime( TIME_FORMAT )
