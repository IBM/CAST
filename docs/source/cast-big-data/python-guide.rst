Python Guide
============


.. contents::
   :local:


Big Data Use Cases
------------------

CAST offers a collection of use case scripts designed to interact with the Big Data Store through
the elasticsearch interface.

findJobTimeRange.py
^^^^^^^^^^^^^^^^^^^

This use case may be considered a building block for the remaining ones. This use case demonstrates
the use of the `cast-allocation` transactional index to get the time range of a job.

The usage of this use case is described by the `--help` option.

.. TODO add help output.

findJobKeys.py
^^^^^^^^^^^^^^

This use case represents two comingled use cases. First when supplied a job identifier (allocation
id or job id) and a keyword (regular expression case insensitive) the script will generate a 
listing of keywords and their occurrence rates on records associated with the supplied job.
Association is filtered on by the time range of the jobs and hostnames that participated on the job.

A secondary usecase is presented in the verbose flag, allowing the user to see a list of 
all entries matching the keyword.

.. code-block:: none 

    usage: findJobKeys.py [-h] [-a int] [-j int] [-s int] [-t hostname:port]
                          [-k [key [key ...]]] [-v] [--size size]
                          [-H [host [host ...]]]
    
    A tool for finding keywords during the run time of a job.
    
    optional arguments:
      -h, --help            show this help message and exit
      -a int, --allocationid int
                            The allocation ID of the job.
      -j int, --jobid int   The job ID of the job.
      -s int, --jobidsecondary int
                            The secondary job ID of the job (default : 0).
      -t hostname:port, --target hostname:port
                            An Elasticsearch server to be queried. This defaults
                            to the contents of environment variable
                            "CAST_ELASTIC".
      -k [key [key ...]], --keywords [key [key ...]]
                            A list of keywords to search for in the Big Data
                            Store. Case insensitive regular expressions (default :
                            .*). If your keyword is a phrase (e.g. "xid 13")
                            regular expressions are not supported at this time.
      -v, --verbose         Displays any logs that matched the keyword search.
      --size size           The number of results to be returned. (default=30)
      -H [host [host ...]], --hostnames [host [host ...]]
                            A list of hostnames to filter the results to

findJobsRunning.py
^^^^^^^^^^^^^^^^^^

A use case for finding all jobs running at the supplied timestamp. This usecase will display a 
list of jobs for which the start time is less than the supplied time and have either no end time
or an end time greater than the supplied time.

.. code-block:: none

    usage: findJobsRunning.py [-h] [-t hostname:port] [-T YYYY-MM-DD HH:MM:SS]
                              [-s size] [-H [host [host ...]]]
    
    A tool for finding jobs running at the specified time.
    
    optional arguments:
      -h, --help            show this help message and exit
      -t hostname:port, --target hostname:port
                            An Elasticsearch server to be queried. This defaults
                            to the contents of environment variable
                            "CAST_ELASTIC".
      -T YYYY-MM-DD HH:MM:SS, --time YYYY-MM-DD HH:MM:SS
                            A timestamp representing a point in time to search for
                            all running CSM Jobs. HH, MM, SS are optional, if not
                            set they will be initialized to 0. (default=now)
      -s size, --size size  The number of results to be returned. (default=1000)
      -H [host [host ...]], --hostnames [host [host ...]]
                            A list of hostnames to filter the results to.


findJobMetrics.py
^^^^^^^^^^^^^^^^^

Leverages the built in Elasticsearch statistics functionality. Takes a list of fields and a job
identifier then computes the *min*, *max*, *average*, and *standard deviation* of those fields. The
calculations are computed against all records for the field during the running time of the job
on the nodes that participated.

This use case also has the ability to generate correlations between the fields specified.

.. code-block:: none
   
    usage: findJobMetrics.py [-h] [-a int] [-j int] [-s int] [-t hostname:port]
                             [-H [host [host ...]]] [-f [field [field ...]]]
                             [-i index] [--correlation]
    
    A tool for finding metrics about the nodes participating in the supplied job
    id.
    
    optional arguments:
      -h, --help            show this help message and exit
      -a int, --allocationid int
                            The allocation ID of the job.
      -j int, --jobid int   The job ID of the job.
      -s int, --jobidsecondary int
                            The secondary job ID of the job (default : 0).
      -t hostname:port, --target hostname:port
                            An Elasticsearch server to be queried. This defaults
                            to the contents of environment variable
                            "CAST_ELASTIC".
      -H [host [host ...]], --hostnames [host [host ...]]
                            A list of hostnames to filter the results to.
      -f [field [field ...]], --fields [field [field ...]]
                            A list of fields to retrieve metrics for (REQUIRED).
      -i index, --index index
                            The index to query for metrics records.
      --correlation         Displays the correlation between the supplied fields
                            over the job run. 

findUserJobs.py
^^^^^^^^^^^^^^^

Retrieves a list of all jobs that the the supplied user owned. This list can be filtered to
a time range or on the state of the allocation. If the `--commonnodes` argument is supplied a 
list nodes will be displayed where the node participated in more nodes than the supplied threshold.
The colliding nodes will be sorted by number of jobs they participated in.

.. code-block:: none
    
    usage: findUserJobs.py [-h] [-u username] [-U userid] [--size size]
                           [--state state] [--starttime YYYY-MM-DD HH:MM:SS]
                           [--endtime YYYY-MM-DD HH:MM:SS]
                           [--commonnodes threshold] [-v] [-t hostname:port]
    
    A tool for finding a list of the supplied user's jobs.
    
    optional arguments:
      -h, --help            show this help message and exit
      -u username, --user username
                            The user name to perform the query on, either this or
                            -U must be set.
      -U userid, --userid userid
                            The user id to perform the query on, either this or -u
                            must be set.
      --size size           The number of results to be returned. (default=1000)
      --state state         Searches for jobs matching the supplied state.
      --starttime YYYY-MM-DD HH:MM:SS
                            A timestamp representing the beginning of the absolute
                            range to look for failed jobs, if not set no lower
                            bound will be imposed on the search.
      --endtime YYYY-MM-DD HH:MM:SS
                            A timestamp representing the ending of the absolute
                            range to look for failed jobs, if not set no upper
                            bound will be imposed on the search.
      --commonnodes threshold
                            Displays a list of nodes that the user jobs had in
                            common if set. Only nodes with collisions exceeding
                            the threshold are shown. (Default: -1)
      -v, --verbose         Displays all retrieved fields from the `cast-
                            allocation` index.
      -t hostname:port, --target hostname:port
                            An Elasticsearch server to be queried. This defaults
                            to the contents of environment variable
                            "CAST_ELASTIC".

findWeightedErrors.py
^^^^^^^^^^^^^^^^^^^^^

An extension of the `findJobKeys.py`_ use case. This use case will query elasticsearch for a job
then run a predefined collection of mappings to assist in debugging a problem with the job.


.. code-block:: none 
   
    usage: findWeightedErrors.py [-h] [-a int] [-j int] [-s int]
                                 [-t hostname:port] [-k [key [key ...]]] [-v]
                                 [--size size] [-H [host [host ...]]]
                                 [--errormap file]

    A tool which takes a weighted listing of keyword searches and presents
    aggregations of this data to the user.
    
    optional arguments:
      -h, --help            show this help message and exit
      -a int, --allocationid int
                            The allocation ID of the job.
      -j int, --jobid int   The job ID of the job.
      -s int, --jobidsecondary int
                            The secondary job ID of the job (default : 0).
      -t hostname:port, --target hostname:port
                            An Elasticsearch server to be queried. This defaults
                            to the contents of environment variable
                            "CAST_ELASTIC".
      -v, --verbose         Displays the top --size logs matching the --errormap mappings.
      --size size           The number of results to be returned. (default=10)
      -H [host [host ...]], --hostnames [host [host ...]]
                            A list of hostnames to filter the results to.
      --errormap file       A map of errors to scan the user jobs for, including
                            weights.


JSON Mapping Format
+++++++++++++++++++

This use case utilizes a JSON mapping to define a collection of keywords and values to query 
the elasticsearch cluster for. These values can leverage the native elasticsearch boost feature
to apply weights to the mappings allowing a user to quickly determine high priority items using 
scoring.

The format is defined as follows:

.. code-block:: json

    [
        {
            "category" : "A category, used for tagging the search in output. (Required)",
            "index"    : "Matches an index on the elasticsearch cluster, uses elasticsearch syntax. (Required)",
            "source"   : "The hostname source in the index.",
            "mapping" : [
                {
                    "field" : "The field in the index to check against(Required)",
                    "value" : "A value to query for; can be a phrase, regex or number. (Required)",
                    "boost" : "The elasticsearch boost factor, may be thought of as a weight. (Required)",
                    "threshold" : "A range comparison operator: 'gte', 'gt', 'lte', 'lt'. (Optional)"
                }
            ]
        }
    ]


When applied to a real configuration a mapping file will look something like this:

.. code-block:: json

    [
        {
            "index"   : "*syslog*",
            "source"  : "hostname",
            "category": "Syslog Errors" ,
            "mapping" : [
                {
                    "field" : "message",
                    "value" : "error",
                    "boost" : 50
                },
                {
                    "field" : "message",
                    "value" : "kdump",
                    "boost" : 60
                },
                {
                    "field" : "message",
                    "value" : "kernel",
                    "boost" : 10
                }
            ]
        },
        {
            "index"    : "cast-zimon*",
            "source"   : "source",
            "category" : "Zimon Counters",
            "mapping"  : [
                {
                    "field"     : "data.mem_active",
                    "value"     : 12000000,
                    "boost"     : 100,
                    "threshold" : "gte"
                },
                {
                    "field"     : "data.cpu_system",
                    "value"     : 10,
                    "boost"     : 200,
                    "threshold" : "gte"
                }
    
            ]
        }
    ]

.. note:: The above configuration was designed for demonstrative purposes, it is recommended 
    that users create their own mappings based on this example.


UFM Collector
-------------

A tool interacting with the UFM collector is provided in `ibm-csm-bds-*.noarch.rpm`.
This script performs 3 key operations:

1. Connects to the UFM monitoring snapshot RESTful interface.
    * This connection specifies a collection attributes and functions to execute against the 
        interface.

2. Processes and enriches the output of the REST connection.
    * Adds a `type`, `timestamp` and `source` field to the root of the JSON document.

3. Opens a socket to a target logstash instance and writes the payload.

.. _Elastic Tests: https://github.com/IBM/CAST/tree/master/csm_big_data/Python/elastic_tests


