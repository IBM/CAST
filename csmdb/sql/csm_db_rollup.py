#!/bin/python
# encoding: utf-8
#================================================================================
#   
#    csm_db_rollup.py
# 
#    © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

#================================================================================
# usage             ./csm_db_rollup.py <archive directory> # DEFAULT :/var/log/ibm/csm/archive 
# current_version   1.0
# date_created:     05-17-2019
# date_modified:    05-17-2019
#================================================================================
import os
import sys
from datetime import datetime

DEFAULT_TARGET='''/var/log/ibm/csm/archive'''

def rollupDir (directory):
    ''' Performs a rollup operation on a directory, designed to condense in to weeks of data. 
    This requires the files provided are in the format `<table>.archive.<date>.json`

    directory -- A directory containing a collection of archive files to condense (string).
    '''
    # If the directory is not present return early.
    if not os.path.exists(directory):
        print("{0} does not exist".format(directory))
        return
    
    # Create an srchhive directory as needed. 
    archiveDir="{0}/{1}".format(directory,"old")
    if not os.path.exists(archiveDir): 
        os.makedirs(archiveDir)

    # Iterate over a list of strings
    files=[f for f in os.listdir(directory) if os.path.isfile("{0}/{1}".format(directory,f))]
    for f in files:
        # Skip Hidden Files.
        if f[0] == '.':
            continue

        iFile="{0}/{1}".format(directory,f)

        sFile=f.split(".")
        weekStr=datetime.strptime(sFile[2],"%Y-%m-%d").strftime("%Y-%U")
        aFile="{0}/{1}-{2}.json".format(archiveDir, sFile[0], weekStr)

        # Get the contents first
        contents=""
        with open(iFile,'r') as inputFile:
            contents=inputFile.read()

            # remove and skip empties. 
            if len(contents) == 0:
                os.remove(iFile)
                continue

        # Archive the contents.
        with open(oFile, 'a') as ofile:
            ofile.write(contents)
            os.remove(oFile)

def main(args):
    target = DEFAULT_TARGET
    if len(sys.argv) > 1:
        target=sys.argv[0]

    rollupDir(target)

if __name__ == "__main__":
    sys.exit(main(sys.argv))
