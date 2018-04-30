#!/bin/bash
# ================================================================================
# 
#     csmd/bench/actiontest/stdout.sh
# 
#   © Copyright IBM Corporation 2015,2016. All Rights Reserved
# 
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
# 
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
# 
# ================================================================================

echo "$1";
for (( c=1; c<=5; c++ ))
do
    echo "stderr $c" 1>&2;
    echo "stdout $c";
    if [[ $c == 4 ]]; then 
	sleep 3
    fi
done
sleep 12
echo "script done";



