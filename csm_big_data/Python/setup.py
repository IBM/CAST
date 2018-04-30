#!/bin/python
# encoding: utf-8
# ================================================================================
#
# setup.py
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
# ================================================================================

import sys

from setuptools import setup, find_packages

if sys.version_info[0] ==2:
    setup(
        name="csmbigdata",
        version="0.1.1",
        description="A collection of modules for interfacing with the LogAnalysis Big Data Store",
        long_description=open('README.rst').read(),
        scripts=[
            'usecases/csm_bds_context.py',
            'usecases/find_job_time_range.py',
            'usecases/find_jobs_running.py',
            'usecases/find_job_metrics.py',
            'usecases/find_temp_info.py',
            'usecases/find_job_keys.py'],
        install_requires=[
            'elasticsearch>= 6.2.0,<7.0.0',
            'configparser >= 3.5.0', 
            'PyYaml >= 3.11',
            'numpy >= 1.7.1',
            'python-dateutil >= 1.5',
            'PyHive >= 0.2.1',
            'requests >= 2.13.0',
            'thrift >= 0.10.0',
            'thrift_sasl >= 0.2.1'],
        packages=find_packages(exclude=['tests','usecases','docs'])
    )
elif sys.version_info[0] == 3:
   sys.exit("python 3 is currently not supported by csmbigdata")
