.. _csm-event-correlator-config:

CSM Event Correlator Filter Plugin
==================================

.. attention:: The following document is a work in progress! The CSM Event Correlator is currently
    under development and the interface is subject to change. 


Parses arbitrary text and structures the results of the parse into actionable events.

The CSM Event Correlator is a utility by which a system administrator may specify a collection
of patterns (grok style), grouping by context (e.g. syslog, event log, etc.), which trigger 
actions (ruby scripts).

.. contents::
   :local:


Installation
------------

The CSM Event Correlator comes bundled in the `ibm-csm-bds-logstash*.noarch.rpm` rpm. 
When installing the rpm, any old versions of the plugin will be removed and the bundled version
will be installed.

CSM Event Correlator Pipeline Configuration Options
---------------------------------------------------
This plugin supports the following configuration options:

+------------------------+-------------+----------+
| Setting                | Input type  | Required |
+========================+=============+==========+
| `events_dir`_          | `string`_   | No       |
+------------------------+-------------+----------+
| `patterns_dir`_        | `array`_    | No       |
+------------------------+-------------+----------+
| `named_captures_only`_ | `boolean`_  | No       |
+------------------------+-------------+----------+

Please refer to `common-options`_ for options supported in all Logstash
filter plugins.

This plugin is intended to be used in the filter block of the logstash configuration file.
A sample configuration is reproduced below:

.. code-block:: none

    filter {
        csm_event_correlator {
            events_dir => "/etc/logstash/patterns/events.yml"
            patterns_dir => "/etc/logstash/patterns/*.conf"
        }
    }

events_dir
^^^^^^^^^^
:Value type: `string`_
:Default value:  `/etc/logstash/conf.d/events.yml`

The configuration file for the event correlator, see `CSM Event Correlator Event Configuration File`_
for details on the contents of this file.

This file is loaded on pipeline creation.

.. attention:: This field will use an `array`_ in future iterations to specify multiple configuration
    files. This change should not impact existing configurations.

patterns_dir
^^^^^^^^^^^^
:Value type: `array`_
:Default value: `[]`

A directory, file or filepath with a glob. The listing of files will be parsed for grok patterns
which may be used in writing patterns for event correlation. If no glob is specified in the path
`*` is used.

Configuration with a file glob:

.. code-block:: ruby
    
    patterns_dir => "/etc/logstash/patterns/*.conf" # Retrieves all .conf files in the directory.

Configuration with multiple files:

.. code-block:: ruby
   
   patterns_dir => ["/etc/logstash/patterns/mellanox_grok.conf", "/etc/logstash/patterns/ibm_grok.conf"]

CSM Event Correlator will load the default Logstash patterns regardless of the contents of this
field.

Pattern files are plain text with the following format:

.. code-block:: ruby
    
    NAME PATTERN

For example:

.. code-block:: ruby

    GUID [0-9a-f]{16}

The patterns are loaded on pipeline creation.

named_captures_only
^^^^^^^^^^^^^^^^^^^
:Value type: `boolean`_
:Default value: `true`

If true only store captures that have been named for grok. `Anonymous` captures are considered 
named.

CSM Event Correlator Event Configuration File
---------------------------------------------

CSM Event Correlator uses a YAML file for configuration. The YAML configuration is

heirarchical with 3 major groupings:

* `Metadata`_
    * `Data Sources`_
        * `Event Categories`_

This is a sample configuration of this file:

.. code-block:: YAML

    ---
    # Metadata
    ras_create_url: "/csmi/V1.0/ras/event/create"
    csm_target: "localhost"
    csm_port: 4213
    data_sources:
    
     # Data Sources
     syslog:
        ras_location:  "syslogHostname"
        ras_timestamp: "timestamp"
        event_data:    "message"
        category_key:  "programName"
        categories:
    
         # Categories
         NVRM:
            - tag: "XID_GENERIC"
              pattern:    "Xid(%{DATA:pciLocation}): %{NUMBER:xid:int},"
              ras_msg_id: "gpu.xid.%{xid}"
              action:     'unless %{xid}.between?(1, 81); ras_msg_id="gpu.xid.unknown" end; .send_ras;'
         mlx5_core:
            - tag: "IB_CABLE_PLUG"
              pattern:    "mlx5_core %{MLX5_PCI}.*module %{NUMBER:module}, Cable (?<cableEvent>(un)?plugged)"
              ras_msg_id: "ib.connection.%{cableEvent}"
              action:     ".send_ras;"
         mmsysmon:
            - tag: "MMSYSMON_CLEAN_MOUNT"
              pattern: "filesystem %{NOTSPACE:filesystem} was (?<mountEvent>(un)?mounted)"
              ras_msg_id: "spectrumscale.fs.%{mountEvent}"
              action: ".send_ras;"
            - tag: "MMSYSMON_UNMOUNT_FORCED"
              pattern: "filesystem %{NOTSPACE:filesystem} was.*forced.*unmount"
              ras_msg_id: "spectrumscale.fs.unmount_forced"
              action: ".send_ras;" 
    ...


Metadata
^^^^^^^^

The metadata section may be thought of as global configuration options that will apply to all events
in the event correlator. 

+-------------------+------------+----------------------+
| Field             | Input type | Required             |
+===================+============+======================+
| `ras_create_url`_ | string     | Yes <Initial Release>|
+-------------------+------------+----------------------+
| `csm_target`_     | string     | Yes <Initial Release>|
+-------------------+------------+----------------------+
| `csm_port`_       | integer    | Yes <Initial Release>|
+-------------------+------------+----------------------+
| `data_sources`_   | map        | Yes                  |
+-------------------+------------+----------------------+

ras_create_url
**************
:Value type: string
:Sample value: `/csmi/V1.0/ras/event/create`

Specifies the REST create resource on the node runnning the CSM REST Daemon. This path will be
used by the `.send_ras;` utility.

.. attention:: In a future release `/csmi/V1.0/ras/event/create` will be the default value.

csm_target
**********
:Value type: string
:Sample value: `127.0.0.1`

A server running the CSM REST daemon. This server will be used to generate ras events with the
`.send_ras;` utility.

.. attention:: In a future release `127.0.0.1` will be the default value.

csm_port
********
:Value type: integer
:Sample value: `4213`

The port on the server running the CSM REST daemon. This port will be used to connect by the 
`.send_ras;` utility.

.. attention:: In a future release `4213` will be the default value.

data_sources
************
:Value type: map

A mapping of data sources to event correlation rules. The key of the `data_sources` field 
matches `type` field of the logstash event processed by the filter plugin. The type field
may be set in the `input` section of the logstash configuration file.

Below is an example of setting the type of all incoming communication on the `10515` tcp port to
have the `syslog` `type`:

.. code-block:: none

    input {
        tcp {
            port => 10515
            type => "syslog"
        }
    }

The YAML configuration file for the `syslog` data source would then look something like this:

.. code-block:: YAML

        syslog:
            # Event Data Sources configuration settings.
        # More data sources.

The YAML configuration uses this structure to reduce the pattern space for event matching. If the
user doesn't configure a type in this `data_sources` map CSM will discard events of that type for
consideration in event correlation.


Data Sources
^^^^^^^^^^^^

Event data sources are entries in the `data_sources`_ map.
Each data source has a set of configuration options which allow the event correlator to parse
the structured data of the logstash event being checked for event corelation/action generation.

This section has the following configuration fields:

+------------------+------------+----------------------+
| Field            | Input type | Required             |
+==================+============+======================+
| `ras_location`_  | string     | Yes <Initial release>|
+------------------+------------+----------------------+
| `ras_timestamp`_ | string     | Yes <Initial release>|
+------------------+------------+----------------------+
| `event_data`_    | string     | Yes                  |
+------------------+------------+----------------------+
| `category_key`_  | string     | Yes                  |
+------------------+------------+----------------------+
| `categories`_    | map        | Yes                  |
+------------------+------------+----------------------+

ras_location
************
:Value type: string
:Sample value: `syslogHostname`

Specifies a field in the logstash event received by the filter. The contents of this
field are then used to generate the ras event spawned with the `.send_ras;` utility. 

The referenced data is used in the `location_name` of the of the REST payload sent by `.send_ras;`.

For example, assume an event is being processed by the filter. This event has the field 
`syslogHostname` populated at some point in the pipeline's execution to have the value of `cn1`.
It is determined that this event was worth responding to and a RAS event is created. Since
`ras_location` was set to `syslogHostname` the value of `cn1` is POSTed to the CSM REST daemon
when creating the RAS event.

ras_timestamp
*************
:Value type: string
:Sample value: `timestamp`

Specifies a field in the logstash event received by the filter. The contents of this
field are then used to generate the ras event spawned with the `.send_ras;` utility. 

The referenced data is used in the `time_stamp` of the of the REST payload sent by `.send_ras;`.

For example, assume an event is being processed by the filter. This event has the field 
`timestamp` populated at some point in the pipeline's execution to have the value of 
*Wed Feb 28 13:51:19 EST 2018*. It is determined that this event was worth responding to 
and a RAS event is created. Since `ras_timestamp` was set to `timestamp` the value of 
*Wed Feb 28 13:51:19 EST 2018* is POSTed to the CSM REST daemon when creating the RAS event.

event_data
**********
:Value type: string
:Sample value: `message`

Specifies a field in the logstash event received by the filter. The contents of this field
are matched against the specified patterns. 

.. attention:: This is the data checked for event correlation once the event list has been selected,
    make sure the correct event field is specified.

category_key
************
:Value type: string
:Sample value: `programName`

Specifies a field in the logstash event received by the filter. The contents of this field
are used to select the category in the `categories` map. 

categories
**********
:Value type: map

A mapping of data sources categories to event correlation rules. The key of the `categories` field
matches field specified by `category_key`. In the included example this is the program name of a 
syslog event.

This mapping exists to reduce the number of pattern matches performed per event. Events that don't
have a match in the categories map are ignored when performing further pattern matches.

Each entry in this map is an array of event correlation rules with the schema described in 
`Event Categories`_. Please consult the sample for 
formatting examples for this section of the configuration.

Event Categories
^^^^^^^^^^^^^^^^

Event categories are entries in the `categories`_ map.
Each category has a list of tagged configuration options which specify an event correlation rule.

This section has the following configuration fields:

+---------------+------------+-----------------------+
| Field         | Input type | Required              |
+===============+============+=======================+
| `tag`_        | string     | No                    |
+---------------+------------+-----------------------+
| `pattern`_    | string     | Yes <Initial Release> |
+---------------+------------+-----------------------+
| `action`_     | string     | Yes <Initial Release> |
+---------------+------------+-----------------------+
| `extract`_    | boolean    | No                    |
+---------------+------------+-----------------------+
| `ras_msg_id`_ | string     | No <Needed for RAS>   |
+---------------+------------+-----------------------+

tag
***
:Value type: string
:Sample value: `XID_GENERIC`

A tag to identify the event correlation rule in the plugin. If not specified an internal identifier
will be specified by the plugin. Tags starting with `.` will be rejected at the load phase as 
this is a reserved pattern for internal tag generation.

.. note:: In the current release this mechanism is not fully implemented.

pattern
*******
:Value type: string
:Sample value: `mlx5_core %{MLX5_PCI}.*module %{NUMBER:module}, Cable (?<cableEvent>(un)?plugged)`

A grok based pattern, follows the rules specified in `Grok Primer`_.
This pattern will save any pattern match extractions to the event travelling through the pipeline. 
Additionally, any extractions will be accessible to the `action` to drive behavior. 

action
******
:Value type: string
:Sample value: `unless %{xid}.between?(1, 81); ras_msg_id="gpu.xid.unknown" end; .send_ras;`

A ruby script describing an action to take in response to an event. The `action` is taken when
an event is matched. The plugin will compile these scripts at load time, cancelling the startup
if invalid scripts are specified.

This script follows the rules specified in `CSM Event Correlator Action Programming`_.

extract
*******
:Value type: boolean
:Default value: false

By default the Event Correlator doesn't save the extract pattern matches in `pattern`_ to the final event
shipped to elastic search or your big data platform of choice. To save the pattern extraction
this field must be set to true.

.. note:: This field does not impact the writing of `action`_ scripts.

ras_msg_id
**********
:Value type: string
:Sample value: `gpu.xid.%{xid}`

A string representing the ras message id in event creation. This string may specify fields in the 
event object through use of the `%{FIELD_NAME}` pattern. The plugin will attempt to populate
the string using this formatting before passing to the action processor.

For example, if the event has a field `xid` with value `42` the pattern `gpu.xid.%{xid}` will resolve
to `gpu.xid.42`.

Grok Primer
-----------

CSM Event Correlator uses grok to drive pattern matching. 

Grok is a regular expression pattern checking utility. A typical grok pattern has the following
syntax: `%{PATTERN_NAME:EXTRACTED_NAME}`

`PATTERN_NAME` is the name of a grok pattern specified in a pattern file or in the default Logstash
pattern space. Samples include `NUMBER`, `IP` and `WORD`. 

`EXTRACTED_NAME` is the identifier to be assigned to the text in the event context. The 
`EXTRACTED_NAME` will be accessible in the action through use of the `%{EXTRACTED_NAME}` pattern
as described later. `EXTRACTED_NAME` identifiers are added to the big data record in elasticsearch.
The `EXTRACTED_NAME` section is optional, patterns without the `EXTRACTED_NAME` are matched, but
not extracted.

For specifying custom patterns refer to `custom patterns`_.

A grok pattern may also use raw regular expressions to perform non-extracting pattern matches.
`Anonymous` extraction patterns may be specified with the following syntax: `(?<EXTRACTED_NAME>REGEX)`

`EXTRACTED_NAME` in the `anonymous` extraction pattern is identical to the named pattern. `REGEX` is
a standard regular expression.

CSM Event Correlator Action Programming
---------------------------------------

Programming actions is a central part of the CSM Event Correlator. This plugin supports action scripting
using ruby. The action script supplied to the pipeline is converted to an anonymous function which
is invoked when the event is processed.

Default Variables
^^^^^^^^^^^^^^^^^

The action script has a number of variables which are acessible to action writers:

+--------------+-----------------+----------------------------------------------------------------+
| Variable     | Type            | Description                                                    |
+==============+=================+================================================================+
| event        | LogStash::Event | The event the action is generated for, getters provided.       |
+--------------+-----------------+----------------------------------------------------------------+
| ras_msg_id   | string          | The ras message id, formatted.                                 |
+--------------+-----------------+----------------------------------------------------------------+
| ras_location | string          | The location the RAS event originated from, parsed from event. |
+--------------+-----------------+----------------------------------------------------------------+
| ras_timestamp| string          | The timestamp to assign to the RAS event.                      |
+--------------+-----------------+----------------------------------------------------------------+
| raw_data     | string          | The raw data which generated the action.                       |
+--------------+-----------------+----------------------------------------------------------------+

The user may directly influence any of these fields in their action script, however it is recommended
that the user take caution when manipulating the `event` as the contents of this field are ultimately
written to any Logstash targets. The `event` members may be accessed using the `%{field}` syntax.

The `ras_msg_id`, `ras_location`, `ras_timestamp`, and `raw_data` fields are used with the 
`.send_ras;` action keyword.

Accessing Event Fields
^^^^^^^^^^^^^^^^^^^^^^

Event fields are commonly used to drive event actions. These fields may be specified by the 
event corelation rule or other Logstash plugins. Due to the importance of this pattern the 
CSM Event Correlator provides a special syntaxtic sugar for field access `%{FIELD_NAME}`.

This syntax is interpreted as `event.get(FIELD_NAME)` where the field name is a field in the 
event. If the field was not present the field will be interpreted as `nil`.

Action Keywords
^^^^^^^^^^^^^^^

Several action keywords are provided to abstract or reduce the code written in the actions. 
Action keywords always start with a `.` and end with a `;`.


.send_ras; 
**********
Creates a ras event with `msg_id` == `ras_msg_id`, `location_name` == `ras_location`, 
`time_stamp` == `ras_timestamp`, and `raw_data` == `raw_data`.

Currently only issues RESTful create requests. Planned improvements add local calls.

.. TODO Rewrite this documentation.

.. attention:: A clarification for this section will be provided in the near future. (5/18/2018 jdunham@us.ibm.com)

Sample Action
^^^^^^^^^^^^^

Using the above tools an action may be written that:
 1. Processes a field in the event, checking to see it's in a valid range.

    .. code-block:: ruby

        unless %{xid}.between?(1, 81);

 2. Sets the message id to a default value if the field is not within range.

    .. code-block:: ruby
    
        ras_msg_id="gpu.xid.unknown" end;

 3. Generate a ras message with the new id.

    .. code-block:: ruby
    
        .send_ras;

All together it becomes:

.. code-block:: ruby

    unless %{xid}.between?(1, 81); ras_msg_id="gpu.xid.unknown" end; .send_ras;

This action script is then compiled and stored by the plugin at load time then executed when
actions are triggered by events.


Debugging Issues
----------------

Perform the following checks in order, when a matching condition is found, exit the 
debug process and handle that condition. Numbered sequences assume that the user performs 
each step in order.

RAS Event Not Firing
^^^^^^^^^^^^^^^^^^^^

If RAS events haven't been firing for conditions matching `.send_ras` perform the following
diagnostic steps:

*Check the `/var/log/logstash/logstash-plain.log`*

1. Search for the phrase "`Unable send RAS event`" :

    This indicates that the corelator was unable to connect to the CSM REST Daemon. Verify that 
    Daemon is running on the specified hostname and port.

2. Search for the phrase "`Posting ras message`" :

    This indicates that the corelator connected to the CSM REST Daemon, but the RAS events were
    malconfigured. Verify that the message id sent has an analog in the list of RAS events registered
    in CSM.

    The RAS mesage id may be checked using the following utility:

    .. code-block:: bash

        csm_ras_msg_type_query -m "MESSAGE_ID"

3. Neither of these strings were found:

    Review `CSM Event Correlator Event Configuration File`_.


.. Links
.. _common-options: https://www.elastic.co/guide/en/elasticsearch/reference/current/common-options.html
.. _array: https://www.elastic.co/guide/en/elasticsearch/reference/current/array.html
.. _string: https://www.elastic.co/guide/en/elasticsearch/reference/current/text.html
.. _boolean: https://www.elastic.co/guide/en/elasticsearch/reference/current/boolean.html
.. _custom patterns: https://github.com/logstash-plugins/logstash-patterns-core/tree/master/patterns
