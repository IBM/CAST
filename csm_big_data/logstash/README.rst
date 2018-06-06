.. _logstash:

Logstash
********

Logstash is a data collection engine bundled with Log Analysis that parses then pipes
data to the Big Data Store.

.. note:: This documentation assumes that Log Analysis has been installed, along with 
   Logstash ( this is covered in the documentation received with Log Analysis ).

**Contents:**

.. contents::
    :local:

Quick Configuration
-------------------

:Configuration Script:
    `/opt/ibm/csm/bigdata/Logstash/quick_config.sh`

To quickly deploy the default configuration supplied in this sample, run
`/opt/ibm/csm/bigdata/Logstash/quick_config.sh`

.. warning:: The default behavior of this script uses the existing scala url in the default
    config file. To change the IP supply the `-i <ip_address>` flag 
    (e.g. ./quick_config.sh -i 172.0.0.1). For more granular control modifying this script
    is necessary.


.. _logstash-configuration-file:

Configuration File
------------------

:Source File:
    `/opt/ibm/csm/bigdata/Logstash/config/logstash-scala.conf`

:Target File:
    `${LogstashHome}/logstash-scala/logstash/config/logstash-scala.conf`

.. note:: This configuration assumes only a single configuration file.

The Logstash configuration file defines the pipline for processing input to the big data store and is divided into 3 categories:

* `input`_
* `filter`_
* `output`_

If the default configuration is sufficient for your needs simply do the following:

1. Copy the source file from the rpm to the config directory:

.. code-block:: bash

    cp /opt/ibm/csm/bigdata/Logstash/config/logstash-scala.conf \
        ${LogstashHome}/logstash-scala/logstash/config/logstash-scala.conf

2. Change the `scala_url`, `scala_user` and `scala_password` fields in logstash-scala.conf (`output`_).
    
3. Copy over the patterns for Logstash:

.. code-block:: bash

   cp -r /opt/ibm/csm/bigdata/Logstash/patterns/ \
        ${LogstashHome}/logstash-scala/logstash/patterns/

4. Install the `grok_dynamic`_ plugin:

.. code-block:: bash

    /opt/ibm/csm/bigdata/Logstash/plugins/grok_dynamic/build.sh

    ${LogstashHome}/bin/logstash-plugin install  \
        /opt/ibm/csm/bigdata/Logstash/plugins/grok_dynamic/grok_dynamic-0.0.1.gem

5. Restart Logstash:

.. code-block:: none
    
    /opt/utilities/logstash-util.sh restart


.. warning:: ${LogstashHome} must be replaced with the Logstash home directory, this will change depending on your version of Log Analysis!


For data source port details please review `input`_

For data source output details please review `output`_

input
^^^^^

+--------------------------------------+
|       Default Port Values            |
+-----------------+--------------------+
|    Data Type    |     Port Number    |
+-----------------+--------------------+
|      syslog     |       10515        | 
+-----------------+--------------------+
| bmc_temp_sensor |       10516        |
+-----------------+--------------------+
| ib_temp_sensor  |       10517        |
+-----------------+--------------------+
|     bmc_sel     |       10518        |
+-----------------+--------------------+
|      zimon      |       10519        | 
+-----------------+--------------------+

.. note:: If a Data Record is passed through syslog it must be tagged, please 
   refer to `filter`_ for more details.

.. note:: bmc_sel is an in progress data source, it is represented on this table
   to earmark its default port.

The input section of the configuration file configures what Logstash monitors for input
to its pipeline. Currently only tcp inputs are configured in the sample, but Logstash 
supports monitoring files, databases and more.

Each input has the ability to assign a type to an `event` that is then passed through
the pipeline. These types are used to failitate the `filter`_ step of the process.

filter
^^^^^^

The filter step of the pipeline enriches the data received in the `input`_ step:
adding tags for special logs, reformatting timestamps, resolving logs to 
variable mappings in the pipeline's `event` object. This section heavily depends 
on `Patterns`_ which defines the variable mappings for grok parsing. 

.. warning:: The sample config file uses the `grok_dynamic` plugin in the Mellanox_event_log filter, ensure this plugin is is installed!

In the sample configuration there are filters for all of the types listed in
`input`_, additionally there are further filters for several of the data types:

:syslog:
    * eventlog <Mellanox Event Log>
        * This log will then filter further using the event_id and grok_dynamic.
    * mmfs <GPFS Message Log>
        * This further filter is determined by the existence of the `mmfs:` tag.

output
^^^^^^

The output step performs the final packaging to send the filtered `event` object to
Log Analysis using the Log Analysis provided `scala` plugin.

The following **must** be changed in the scala plugin:

:scala_url: The Log Analysis data collector url. If configured with the defaults, 
    only the ip address should need to be changed.

:scala_user: A Log Analysis user with write privleges.

:scala_user: The password for `scala_user`.

.. warning:: If this is not configured, Logstash will not writeto Log Analysis.

When a new data source is added, add a key value pair to the `scala_fields` attribute as follows:

.. code-block:: bash

    "${data_source}@${data_source}" => "field_1, field_2, .., field_n"

The field names should be the ones specified in the `Patterns`_.

Plugins
-------

Logstash has support for a number of useful plugins, specified in the 
`Logstash Reference`_. These plugins allow for various types of data 
enrichment, processing and delivery. The focus of this document is to outline 
how to install the custom plugins this document is shipped with and how to use them.
    
To build the plugin run the following from the source directory of the plugin:

.. code-block:: none

   ./build.sh

After building, a gem should be produced ( as seen in the output of build.sh ), 
copy this to the logstash node and run the following:

.. code-block:: bash

    ${LogstashHome}/bin/logstash-plugin install ${PluginGem}

    /opt/utilities/logstash-util.sh restart
    

.. _Logstash Reference: https://www.elastic.co/guide/en/logstash/current/index.html 

grok_dynamic
^^^^^^^^^^^^

:Source Directory: /opt/ibm/csm/bigdata/Logstash/plugins/grok_dynamic

:Plugin Type: filter

The `grok_dynamic` plugin is an enhancement to the existing grok plugin. 
`grok_dynamic` should only be envoked after fields have been added to the `event`
through grok or som other filter plugin.

`grok_dynamic` accepts the following arguments:

:data_field: The field to perform the pattern match on. This corresponds to a field
    in the Logstash event being passed through the pipeline.

:selector_field: The field that selects the pattern to match against `data_field`.
    This field corresponds to a field in the Logstash event being passed through the pipeline.

:dynamic_patterns_dir: A directory containing `Patterns`_ that have keys matching
    potential values of the `selector_field` ( e.g. event ids ). These patterns 
    will be run against the `data_field` assuming a pattern matching the 
    `selector_field` is found.


If `grok_dynamic` successfully runs more fields willbe added to the event for use in 
the `output`_ portion of the pipeline.


.. _logstash-patterns:

Patterns
--------

:Source File:
    `/opt/ibm/csm/bigdata/Logstash/patterns`

:Target File:
    `${LogstashHome}/logstash-scala/logstash/patterns`

The patterns directory is core to the operation of the `filter`_ step of the pipeline.
Each file in the patterns directory contains a newline delimited list 
of grok patterns. To use the patterns in a search, the directory is added to 
`patterns_dir` in a grok filter. 

In the supplied sample, several data source and event patterns have been implemented.
To add custom patterns, please consult the `grok basics` documentation on the 
logstash site.

.. warning:: If a supplied pattern is changed, the `output`_ configuration and 
   :ref: `log-analysis` data sources may need to be changed!

.. _grok basics: https://www.elastic.co/guide/en/logstash/current/plugins-filters-grok.html#_grok_basics


Grok Benchmarking
^^^^^^^^^^^^^^^^^

Grok can be slow if it's not properly configured and the Regular Expression patterns are not properly planned.
Reading the elastic `groking grok` blog post is highly recommended by CSM. Some key points are 
reproduced below with additional observations:

1. Profile your patterns, elastic has a benchmarking script provided in the `groking grok` blog post.
2. Grok failure can be expensive, use anchors (^ and $) to make string matches precise to reduce failure costs.
3. _groktimeout tagging can set an upper bound.


Writing Grok Patterns:

* Avoid `DATA` and `GREEDYDATA` if possible // TODO Verify through profiling.


Logstash benchmark-cli tool: 
https://github.com/elastic/logstash/tree/5553f5886a3254f8206cbc5113d30ee7099397d1/tools/benchmark-cli

Need to link the config directory.
./benchmark.sh --workdir=/tmp/benchmark2 --testcase=baseline --local-path /usr/share/logstash/

.. _groking grok: https://www.elastic.co/blog/do-you-grok-grok

generic
^^^^^^^

:Pattern Files:
    | logstash-scala

This pattern directory contains the generic patterns that extract attributes from
the enabled data sources. 

The following mappings are supplied in the sample:

:RSYSLOGDSV:
    :timestamp: TIMESTAMP_ISO8601
    :syslogHostname: HOSTNAME
    :syslogRelayHostname: DATA
    :tag: DATA
    :programName: DATA
    :processID: DATA
    :facility: DATA
    :syslogSeverity: DATA
    :syslogAppName: DATA
    :originalMessage: GREEDYDATA

:MMFSMSG:
    :mmfsSeverity: WORD
    :mmfsEventDescription: GREEDYDATA

:ZIMONMSG:
    :zimonHostname: HOSTNAME
    :zimon_unix_time: INT
    :zimon_cpu_system: NUMBER
    :zimon_cpu_user: NUMBER
    :zimon_mem_active: NUMBER
    :zimon_gpfs_ns_bytes_read: NUMBER
    :zimon_gpfs_ns_bytes_written: NUMBER
    :zimon_gpfs_ns_tot_queue_wait_rd: NUMBER
    :zimon_gpfs_ns_tot_queue_wait_wr: NUMBER

:IBSENSORMSG: 
    :sensor_unix_time: TIMESTAMP_ISO8601
    :ib_hostname: HOSTNAME
    :ib_temp_cpu_core_t1: NUMBER
    :ib_temp_cpu_core_t2: NUMBER
    :ib_temp_cpu_package: NUMBER
    :ib_temp_power_mon_ps1: NUMBER
    :ib_temp_power_mon_ps2: NUMBER
    :ib_temp_board_ambient: NUMBER
    :ib_temp_ports_ambient: NUMBER
    :ib_temp_SIB: NUMBER

:SENSORMSG: 
    :sensor_unix_time: TIMESTAMP_ISO8601
    :bmc_hostname: HOSTNAME
    :bmc_ip: HOSTNAME
    :bmc_temp_ambient: NUMBER
    :bmc_temp_CPU_min: NUMBER
    :bmc_temp_CPU_max: NUMBER
    :bmc_temp_CPU_Core_min: NUMBER
    :bmc_temp_CPU_Core_max: NUMBER
    :bmc_temp_DIMM_min: NUMBER
    :bmc_temp_DIMM_max: NUMBER
    :bmc_temp_GPU_min: NUMBER
    :bmc_temp_GPU_max: NUMBER
    :bmc_temp_Mem_Buff_min: NUMBER
    :bmc_temp_Mem_Buff_max: NUMBER

:BMC_SEL: 
    :bmc_sel_time: TIMESTAMP_ISO8601
    :bmc_sel_hostname: HOSTNAME
    :bmc_sel_ip: HOSTNAME
    :bmc_sel_event_id: DATA
    :bmc_sel_event_description: GREEDYDATA

.. note:: This is only intended as a reference to the contents of the directory,
   specifics regarding the formatting of the patterns are ommitted for brevity.

mellanox_event_log
^^^^^^^^^^^^^^^^^^

:Pattern Files:
    | logstash-mellanox_event_log

This pattern directory contains a collection of Mellanox event id patterns and
logging patterns for specific mellanox log types.

Several helper patterns are defined to assist the log mapping patterns:

.. code-block:: none

   MELLANOXTIME %{YEAR}-%{MONTHNUM}-%{MONTHDAY} %{TIME}
   GUID [0-9a-f]{16}
   SOURCE_LINK \[Source .*TO Dest:.*\]:


The following log mappings are supplied in the sample:

:MELLANOXMSG:
    :Origtimestamp: MELLANOXTIME
    :log_counter: NUMBER
    :event_id: NUMBER
    :severity: WORD
    :event_type: WORD
    :category: WORD
    :event_description: GREEDYDATA

:328: 
    :s_host: HOSTNAME
    :s_port: NUMBER
    :d_host: HOSTNAME
    :d_port: NUMBER

:329: 
    :s_host: HOSTNAME
    :s_port: NUMBER
    :d_host: HOSTNAME
    :d_port: NUMBER
 
