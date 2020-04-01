#!/usr/bin/python2
# encoding: utf-8
#================================================================================
#
#    bb_cmd.py   
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

#import wm_structs
#import wm
import sys
#sys.path.append('.')
sys.path.append('/u/jdunham/bluecoral/bluecoral/work/csm/lib')

import lib_csm_py as csm
import lib_csm_bb_py as bb
from pprint import pprint
import argparse

parser = argparse.ArgumentParser(
    description='''A tool for parsing burst buffer''')

parser.add_argument( '-n', metavar='node', dest='nodes', nargs='*', default=None,
    help='A list of hostnames to filter results to.')

args = parser.parse_args()

csm.init_lib()

bb_cmd_input=bb.bb_cmd_input_t()
bb_cmd_input.set_node_names(args.nodes)

rc,handler,output=bb.cmd(bb_cmd_input)

print(output.command_output)
csm.api_object_destroy(handler)

csm.term_lib()
