#!/bin/bash
# encoding: utf-8
# ================================================================================
#
# quick_config.sh
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

# The Log Analysis Linux User
Log_Analysis_User="ioala"

# The DSV Properties directory.
props_dir="DSV_props"

# The Log analysis GUI Admin
Unity_User="unityadmin"
Unity_Pass="unityadmin"


# The install directory of Log Analysis
Log_Analysis_Home="/opt/IBM/ioala/LogAnalysis/"
DSV_ToolKit=${Log_Analysis_Home}"unity_content/DSVToolkit*"

# Copy over the props_dir
cp -r ${props_dir} ${DSV_ToolKit}

# Replace the scala home field to match Log Analysis Home
cd ${DSV_ToolKit}/${props_dir}
sed -i "s;scalaHome:.*;scalaHome: "${Log_Analysis_Home}";g" *
chown -R ${Log_Analysis_User}:users ${DSV_ToolKit}

# Run the Insight Pack generator, executes as ioala
cd ${DSV_ToolKit}
for file in $(ls -1 ${props_dir}/*)
do
    su ${Log_Analysis_User} -c "python dsvGen.py ${file} -d -f -o -u ${Unity_User} -p ${Unity_Pass}"
done

