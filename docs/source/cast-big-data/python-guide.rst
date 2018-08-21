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

.. TODO add help output.

findJobsRunning.py
^^^^^^^^^^^^^^^^^^

A use case for finding all jobs running at the supplied timestamp. This usecase will display a 
list of jobs for which the start time is less than the supplied time and have either no end time
or an end time greater than the supplied time.

.. TODO add help output.

findJobMetrics.py
^^^^^^^^^^^^^^^^^

Leverages the built in Elasticsearch statistics functionality. Takes a list of fields and a job
identifier then computes the *min*, *max*, *average*, and *standard deviation* of those fields. The
calculations are computed against all records for the field during the running time of the job
on the nodes that participated.

This use case also has the ability to generate correlations between the fields specified.

.. TODO add help output.

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


