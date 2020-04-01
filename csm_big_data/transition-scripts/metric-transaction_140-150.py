#!/usr/bin/python2

import os
import sys
import glob
import json
import argparse

def processLogFile(logfile, outputfile):
    ''' A function for processing log files and replacing them with the new structure.
        @timestamp => timestamp
        source => data.source

        :param logfile: The old format logfile to perform a fix operation on.
        :param outputfile: An output file to store the fix results on.
    '''

    for line in logfile:
        data=json.loads(line)
        
        # Move the source field to the data.source field.
        if 'data' in data :
            if 'source' in data :
                data['data']['source']=data.pop('source',None)
        
        # Move the @timestamp field to the timestamp field. 
        timestamp=data.pop('@timestamp',None)
        if timestamp is not None:
            data['timestamp'] = timestamp
    
        outputfile.write("{0}\n".format(json.dumps(data, ensure_ascii=False,separators=(',', ':'))))
     
def main(args):
    
    parser = argparse.ArgumentParser(
        description='''A tool for converting 1.4.0 CSM BDS logs to 1.5.0 CSM BDS logs.''')

    parser.add_argument( '-f', '--files', metavar='file-glob', dest='fileglob', default=None,
        required=True,
        help='A file glob containing the bds logs to run the fix operations on.')
    parser.add_argument( '--overwrite', action='store_true',
        help='If set the script will overwrite the old files. Default writes new file *.fixed')

    args = parser.parse_args()

    
    # Open all of the files matching the glob and run the fix.
    for name in glob.glob(args.fileglob):
        print("Now Processing: {0}".format(name))
        outputname="{0}.fixed".format(name)

        if name.endswith("fixed"):
            print("Detected 'fixed' file, skipping")
            continue

        with open(name, 'r') as logfile, open(outputname, 'w') as outputfile:
            processLogFile(logfile, outputfile)

        if args.overwrite:
            os.rename(outputname, name)
            outputname=name

        print("Processed {0}, results written to {1}".format(name, outputname))

if __name__ == "__main__":
    sys.exit(main(sys.argv))


