#!/bin/bash
#================================================================================
#
#    csmd/extract_csmdb_version.sh
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


# WARNING:
# currently a very error prone process because we're "assuming" a particular format of the input file
# it's currently a csv file with the version number in the first column of a single non-comment line
# in that file.  values are not enclosed in quotes

# grep for THE non-comment line in the file and print the first column
awk 'BEGIN{ FS=",";}/^ *[^#]/{ print $1; }' $1
