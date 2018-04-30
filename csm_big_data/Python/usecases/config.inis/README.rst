CSM Big Data Use Case Configuration
***********************************

.. warning:: THIS FILE IS DEPRECATED! USE --helpconfig!

The CSM Big Data use cases are configurable through 2 different means: configparser formatted 
`Configuration Files`_ and use case specific `Command Line` arguments. For details regarding
the command line options please consult the use case and `csmbigdata.config` documentation.

**Contents**

.. contents::
   :local:

Configuration Files
-------------------

The following configuration files make use of the default `configparser`_ format.
Each section header represents a cofiguration section header in an ini file.

.. _configparser: https://docs.python.org/2/library/configparser.html

remote.server
^^^^^^^^^^^^^

:Config Module: csmbigdata.config.server_config

The `remote.server` configuration section specifies the components of a uri to a remote
server. By default this has a few settings that make connecting to a `Log Analysis` server,
but this is a destination agnostic section.

**Supported Attributes**

:protocol: 
    :Description: The internet protocol to use in connecting with the remote server; typically https.
    :Type: **string**
    :Default: **https**

:host:
    :Description: The hostname or IP address of the server to be connected to.
    :Type: **string**
    :Default: **N/A**

:port:
    :Description: The port that the script should attempt to access. If no port is specified, it is assumed that no port should be used in the uri.
    :Type: **string**
    :Default: **-1**

:dir:
    :Description: The base directory to perform commands upon, for `Log Analysis` the default should be sufficient.
    :Type: **string**
    :Default: **Unity**

:access:
    :Description: The path, relative or absolute, to a coniguration file that has an `access.details`_ configuration section.
    :Type: **string**
    :Default: **N/A**


access.details
^^^^^^^^^^^^^^

:Config Module: csmbigdata.config.server_config

The `access.details` section specifies access settings for connecting to a remote server. 
It is recommended that this configuration file be separated in another directory with stricter 
file access controls. The contents of this file is subject to change pending improved 
access models to the Log Analysis Server. 600 is the recommended mask for a file with these
details.

**Supported Attributes**

:userid: 
    :Description: The user id to connect to a server.
    :Type: **string**
    :Default: **N/A**

:userpass:
    :Description: The user password to connect to a server.
    :Type: **string**
    :Default: **N/A**

job.settings
^^^^^^^^^^^^

:Config Module: csmbigdata.config.job_config

The `job.settings` section specifies job related details (e.g. job ids, hostnames, etc.). All of 
the currently implemented use cases take advantage of this configuration setting.

**Supported Attributes**

:num_days:
    :Description: The number of days to search around the `target_datetime`, how this range is computed is dependant upon the script.
    :Type: **float**
    :Default: **7**

:keywords:
    :Description: Keywords to filter on or search for, actual implementation determined by script.
    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **None** 

:job_id:
    :Description: The primary job id that the script is performing an operation for.
    :Type: **int**
    :Default: **-1**

:secondary_job_id:
    :Description: The secondary job id that the scriptis performing an operation for.
    :Type: **int**
    :Default: **0**

:target_hostnames:
    :Description: The hostnames to filter any searching or queries down to.
    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **None**

:target_datetime: 
    :Description: A date near when the job was expected to be run, this is typically a fallback.
    :Type: **string**
    :Format: **MM-DD-YYYY HH:MM:SS.sX**
    :Default: **None**



stat.settings
^^^^^^^^^^^^^

:Config Module: csmbigdata.config.stat_config

The `stat.settings` section is designed to aid in the computation, aggregation and presentation
of statistics from the Log Analysis Big Data Store.


**Supported Attributes**

:log_sources:
    :Description: Log Sources to perform statistical analysis on, filtered by script computed or user specified filter. Each value should lead with a '/'.
    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **None**

:log_tags:
    :Description: Log Tags to perform statistical analysis on, filtered by script computed or user specified filter. Each value should lead with a '/'.
    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **None**

:log_sources_all:
    :Description: Log Sources to perform statistical analysis on, this performs no filtering on the results. Each value should lead with a '/'.
    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **None**

:log_tags_all:
    :Description: Log Tags to perform statistical analysis on, this performs no filtering on the results. Each value should lead with a '/'.
    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **None**

:stat_options:
    :Description: The metrics to compute and display.
    :Accepted Values:
        * **min** - The minimum values in the statistical window.
        * **min** - The maximum values in the statistical window.
        * **avg** - The averages in the statistical window.
        * **std** - The standard deviations for the statistical window.

    :Type: **string**
    :Format: **Comma Separated Values**
    :Default: **min,max,avg,std**


The `csmbigdata.config.stat_config` module also supports specifying the following details about
the `Log Sources` and `Log Tags` specified in above sections.

:section header: This is the name of the Log Source or tag without the leading '/'. This section should specify one or more of the following fields:

    :hostname_key: 
        :Description: The name of the hostname field in the source or tag. If this is not specified a script must guess at the field containing the hostname.
        :Type: **string**

    :field_keys: 
        :Description: The name of the fields to perform statistical analysis on. If this is not specified a script must guess at the fields that contain actionable data.
        :Type: **string**
        :Format: **Comma Separated Values**

    :timestamp_key: 
        :Description: The name of the timestamp field to use in the source or tag. If this is not specified a script must guess at the field containing the timestamp.
        :Type: **string**

