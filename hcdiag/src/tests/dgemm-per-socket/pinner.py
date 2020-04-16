#!/usr/bin/python2
# -*- coding: utf-8 -*-                   # remove when Python 3
#================================================================================
#
#    hcdiag/src/tests/dgemm-per-socket/pinner.py
#
#  © Copyright IBM Corporation 2015-2020. All Rights Reserved
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
#   Usage:
#
#       /opt/ibm/spectrum_mpi/jsm_pmix/bin/jsrun \
#           --rs_per_host 22 \
#           --env PINNER_DEBUG=0 --env PINNER_SOCKET=0 \
#           ./pinner.py APPLICATION
#
#   Where:
#
#       PINNER_DEBUG: (optional) print command and other information
#           0 = off | 1 = on
#       PINNER_SOCKET: (required) socket to use
#           0 = first | 1 = second
#

import os
import sys

# Turn on debugging mode
envar = 'PINNER_DEBUG'
if envar in os.environ:
    debug = bool(int(os.environ[envar]))
else:
    debug = False

# Get MPI rank
envar = 'PMIX_RANK'
if envar not in os.environ:
    print 'error: environment variable "%s" not found' % (envar)
    exit(1)
rank = int(os.environ[envar])

# Get socket
envar = 'PINNER_SOCKET'
if envar not in os.environ:
    print 'error: environment variable "%s" not found' % (envar)
    exit(1)
socket = int(os.environ[envar])

# Determine core
ppn   = 44 / 2      # half of cores are available 
local = rank % ppn  # local rank
cps   = 44          # cores per socket
smt   = 4           
cpu   = (socket * cps / 2 * smt) + local * smt

# Run command
cmd  = 'taskset -c %d ' % (cpu)
cmd += ' '.join(sys.argv[1:])
if debug:
    print 'rank %05d socket %01d ppn %03d local %05d cpu %03d -> %s' % (\
            rank,
            socket,
            ppn,
            local,
            cpu,
            cmd)
os.system(cmd)

