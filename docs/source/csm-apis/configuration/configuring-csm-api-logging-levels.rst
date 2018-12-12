.. _CSMAPILog:

Configuring CSM API Logging Levels
==================================

CSM has lots of things that print out to the logs. Some things are printed at different log levels. 
You can configure CSM APIs to switch between these log levels. Logging is handled through the CSM 
infrastructure and divided into two parts, “Front end” and “back end”.

“Front end” is supposed to represent the part of the API a user would interface with and before an 
API connects and goes into the CSM infrastructure. “Back end” refers to the part of an API that the 
user would not interact with and after an API connects and goes into the CSM infrastructure. 

Front end logging
-----------------

Front end logging is done through the csm logging utility. You will need to include the header 
file to call the function.

.. code-block:: c

    #include "csmutil/include/csmutil_logging.h"

Set the log level with this function:

.. code-block:: c

    csmutil_logging_level_set(my_level);


Where `my_level` is either:
- off
- trace
- debug
- info
- warning
- error
- critical
- always
- disable

After this function is called, the logging level will change. For example, below we set the 
logging level to `error`. So none of these logging calls will print. When we call the API at the 
end, then only prints that are at level `error` and above will print.

.. code-block:: none

    csmutil_logging_level_set(“error”);
    
    // This will print out the contents of the struct that we will pass to the api
    csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
    csmutil_logging(debug, "  Preparing to call the CSM API...");
    csmutil_logging(debug, "  value of input:    %p", input);
    csmutil_logging(debug, "  address of input:  %p", &input);
    csmutil_logging(debug, "  input contains the following:");
    csmutil_logging(debug, "    comment:          %s", input->comment);
    csmutil_logging(debug, "    limit:            %i", input->limit);
    csmutil_logging(debug, "    node_names_count: %i", input->node_names_count);
    csmutil_logging(debug, "    node_names:       %p", input->node_names);
    for(i = 0; i < input->node_names_count; i++){
        csmutil_logging(debug, "      node_names[%i]: %s", i, input->node_names[i]);
    }
    csmutil_logging(debug, "    offset:           %i", input->offset);
    csmutil_logging(debug, "    type:             %s", csm_get_string_from_enum(csmi_node_type_t, input->type) );

    /* Call the C API. */
    return_value = csm_node_attributes_query(&csm_obj, input, &output);

If we called the same function, but instead passed in `debug`, then all those logging calls 
would print, and when we call the API at the end, all prints inside the API that were set to 
level `debug` and above would print.
CSM API wrappers such as the CMD Line interfaces include access to this function via the 
`–v, --verbose` field on the cmd line parameters. 


Back end logging
----------------

APIs incorporate the CSM daemon logging system, under the sub channel of `csmapi`.  If you want to 
change the level of default API logging, then you must configure the field in the appropriate csm 
daemon config file. `csmapi` is the field you would need to change. It is found in all the CSM 
daemon config files, under the `csm` level, then under sub level `log`.

An excerpt of the `csm_master.cfg` is reproduced below as an example.

.. code-block:: javascript
    
    "csm" :
    {
        "log" :
        {
            "format"                    :   "%TimeStamp% %SubComponent%::%Severity% | %Message%",
            "consoleLog"                :   false,
            "fileLog"                   :   "/var/log/ibm/csm/csm_master.log",
            "__rotationSize_comment_1"  :   "Maximum size (in bytes) of the log file, 10000000000 bytes is ~10GB",
            "rotationSize"              :   10000000000,
            "default_sev"               :   "warning",
            "csmdb"                     :   "info",
            "csmnet"                    :   "info",
            "csmd"                      :   "info",
            "csmras"                    :   "info",
            "csmapi"                    :   "info",
            "csmenv"                    :   "info"
        }
    }


An example of editing this field from `info` to `debug` is shown below.

.. code-block:: javascript

    "csm" :
    {
        "log" :
        {
            "format"                    :   "%TimeStamp% %SubComponent%::%Severity% | %Message%",
            "consoleLog"                :   false,
            "fileLog"                   :   "/var/log/ibm/csm/csm_master.log",
            "__rotationSize_comment_1"  :   "Maximum size (in bytes) of the log file, 10000000000 bytes is ~10GB",
            "rotationSize"              :   10000000000,
            "default_sev"               :   "warning",
            "csmdb"                     :   "info",
            "csmnet"                    :   "info",
            "csmd"                      :   "info",
            "csmras"                    :   "info",
            "csmapi"                    :   "debug",
            "csmenv"                    :   "info"
        }
    }


If you have trouble finding the config files, then daemon config files are located:
- source repo: "bluecoral/csmconf/”
- ship to: “/opt/ibm/csm/share/”
- run from: “etc/ibm/csm/”

*Note:* You may need to restart the daemon for the logging level to change.

If you want to make a run time change to logging, but don’t want to change the configuration file. 
You can use this tool found it here: `opt/ibm/csm/sbin/csm_ctrl_cmd`

You must run this command on the node with the CSM Daemon that you would like to change the logging level of. 

