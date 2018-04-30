csmbigdata.config
*****************
This Package is designed to consolidate configuration and command line loading in one 
location when interfacing with the `Big Data Store` and `CSM APIs`.

**Contents**

.. contents::
   :local: 

CSM Settings (csmbigdata.config.settings)
=========================================
The primary module for managing user configuration, defines the Settings dictionary.
The Settings dictionary is a specialization of the standard Python dictionary for
parsing and storing user configuration settings.

Usage of the Module
-------------------
To add user input handling support using the Settings module perform the following
steps:

0. Import the settings module and any Settings objects desired:

.. code:: python

   from csmbigdata.config.settings        import Settings
   from csmbigdata.config.time_settings   import TimeSettings

1. Initialize the Settings dictionary:

.. code:: python

   self.settings = Settings( )

2. Set the Help String for the commandline interface (optional):

.. code:: python
   
   self.settings.help_string = "This will be displayed for either -h or --help"

3. Specify what classes the Settings dictionary should use when defining valid user 
    input. These classes must extend the `DefaultSettings` class defined in 
    `csmbigdata.config.default_settings` (optional):

.. code:: python
   
   ''' Appends a class to the Settings Dictionary: 
        * Uses all options defined in the in the class
        * Uses the default configparser section for the class.
        * Uses the default setting name for the key in the settings dictionary.

    This element would be accessed by `self.settings.time`
    The config section would be `time.settings`
    The available options would be: date, days
    '''
    self.settings.append_class( TimeSettings )

    ''' Appends a class to the Settings Dictionary:
        * Uses only the options in the specified list.
        * Uses "time.test" for the configparser section.
        * Uses "time_2 as" the key for in the settings dictionary.

    This element would be accessed by `self.settings.time_2`
    The config section would be `time.test`
    The available options would be: date
    '''
    self.settings.append_class( TimeSettings, [ "date" ], "time.test", "time_2")

.. warning:: If two classes have the same command line arguments collisions may occur!

4. Append any custom settings that didn't warrant their own Module (optional):

.. code:: python

    ''' Appends an anonymous Settings Element to the Settings dictionary.

        This Element will be registered with the `hive` key in the dictionary,
        with one option and be associated with the "
    '''
    self.settings.append_class_anon( "hive",
        {"query_user" : SettingsOption( None, "hiveuser", "user", "hdfs",
             '''The user to execute Hive queries''', '''string''' )
        }, "hive.server" )


5. Run the parse args function on the `sys.argv` list to process the user input:

.. code:: python

    self.settings.parse_args(args)

The results of the user input should be in the Settings dictionary,
accessible by `self.settings`. The keys for the Settings Elements
can be found in `self.settings.settings_elements`.

For more details please see :ref:`csm-settings-element` 
and :ref:`csm-settings-option`.


Module Members
--------------

.. automodule:: csmbigdata.config.settings
   :members:
   :noindex:



.. _csm-settings-element:

CSM Settings Element (csmbigdata.config.default_settings)
=========================================================
A `Settings Element` in the `csmbigdata.config` package is defined by the 
`DefaultSettings` dictionary in the `csmbigdata.config.default_settings`
module. A settings element has a collection of `Settings Options` 
(see :ref:`csm-settings-option`) key-value pairs representing the possible 
inputs that the Element supports.

Module Usage
------------
There are three critical components to the `Settings Element`: `CONFIG_MAP`_,
`CONFIG_SECTION`_, `CONFIG_NAME`_. An extension of `DefaultSettings` should
define all three. For an example of all three in action see :ref:`csm-time-settings`.

CONFIG_MAP
++++++++++
`CONFIG_MAP` is a statically defined map of {"option_name" : "SettingsOption"}.
This mapping defines the available options for a `Settings Element`. If the 
user doesn't supply a map of their own in the initialization of a `DefaultSettings`
dictionary this mapping will be used to populate it.

CONFIG_SECTION
++++++++++++++
`CONFIG_SECTION` is the name of the section in a configparser ini file for this
`Settings Element`. If the user does not specify the section name this will take over
as a default for the `Settings Element` instantiated.

CONFIG_NAME
+++++++++++
`CONFIG_NAME` is the default name that the `Settings` dictionary will assign
to a `Settings Element` by default. 

Module Members
--------------

.. automodule:: csmbigdata.config.default_settings
      :members:
      :noindex:

.. _csm-settings-option:

CSM Settings Option (csmbigdata.config.settings_option)
=========================================================
A `Settings Option` in the `csmbigdata.config` package is defined by the
`SettingsOption` class in the `csmbigdata.config.settings_option`. This 
class represents a command line option and defines several fields that are 
processed by the base `Settings Element`. Behavior for the generation of help 
strings and generation of sample configuration lines.

Module Usage
------------

This module is largely a struct, storing an option and its details.
The function prototype for its constructor is as follows:

.. code:: python

    def __init__(self,
        short_name = None,
        long_name = None,
        config_name = None,
        default = None,
        description = "",
        arg_pattern = "",
        parse_funct = None,
        subsection_funct = None,
        is_flag = False):

For details on the individual fields refer to the documentation below.


Module Members
--------------

.. automodule:: csmbigdata.config.settings_option
   :members:
   :noindex:

.. _csm-default-settings:

CSM Default Settings Element (csmbigdata.config.DefaultCommandSettings)
=======================================================================

A collection of options that are always enabled in the `Settings` dictionary 
in the `default` key. `help` and `help_config` will display help an exit execution
when they are complete.

+-------------------------------------------------+
|            Default Settings Options             |
+------------------+-----------+------------------+
|   Setting Name   | CMD Short |     CMD Long     |
+------------------+-----------+------------------+
|      help        |    -h     |    --help        |
+------------------+-----------+------------------+
|   help_config    |    N/A    |    --helpconfig  |
+------------------+-----------+------------------+
|     verbose      |    -v     |    --verbose     |
+------------------+-----------+------------------+
|     force        |    -f     |    --force       |
+------------------+-----------+------------------+
|     output       |    N/A    |    --output      |
+------------------+-----------+------------------+
|     config       |    -c     |    --config      |
+------------------+-----------+------------------+

.. automodule:: csmbigdata.config.default_command_settings
    :members:
    :noindex:


CSM Job Settings Element (csmbigdata.config.job_settings)
=========================================================

+--------------------------------------------------------------------+
|                       Job Settings Options                         |
+------------------+-----------+------------------+------------------+
|   Setting Name   | CMD Short |     CMD Long     |    Config File   |
+------------------+-----------+------------------+------------------+
|      job_id      |    -j     |     --jobid      |      job_id      |
+------------------+-----------+------------------+------------------+
| secondary_job_id |    -s     | --jobidsecondary | secondary_job_id |
+------------------+-----------+------------------+------------------+
|     keywords     |    -k     |    --keywords    |     keywords     |
+------------------+-----------+------------------+------------------+
| target_hostnames |    -n     |     --hosts      | target_hostnames |
+------------------+-----------+------------------+------------------+
|     nohosts      |    N/A    |     --nohosts    |        N/A       |
+------------------+-----------+------------------+------------------+
|     nokeys       |    N/A    |     --nokeys     |        N/A       |
+------------------+-----------+------------------+------------------+

.. note:: The `Setting Name` column will be a member name of a `SettingsElement`
   added to the `Settings` dictionary by `append_class`.

.. automodule:: csmbigdata.config.job_settings
    :members:
    :noindex:

CSM Server Settings Element (csmbigdata.config.server_settings)
===============================================================

+--------------------------------------------------------------------+
|                      Server Config Options                         |
+------------------+-----------+------------------+------------------+
|   Setting Name   | CMD Short |     CMD Long     |    Config File   |
+------------------+-----------+------------------+------------------+
|       host       |    N/A    |        N/A       |       host       |
+------------------+-----------+------------------+------------------+
|       port       |    N/A    |        N/A       |       port       |
+------------------+-----------+------------------+------------------+
|     protocol     |    N/A    |        N/A       |     protocol     |
+------------------+-----------+------------------+------------------+
|     directory    |    N/A    |        N/A       |       dir        |
+------------------+-----------+------------------+------------------+
|      access      |    N/A    |        N/A       |      access      |
+------------------+-----------+------------------+------------------+

.. note:: The `Setting Name` column will be a member name of a `SettingsElement`
   added to the `Settings` dictionary by `append_class`.

The access field contains a file address that references an ini file that
has the following format:

.. code::

   [access.details]
   userid     : unityadmin
   userpass   : unityadmin

This ini file will set the contents of `access` in the `RemoteServerSettings` object
to a dict **{ "userid":"unityadmin", "userpass":"unityadmin"}**.


.. automodule:: csmbigdata.config.server_settings
    :members:
    :noindex:

CSM Statistics Settings Element (csmbigdata.config.stat_settings)
=================================================================

+--------------------------------------------------------------------+
|                         Stat Config Options                        |
+------------------+-----------+------------------+------------------+
|   Setting Name   | CMD Short |     CMD Long     |    Config File   |
+------------------+-----------+------------------+------------------+
|   stat_options   |    -m     |    --metrics     |   stat_options   |
+------------------+-----------+------------------+------------------+
|   log_sources    |    N/A    |        N/A       |    log_sources   |
+------------------+-----------+------------------+------------------+
|   log_tags       |    N/A    |        N/A       |     log_tags     |
+------------------+-----------+------------------+------------------+
|   log_sources    |    N/A    |        N/A       |  log_sources_all |
+------------------+-----------+------------------+------------------+
|   log_tags_all   |    N/A    |        N/A       |   log_tags_all   |
+------------------+-----------+------------------+------------------+

.. note:: The `Setting Name` column will be a member name of a `SettingsElement`
   added to the `Settings` dictionary by `append_class`.

.. automodule:: csmbigdata.config.stat_settings
    :members: 
    :noindex:

.. _csm-time-settings:

CSM Time Settings Element (csmbigdata.config.time_settings)
============================================================

+--------------------------------------------------------------------+
|                       Stat Config Options                          |
+------------------+-----------+------------------+------------------+
|   Setting Name   | CMD Short |     CMD Long     |    Config File   |
+------------------+-----------+------------------+------------------+
|      date        |    -t     |   --targettime   | target_datetime  |
+------------------+-----------+------------------+------------------+
|      days        |    -d     |      --days      |     num_days     |
+------------------+-----------+------------------+------------------+

.. note:: The `Setting Name` column will be a member name of a `SettingsElement`
   added to the `Settings` dictionary by `append_class`.

.. automodule:: csmbigdata.config.time_settings
    :members: 
    :noindex:



