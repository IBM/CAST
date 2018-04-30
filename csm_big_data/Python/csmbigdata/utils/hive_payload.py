#!/bin/python
# encoding: utf-8
#================================================================================
#
#   hive_payload.py
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
.. module::csmbigdata.utils.hive_payload
    :platform: Linux
    :synopsis: Maintains a cursor connection to a local or remote Hive instance.
    
    Hive will translate SQL-like commands into MapReduce jobs.
 .. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

# Modules for hive interaction.
from pyhive import hive
from TCLIService.ttypes import TOperationState

# Output helpers for standardized output. 
import csmbigdata.utils.output_helpers as output_helpers

# Logging resources.
import logging.config
logger=logging.getLogger(__name__)

class HiveConnection():
    ''' Defines a connection to a Hive server using pyHive, then abstracts functionality.'''

    def __init__(self):
        ''' Initializes a HiveConnection, connect must be called after initialization. '''
        
        logger.debug("Begin HiveConnection init")

        #: The cursor for use in the Hive connection.
        self.cursor = None    

        logger.debug("End HiveConnection init")

    def connect(self, host_name="localhost", port="10000", query_user="hdfs"):
        ''' Initializes a connection to the Hive resource specified in the function parameters.
        
        :param string host_name: The host of the targeted Hive instance. Default is "localhost".
        :param int port: The port of the targeted Hive instance. Default is 10000.
        :param string query_user: The user to execute the query as. Default is "hdfs".
        '''
        logger.debug("Enter HiveConnection connect")

        # TODO Error handling.
        self.cursor = hive.connect(host_name, port, query_user).cursor()

        logger.debug("Exit HiveConnection connect")
        

    def close(self):
        ''' Closes a connection to the Hive resource if present. '''
        logger.debug("Enter HiveConnection close")
        
        if self.cursor is not None:
            self.cursor.close()

        logger.debug("Exit HiveConnection close")


    def query(self, query, args, verbose=False):
        ''' Queries the the Hive server asyncronously, printing logs as 
            received (if specified verbose).

        :param string query: A hive query string. Replacements follow the pattern of
            **%(PARAM_NAME)[ds]** where PARAM_NAME corresponds to an element in the args dict.

        :param dict args: A dictionary containing the values to replace in the query.
            The key of an element in this dictionary should correspond to a replacement 
            specified in the query.
        
        :param bool verbose: Sets whether or not the cursor should retrieve the cursor log.
        
        :returns list: Returns a list of results, with each element being a row of 
            results from the database. The exact structure of this response is i
            dependent upon the query.
        '''
        logger.debug("Enter HiveConnection query")

        try:
            self.cursor.execute(query, args, async=True);
        except Exception,e:
            logger.error("Query could not be executed.")
            # FIXME Needs better output.
            logger.error(e)
            return None

        status = self.cursor.poll().operationState
            
        # TODO make this truly asynchronous.

        # While the query is running, poll the status and print logs if verbose.
        while status in (TOperationState.INITIALIZED_STATE, TOperationState.RUNNING_STATE):
            #  FIXME should there be a delay on this.
            
            if verbose:
                logs = self.cursor.fetch_logs()
                for message in logs:
                    output_helpers.eprint(message) 

            status = self.cursor.poll().operationState

        logger.debug("Exit HiveConnection query")
        return self.cursor.fetchall()

    def headers(self):
        ''' Retrieves the headers from the last query executed by this connection. 
        
        :returns list: A list of headers if they have been populated.
        :returns None: If no headers could be found None is returned instead.
        '''
        logger.debug("Enter HiveConnection headers")
        
        # EARLY RETURN if either the cursor or description is none
        if self.cursor is None or self.cursor.description is None:
            return None
    
        headers = []
        for column in self.cursor.description:
            headers.append(str(column[0]))

        logger.debug("Enter HiveConnection headers")
        return headers

