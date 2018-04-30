CSM Big Data Python Packages
****************************

:Python Version: 2.7.x

.. toctree::
    :caption: Table of Contents
    :maxdepth: 3
    :name: pythontoc

    csmbigdata/README.rst
    usecases/README.rst
    
Quick Configuration
===================

A `setup.py` script has been shipped with the csmbigdata and usecases source 
directories for ease of installation.

.. warning:: `setup.py` uses pip to install the necessary pacakges. If using 
   2.7.9 or higher, pip should be installed. If 2.7.8 or lower, please consult the 
   `pip installation`_ documentation.  

.. warning:: Ensure that `python-setuptools.noarch` is installed, before running `setup.py`.

To install these packages for use please follow the following steps:

1. Install `libyaml`, `numpy`, `sasl`, `gcc-c++`, and `python-devel`.

    .. note:: This can be installed on Red Hat 7.2+ with `yum install -y libyaml numpy cyrus-sasl-devel.ppc64le cyrus-sasl-plain.ppc64le gcc-c++ python-devel`.

2. Run `setup.py install`

This should:

* Place the usecases in the `/usr/bin` directory.
* Add a `csmbigdata-0.1.0-py2.7.egg` directory to `${python home}/site-packages`.
* Install the packages noted in `Required Packages`_


For documentation, a sphinx configuration is supplied in the 
`/opt/ibm/csm/bigdata` directory.

.. _pip installation: https://pip.pypa.io/en/stable/installing/


Where to Run?
=============

This collection of usecases is designed to be run anywhere in your cluster.
It may be necessary to install the appropriate packages mentioned in `Quick Configuration`_.
Running these usecases on a node without the csm daemon running will reduce
the feature set of the usecases. To take full advantage of these tools, please run them
on either the login, utility or management node.

Required Packages
=================

The following pakages will be installed with the `setup.py install` directive:

:configparser: 
    :Version: 3.5.0 +
    :Used By: `usecases`, `csmbigdata.config`
    :Purpose: To load the configuration files for the shipped use cases.

:numpy:
    :Version: 1.7.1 +
    :Used By: `usecases`
    :Purpose: To carry out statistical operations for the metrics use case.

:PyYaml:
    :Version:
    :Used By: `csmbigdata`
    :Purpose: To parse the output of CSM APIs.
    :Warning: **Requires `libyaml` be installed.**

:python-dateutil:
    :Version: 1.5 +
    :Used By: `usecases`, `csmbigdata`
    :Purpose: To assist in storing, parsing and modifying dates.

:Pyhive:
    :Version: 0.2.1 +
    :Used By: `csmbigdata.utils`
    :Purpose: Facilitates communication with the Hive Server.

:requests:
    :Version: 2.13.0 +
    :Used By: `csmbigdata.utils`
    :Purpose: PyHive Dependency.

:thrift:
    :Version: 0.10.0 +
    :Used By: `csmbigdata.utils`
    :Purpose: PyHive Dependency.

:thrift_sasl:
    :Version: 0.2.1 +
    :Used By: `csmbigdata.utils`
    :Purpose: PyHive Dependency.
    :Warning: **Requires `rus-sasl-devel` be installed**



