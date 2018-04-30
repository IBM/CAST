#!/bin/bash
#================================================================================
#
#    csmrestd/tests/test_loop_py.sh
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
#================================================================================

#
# simple loop to measure the overhead of doing 10000 calls 
# and connecting and sending to the csmrestapi
#

for (( c=0; c<=100; c++ ))
do
    ./py_one.py
done



