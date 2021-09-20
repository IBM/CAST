.. _CASTBDSCommon:

Common Big Data Store Problems
==============================

The following document outlines some common sources of error for the Big Data Store and how to best
resolve the described issues.

.. contents::
    :local:


Beats Not Starting
------------------

There was a typo in a previous version of CAST. The field "close_removed" is a bool in the ELK config. This typo caused beats to not start up correctly. The CAST team has updated the config file to address this issue. 

.. _Logstash_Not_Starting:

Logstash Not Starting
---------------------

In ELK 7.5.1, Logstash may not start and run on Power, due to an arch issue. 

.. code-block:: none

    [2019-05-03T10:41:38,701][ERROR][org.logstash.Logstash    ] 
    java.lang.IllegalStateException: Logstash stopped processing because of an error: 
    (LoadError) load error: ffi/ffi -- java.lang.NullPointerException: null


The CAST team was able to trace the bug to `jruby/lib/ruby/stdlib/ffi/platform/powerpc64-linux/`. It looks as though the platform.conf file was not created for this platform. Copying the types.conf file to platform.conf appears to resolve the problem.
 
GitHub Issue: https://github.com/elastic/logstash/issues/10755

IBM and the CAST team have made a script to fix this packaging issue. 

The patch can be found in the CAST repo at: https://github.com/IBM/CAST/blob/master/csm_big_data/logstash/csm_logstash_patch_jruby_9.2.8.sh and in the install dir at: ``/opt/ibm/csm/bigdata/logstash/csm_logstash_patch_jruby_9.2.8.sh``.

Run this patch before starting Logstash. 

Timestamps
----------

Timestamps are generally the number one source of problems in the ELK Stack. This is due to
a wide variety of precisions and timestamp formats that may come from different data sources.

Elasticsearch will try its best to parse dates, as outlined in the `ELK Date`_ documentation.
If a date doesn't match the default formats (a usual culprit is epoch time or microseconds) 
the administrator will need to take action.

CAST has two prescribed resolution patterns for this problem:

1. `Fixing Timestamps in Elasticsearch`_
2. `Fixing Timestamps in Logstash`_

The administrator may apply one or more resolution patterns to resolve the issue.

.. attention:: Timestamps created by CSM will **generally** attempt to ship timestamps in the 
    correct format, however, Elasticsearch will only automatically parse up to millisecond.
    The default ISO 8601 format of Postgresql has precision up to microseconds, requiring
    postgres generated timestamps to use a parsing strategy.

.. note:: If any indices have been populated with data not interpreted as dates, those 
    indices will need to be reindexed.

Fixing Timestamps in Elasticsearch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is the preferred methodology for resolving issues in the timestamp. CAST supplies 
a utility in |csm-bds| for generating mappings that fix the timestamps in 
data sources outlined in :ref:`CASTDataAgg`.

The index mapping script is present at `/opt/ibm/csm/bigdata/elasticsearch/createIndices.sh`.
When executed the script will make a request to the Elasticsearch server (determined by 
the input to the script) which creates all of the mappings defined in the
`/opt/ibm/csm/bigdata/elasticsearch/templates` directory. If the user wishes to clear existing 
templates/mappings the `/opt/ibm/csm/bigdata/elasticsearch/removeIndices.sh` is provided to delete 
indices made through the creation script.

If adding a new index, the following steps should be taken to repair timestamps
or any other invalid data types on a per index or index pattern basis:

1. Create a `json` file to store the mapping. CAST recommends naming the file `<template-name>.json` 

2. Populate the file with configuration settings.

    .. code-block:: javascript

        {
            "index_patterns": ["<NEW INDEX PATTERN>"],
            "order" : 0,
            "settings" : {
                "number_of_shards"   : <SHARDING COUNT>,
                "number_of_replicas" : <REPLICA COUNT>
            },
            "mappings" : {
                "_doc": {
                    "properties" : {
                        "<SOME TIMESTAMP>" : { "type" : "date" },
                    },
                    "dynamic_date_formats" :
                        [ "strict_date_optional_time|yyyy/MM/dd HH:mm:ss Z||
                                yyyy/MM/dd Z||yyyy-MM-dd HH:mm:ss.SSSSSS"]
                }
            }
        }

    .. attention:: The `dynamic_date_formats` section is most relevant to the context of this entry.

    .. note:: To resolve timestamps with microseconds (e.g. postgres timestamps) 
        `yyyy-MM-dd HH:mm:ss.SSSSSS` serves as a sample.

3. Ship the `json` file to elasticsearch. There are two mechanisms to achieve this:
    
    a. Place the file in the `/opt/ibm/csm/bigdata/elasticsearch/templates/` directory and run 
        the `/opt/ibm/csm/bigdata/elasticsearch/createIndices.sh` script.
    
    b. Curl the file to Elasticsearch.
        
        .. code-block:: bash

            curl -s -o /dev/null -X PUT "${HOST}:9200/_template/${template_name}?pretty"\
                -H 'Content-Type: application/json' -d ${json-template-file}

    .. attention:: If the template is changed the old template must be removed first!


To remove a template the admin may either run the `/opt/ibm/csm/bigdata/elasticsearch/removeIndices.sh`
script, which removes templates by the file names in `/opt/ibm/csm/bigdata/elasticsearch/templates/`.

The other option is to remove a template specifically with a curl command:

.. code-block:: bash
    
    curl -X DELETE "${HOST}:9200/_template/${template_name}?pretty"


The above documentation is a brief primer on how to modify templates, a powerful elasticsearch utility.
If the user needs more information please consult the official `elastic template documentation`_.

.. _elastic template documentation: https://www.elastic.co/guide/en/elasticsearch/reference/current/indices-templates.html


Fixing Timestamps in Logstash
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the elasticsearch methodology doesn't apply to the use case, logstash timestamp manipulation 
might be the correct solution. 

.. note:: The following section performs modifications to the `logstash.conf`
    file that should be placed in `/etc/logstash/conf.d/logstash.conf` if following the 
    :ref:`CASTLogstash` configuration documentation.

The CAST solution uses the `date filter plugin`_ to achieve these results. In the shipped 
configuration the following sample is provided:

.. code-block:: javascript
    
    if "ras" in [tags] and "csm" in [tags] {
        date {
            match => ["time_stamp", "ISO8601","YYYY-MM-dd HH:mm:ss.SSS" ]
            target => "time_stamp"
        }
    }

The above sample parses the `time_stamp` field for the `ISO 8601`_ standard and converts it
to something that is definitely parseable by elasticsearch. For additional notes about this
utility please refer to the official `date filter plugin`_ documentation. 

.. _date filter plugin: https://www.elastic.co/guide/en/logstash/current/plugins-filters-date.html
.. _ISO 8601: https://www.iso.org/iso-8601-date-and-time-format.html

.. _ELK Date: https://www.elastic.co/guide/en/elasticsearch/reference/current/date.html



