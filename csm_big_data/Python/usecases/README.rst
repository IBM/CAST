CSM Big Data Use Cases
**********************
The following package consists of a number of python modules designed for commandline operation. 
These modules are implemented using the infastructure described by the csmbigdata packages.

**Contents**

.. contents::
    :local:

Configuration (config_inis/default.ini)
=======================================

For details on the configuration settings of these usecases, please consult the following:

.. code-block:: bash

    # This will output a sample configuration file to `tool.output`
    usecase-name --helpconfig

    # This will output a sample configuration file to the specified output file 
    # relative to the user directory.
    usecase-name --helpconfig --output "output-file"

By default if a usecase does not specify a config file ( `--config {file}` ) `defualt.ini` 
will be used by the script relative to the execution path.



Find Job Time Range (usecases.find_job_time_range)
==================================================

.. automodule:: usecases.find_job_time_range
   :members:
   :noindex:

Find Job Keys (usecases.find_job_keys)
======================================
.. automodule:: usecases.find_job_keys
    :members:
    :noindex:

Find Job Metrics (usecases.find_job_metrics)
============================================
.. automodule:: usecases.find_job_metrics
    :members:
    :noindex:

Find Jobs Running (usecases.find_jobs_running)
==============================================
.. automodule:: usecases.find_jobs_running
    :members:
    :noindex:

Find Temperature Information (usecases.find_temp_info)
=========================================================
.. automodule:: usecases.find_temp_info
    :members:
    :noindex:

