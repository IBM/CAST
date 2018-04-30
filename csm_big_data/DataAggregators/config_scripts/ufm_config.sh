#!/bin/bash
# encoding: utf-8
#================================================================================
#
#   ufm_config.sh
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
# Either use the default config or modify whatever config the user supplies.
CONFIG_FILE="/opt/ufm/files/conf/gv.cfg"
config_file=$CONFIG_FILE

if [ $# -gt 0 ]
then
    config_file=$1
fi

# Backup the original in case of catastrophic failure
config_orig=$config_file".orig"
rm -f $config_orig
cp $config_file $config_orig


# Replace the config settings.
awk -F'=' '
    BEGIN {
        section=""
    }

    /^\[.*\]/{
        section=$1
        print $0
        next
    }
    /^level/{ 
        print $1"= INFO" 
        next
    }

    /^syslog /{
        if ( section == "[Logging]" ){
            print $1"= true"
            next
        }
    }
    
    /^event_syslog/{
        print $1"= true"
        next
    }
    
    /^syslog_level/{
        print $1"= INFO"
        next
    }
    
    /^write_interval/{
        print $1"= 30"
        next
    }
    
    /^ext_ports_only/{
        print $1"= yes"
        next
    }
    
    /^max_files/{
        print $1"= 10"
        next
    }
    
    /^history_configured/{
        print $1"= true"
        next
    }
    
    
    { 
        print $0
    }' $config_file > tmp && mv tmp $config_file
