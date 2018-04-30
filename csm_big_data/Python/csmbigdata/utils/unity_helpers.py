#!/bin/python
# encoding: utf-8
# ================================================================================
#
#   unity_helpers.py
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
# ===============================================================================
'''
.. module:: csmbigdata.utils.unity_helpers
    :platform: Linux
    :synopsis: Helper modules for common operations that interact with LogAnalysis.
.. moduleauthor:: John Dunham (jdunham@us.ibm.com)
'''

import csmbigdata.utils.csmi_tool as csmi_tool

from csmbigdata.utils.unity_payload import UnityResponse, UnityPayload

import json
import re

# Includes some helper functions to ensure that output is consistent across tools.
import csmbigdata.utils.output_helpers as output_helpers

# Include the various date modules.
import dateutil.parser
from datetime import *
from dateutil.relativedelta import *

import logging.config
logger=logging.getLogger(__name__)

#: The Big Data Store query for finding the creation of an allocation/job begin.
BDS_START_QUERY = '%s create: primary_job_id=%s secondary_job_id=%s'

#: The Big Data Store query for finding the deletion of an allocation/job end.
BDS_END_QUERY   = '%s delete: primary_job_id=%s secondary_job_id=%s'

#: Allocation query that retrieves both the create and delete strings from the BDS
BDS_ALLOCATION_QUERY = 'message:(Allocation AND (create OR delete) AND primary_job_id AND secondary_job_id)'

#: A regex extractor for extracting allocation_id, action [create|delete], primary_job_id, and secondary_job_id.
BDS_ALLOCATION_EXTRACTOR = 'A.{10}([0-9]*) (.{6}): p.{13}=([0-9]*) s.{15}=([0-9]*)'

#: Allocation message starting key.
BDS_ALLOCATION_BEGIN_KEY = "create"

#: Allocation message ending key.
BDS_ALLOCATION_END_KEY   = "delete"

def find_job_time_range_csm(
    payload, 
    job_id, 
    secondary_job_id      = 0, 
    target_hostnames      = None,
    target_date           = None,
    target_date_range     = 7,
    use_local_maximum     = False ):
    '''Determines the range of time that a job was run and when the allocation ran on
        the nodes that participated. First gets the allocation from the csm api, then
        queries the big data store for the node time ranges. If the csm api query fails,
        allocation details will be pulled from the Big Data Store connection (if target date
        is specified)

        :param UnityPayload payload: A Unity Payload object with a connection to a 
            Log Analysis server.
        :param int job_id: The primary job id to retrieve the execution time range for.
        :param int secondary_job_id: The secondary job id to retrieve the execution 
            time range for.
        :param int target_hostnames: A collection of hostnames to filter the resultes of 
            the job_id_map to.
        :param datetime target_date: A fallback datetime for when a start date is missing 
            from the allocation (this should almost never happen). If this function falls 
            back to BDS this is the the middle most date.
        :param int target_date_range: The number of days to search for an end time after 
            the start/target date if no end time is found in the allocation. This is 
            centered on the target_date if used.
        :param Boolean use_local_maximum: If set the start and end time will be the 
            max and min times found in the local hostnames.

        :return (dict, datetime, datetime): 
            A dict mapping { hostname => { start => datetime, end => datetime } },
            the job start time, 
            the job end time.


        :raises ValueError: If the csm_allocation_query fails and there was no target_date 
            specified. Additionally, if the target_date wasn't specified in the 
            allocation_dictionary response from allocation_query.
    '''
    logger.debug("Enter find_time_range_csm")
    
    # FIXME Should this become static at some point?
    csm_tool = csmi_tool.CSMITool()

    # This will bubble up a ValueError
    try:
        allocation_dict = csm_tool.allocation_query(
            job_id       = job_id,
            secondary_id = secondary_job_id )
    except Exception as e:
        logger.info("Allocation Query Failed: \n%s\n%s\n%s", "="*30,e,"="*30 )
        logger.info("Falling back on Big Data Store")

        allocation_dict = None
        
        # Modify the local maximum, as the start and end times will be invalid.
        use_local_maximum = True
        if target_date is None:
            logger.debug("Exit find_time_range_csm")
            raise ValueError("target_date was not specified, unable to fall back to Big Data Store")

    # Populate Secondary values        
    if allocation_dict is not None:
        history_dict    = allocation_dict.get(csmi_tool.CSM_AL_HISTORY)
        allocation_id   = allocation_dict.get(csmi_tool.CSM_AL_AID)
        found_hostnames = allocation_dict.get(csmi_tool.CSM_AL_COMPUTE_NODES)
    else:
        history_dict    = None
        allocation_id   = ""
        found_hostnames = None 

    # Determine the starting time.
    if allocation_dict is not None and csmi_tool.CSM_AL_BEGIN_TIME in allocation_dict:
        job_start_time = allocation_dict[csmi_tool.CSM_AL_BEGIN_TIME]
        logger.debug("job_start_time found in allocation: %s", job_start_time)

    elif target_date is not None:
        job_start_time = target_date - relativedelta(days=target_date_range/2.0)
        logger.debug("job_start_time not found in allocation, falling back on" +\
            " target_datetime: %s", job_start_time)
    else:
        logger.debug("Exit find_time_range_csm")
        raise ValueError('find_time_range could not find a starting time in' +\
            ' the allocation and no target_date was supplied as a fall back!')
    
    # Find the end time in the allocation dictionaary
    if history_dict is not None and csmi_tool.CSM_AL_END_TIME in history_dict:
        job_end_time   = history_dict[csmi_tool.CSM_AL_END_TIME]
        logger.debug("job_end_time found in allocation: %s", job_end_time)

    else:
        job_end_time = job_start_time + relativedelta(days=target_date_range)

        max_date = datetime.now(job_end_time.tzinfo)
        if job_end_time > max_date:
            job_end_time = max_date
        
        logger.debug("job_end_time not found in allocation, estimating end time " +\
            "with target_date_range: %s", job_end_time )

    # Communicate with the Big Data Store. 
    job_id_map, job_time_min, job_time_max = find_job_time_range_bds(
        payload,
        job_id,
        secondary_job_id,
        allocation_id,
        found_hostnames,
        target_hostnames,
        job_start_time,
        job_end_time )

    logger.debug("Exit find_time_range_csm")
    
    # Return based on wheter or not the local maximum was requested.
    if use_local_maximum:
        return job_id_map, job_time_min, job_time_max
    else:
        return job_id_map, job_start_time, job_end_time

def find_job_time_range_bds(
    payload,
    job_id,
    secondary_job_id      = 0,
    allocation_id         = "",
    found_hostnames       = None,
    target_hostnames      = None,
    job_start_time        = None,
    job_end_time          = None):
    '''Searches over the supplied time range (job_start_time => job_end_time) on
        the spcified hostnames (found_hostnames) for the start and end times of
        the specified jobid. Returns a tuple containing the job_id_map and the 
        min and max time.

        :param UnityPayload payload: A Unity Payload object with a connection to a Log Analysis server.
        :param int job_id: The primary job id to retrieve the execution time range for.
        :param int secondary_job_id: The secondary job id to retrieve the execution time range for.
        :param int allocation_id: The allocation id of the job, this is not necessary, just provides further specification.
        :param list found_hostnames: A collection of hostnames to filter the resultes of the job_id_map to.
        :param list target_hostnames: A collection of hostnames to filter the resultes of the job_id_map to. This is used by find_job_time_range_csm, not recommended for general use.
        :param datetime job_start_time: The start of the time range to search for the job.
        :param datetime job_end_time: The end of the time range to search for the job.

        :return (dict, datetime, datetime): 
            A dict mapping { hostname => { start => datetime, end => datetime } }, 
            the job start time, 
            the job end time.

        :return (None, None, None): The jobid could not be found within the supplied constraints.
        :return ({..}, {..}, None): The jobid was found, but the end time could not be found.
        :return ({..}, None, {..}): The jobid was found, but the start time could not be found.

    '''
    logger.debug("Enter find_time_range_bds")

    # Initialize the job_id_map
    job_id_map = dict()
    
    if found_hostnames is not None:
        for hostname in found_hostnames:
            if target_hostnames is None or hostname in target_hostnames:
                job_id_map[hostname] = {
                    "start" : job_start_time,
                    "end"   : job_end_time }

    # Build the payload.
    payload.initialize()
    payload.set_getAttributes( ["timestamp","syslogHostname","message"] )
    payload.set_logsources("logSource", "/syslog")
    payload.set_range_timestamp_filter( job_start_time, job_end_time )

    start_query = BDS_START_QUERY % (allocation_id, job_id, secondary_job_id)
    end_query   = BDS_END_QUERY   % (allocation_id, job_id, secondary_job_id)
    payload.set_query( 'message:"%s" OR message:"%s"' % ( start_query, end_query ) )

    # If the job map was populated, filter on that subset,
    # Else filter on the targeted hostnames if present.
    if len(job_id_map.keys()) > 0:
        payload.set_query( 
            payload.scala_host_query( "syslogHostname", job_id_map.keys() ), 
            "AND" )
    elif target_hostnames is not None:
        payload.set_query(
            payload.scala_host_query( "syslogHostname", target_hostnames ),
            "AND" )
    
    # Track the overall start and end time of the job.
    job_time_min = None
    job_time_max = None
    
    while payload.get_start() >= 0:
        json_response = json.loads ( payload.post() )
        
        logger.info("Processing results: %d->%d out of %s",
            payload.get_start(),
            payload.get_start() + payload.get_results(),
            json_response.get("totalResults"))

        payload.set_start( payload.determine_start( json_response ) )
                
        # If an error was found report it and exit.
        if "result" in json_response and\
            json_response["result"].get("status") == "failure":

            logger.error("Error occured in communication: %s",
                json_response["result"].get("message"))
            continue

        # If the search results were not present continue.
        if UnityResponse.SEARCH_RESULTS not in json_response:
            logger.warning("json_response was empty");
            continue     

        # Assumes that this is run only against the syslog log source
        for entry in json_response[UnityResponse.SEARCH_RESULTS]:
            
            # If the attributes were not present continue.
            if UnityResponse.ATTRIBUTES not in entry:
                continue

            hostname   = entry[UnityResponse.ATTRIBUTES]["syslogHostname"]
            message    = entry[UnityResponse.ATTRIBUTES]["message"]

            # Get the timezone.
            last_index = entry[UnityResponse.ATTRIBUTES]["timestamp"].rfind(':')
            date_str   = entry[UnityResponse.ATTRIBUTES]["timestamp"][:last_index] + \
                "." +  entry[UnityResponse.ATTRIBUTES]["timestamp"][last_index+1:]
            event_time = dateutil.parser.parse(date_str)

            search_result  = re.search( BDS_ALLOCATION_EXTRACTOR, message )
        
            # This is a sanity check.
            if search_result is None:
                logger.debug("Message didn't have allocation details.")
                continue
        
            alloc_id, al_type, job_id, sec_id = search_result.group(1,2,3,4)

            # Determine if this is a start or end time.
            if al_type == BDS_ALLOCATION_BEGIN_KEY:
                if hostname not in job_id_map:
                    job_id_map[hostname] = {"start" : None, "end" : None}

                job_id_map[hostname]["start"] = event_time
                if job_time_min is None or event_time < job_time_min:
                    job_time_min = event_time

                logger.debug("Start time found for %s", hostname)

            elif al_type == BDS_ALLOCATION_END_KEY:
                if hostname not in job_id_map:
                    job_id_map[hostname] = {"start" : None, "end" : None}

                job_id_map[hostname]["end"]   = event_time
                if job_time_max is None or event_time > job_time_max:
                    job_time_max = event_time

                logger.debug("End time found for %s", hostname)

    logger.debug("Exit find_time_range_bds")

    # Set the job times to the supplied start and end if no local ones were found.
    if job_time_min is None:
        job_time_min = job_start_time

    if job_time_max is None:
        job_time_max = job_end_time

    # If no local maxima were found set the job_id_map to none.
    # These are the failure states.
    if job_time_min is None:
        if job_time_max is None:
            job_id_map = None
        else:
            logger.debug("No start time was found for %d", job_id)
            # TODO React to there being no start time found!
    elif job_time_max is None:
        logger.debug("No end time was found for %d", job_id)
        # TODO Determine if the job is still running!
        
    logger.debug("Exit find_time_range_bds")
    return job_id_map, job_time_min, job_time_max


def print_job_time_range( job_id_map, job_id, start_time, end_time ):
    ''' Prints the time range of the output of both find_job_time_range_csm and find_job_time_range_bds.

        :param dict job_id_map: The dictionary mapping of 
            { hostname : { start : datetime, end : datetime } }
        :param datetime job_id: The id of the job the mapping is correlated to.
        :param datetime start_time: The starting time of the job.
        :param datetime end_time: The ending time of the job.

        :returns 0: Success
        :returns 1: job_id_map was None
    '''
    logger.debug("Enter print_job_time_range")

    if job_id_map is None:
        logger.debug("Exit print_job_time_range")
        return 1
    
    # Compute the columns:
    base_host_width=8
    host_width=base_host_width
    time_spacing = 21
    for node in job_id_map:
        host_width = max(host_width, len(node))
    host_width+=2
    div_width=host_width + (time_spacing * 2)

    print("JobID {0} found {1} on hosts.".format(str(job_id), str(len(job_id_map))))
    print("JobID {0} ran from \n{1} to {2}\n".format( 
        str(job_id), 
        output_helpers.format_timestamp(start_time),
        output_helpers.format_timestamp(end_time)))

    print("Hostname{0}Start Time{1}End Time".format(
        " "*(host_width - base_host_width), 
        " " * (time_spacing- len("Start Time"))))

    print("-"*div_width)

    for node in job_id_map:
        print(node + "  " + 
            output_helpers.format_timestamp(job_id_map[node].get("start")) + "  " + 
            output_helpers.format_timestamp(job_id_map[node].get("end")))
    
    logger.debug("Exit print_job_time_range")
    return 0

