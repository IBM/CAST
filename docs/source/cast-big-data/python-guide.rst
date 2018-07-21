Python Guide
============

Some `Elastic Tests`_ are being worked on in the IBM CAST repository.

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


