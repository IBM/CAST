#!/bin/python
# encoding: utf-8
#================================================================================
#
#   csmi_tool.py
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
.. module:: csmbigdata.utils.csmi_tool
    :platform: Linux
    :synopsis: A module for wrapping the csm api commandline to python.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''
import os
import subprocess
import yaml

''' Used for the main test, remove in production. '''
import sys

#: Allocation id dictionary key for an allocation.
CSM_AL_AID           = 'allocation_id'

#: Begin time dictionary key for an allocation.
CSM_AL_BEGIN_TIME    = 'begin_time'

#: The alocation's history, contains end time, exit status and consumed energy.
CSM_AL_HISTORY       = 'history'

#: End time dictionary key for an allocation.
CSM_AL_END_TIME      = 'end_time'

#: The exit status of the overall allocation.
CSM_AL_EXIT_STATUS   = 'exit_status'
    
#: The energy consumption of the allocation.
CSM_AL_ENERGY	     = 'energy_consumed'

#: Compute node array key for an allocation.
CSM_AL_COMPUTE_NODES = 'compute_nodes'

class CSMITool(object):
    ''' A class collecting wrappers for the various csm api command line operations. Ensures the environment is properly set up for using the csm api.
    '''

    #: The root of the csm commandline directory.
    #:
    #: | **WARNING IF CSM IS INSTALLED ELSEWHERE THIS TOOL DOES NOT WORK!**
    CSM_HOME = '''/opt/ibm/csm/bin/'''
    #CSM_HOME = '''/u/jdunham/bluecoral/bluecoral/work/csm/bin/'''

    # FIXME if this changes there will be trouble!
    #: The CSM Socket to connect to.
    #CSM_SSOCKET = '''/run/csmd.sock'''

    #: csm_allocation_query executable name.
    CSM_AL_QUERY  = "csm_allocation_query"
    
    #: csm_node_attributes_executable name.
    CSM_ATT_QUERY = "csm_node_attributes_query"
        
    def __init__(self):
        ''' Sets up the csmi environment. '''
        # Sets up the environment for executing the queries.
        #os.environ['CSM_SSOCKET'] = CSMI_Tool.CSM_SSOCKET
        self.set_csm_home(CSMITool.CSM_HOME)
	
    def set_csm_home(self, csmi_home):
        ''' Sets location of the csm commandline executables.

        :param csmi_home: The absolute path to the bin directory of the csm command
            line interfaces.
        '''
        self.csmi_home = csmi_home
        self.alloc_query = csmi_home + CSMITool.CSM_AL_QUERY
        self.attrib_query = csmi_home + CSMITool.CSM_ATT_QUERY
        

    #TODO Make a function for opening the subprocess?
    def allocation_query(self, allocation_id=None, job_id=None, secondary_id=0):
        ''' Executes the csm_allocation_query via a subprocess and converts the output to a python usable format.

        +---------------------------------------------------------------------+
        |Commonly used allocation details                                     |
        +-------------+--------+----------------------------------------------+
        |Detail Key   |Type    |Description                                   |
        +=============+========+==============================================+
        |compute_nodes|(list)  |List of nodes participating in the allocation.|
        +-------------+--------+----------------------------------------------+
        |begin_time   |(string)|RFC 2822 time format.                         |
        +-------------+--------+----------------------------------------------+
        |end_time     |(string)|RFC 2822 time format.                         |
        +-------------+--------+----------------------------------------------+
        |state        |(string)|The current alloction state.                  |
        +-------------+--------+----------------------------------------------+ 



        :param int allocation_id: The id of the allocation.
        :param int job_id: The id of the job, takes precedence over allocation_id.
        :param int secondary_id: The secondary id of the job, 0 there is no 
            secondary id to be found.
        :returns dict: The details of the allocation. 

        :raises ValueError: If the job id or allocation id could not be 
            found in the query.
        '''
        # EARLY RETURN
        # Build the appropriate query.
        if job_id is not None:
            query = [ self.alloc_query, "--primary_job_id", str(job_id), 
                "--secondary_job_id", str(secondary_id) ]

        elif allocation_id is not None:
            query = [ self.alloc_query, "--allocation_id", str(allocation_id) ]    
        else:
            return None

        # Execute the allocation query.
        process = subprocess.Popen(
            query,
            env=os.environ,
            stdout=subprocess.PIPE
        ) 
	
        # Parse the output of the command line.
        output     = process.stdout.read()

        try:
            query_data = yaml.load(output)
            
            # Verify the allocation id was specified.
            if query_data.get(CSM_AL_AID) is None:
                raise ValueError()
        except :
            if job_id is not None:
                error = "JobID: {0} could not be found in the CSM Database".format(job_id)
            else:
                error = "AllocationID: {0} could not be found in the CSM Database".format(allocation_id)
            error += "\n" + output

            raise ValueError(error)

        return query_data
    
    def node_attributes_query(self, node_name):
        ''' Executes the csm_node_attributes_query via a subprocess and converts the output to a python usable format.
    
        :param string node_name: The name of the node attributes are requested of.
        
        :returns dict:
             A dict containing the attributes of the node. 
            If the command fails this will be a dictionary with 
            one key-value pair: {'Total_Records': 0}
        '''
        process = subprocess.Popen(
            [ self.attrib_query, str(node_name) ],
            env=os.environ,
            stdout=subprocess.PIPE
        ) 
        
        output     = process.stdout.read()
        query_data = yaml.load(output)
        return query_data
