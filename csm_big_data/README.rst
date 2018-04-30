CSM Big Data documentation
**************************
Welcome to the CSM Big Data Store documentation. This documentation outlines the configuration and use of the Big Data Store.


.. note:: In the following documentation, examples requiring replacement will be annotated with the bash style 
    `${variable_name}` and followed by an explanation of the variable.

Big Data Store Documentation Compilation
========================================

Included with this documentation is a `configure_sphinx.sh` script. This script will configure 
a `Sphinx` configuration file, make file and add a **Docs** directory for documentation output.
For details on installing the sphinx documentation tools please visit `Sphinx Installation`_


.. _Sphinx Installation: http://www.sphinx-doc.org/en/1.4.8/install.html

RPM Details
===========
The rpm this README is included with has the following root directory structure:

* DataAggregators
* LogAnalysis
* Logstash
* Python
* RAS
* Hadoop

For more details on the preceding packages, 

Big Data Store Configuration
============================
The Big Data Store represents the aggregation of a number of different data sources into one queryable data store.

It is recommended that the following topics are reviewed in the order presented in the table of contents below:


.. toctree::
   :maxdepth: 2
   :caption: BDS Table of Contents

   LogAnalysis/README.rst
   Logstash/README.rst
   DataAggregators/README.rst
   Hadoop/README.rst



Python 
======
The Python directory contains a number of packages that facilitate querying the 
various Big Data Store targets setup by the preceding Configuration directories.

.. toctree::
    :maxdepth: 3
    :caption: Python Table of Contents

    Python/README.rst


