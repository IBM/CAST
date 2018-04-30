# encoding: utf-8
#================================================================================
#
#    lib_csm_bb_py.py
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
def __bootstrap():
    global __bootstrap__, __loader__, __file__
    import sys, pkg_resources, imp
    __file__ = pkg_resources.resource_filename(__name__,'lib_csm_bb_py.py')
    __loader__ = None; del __bootstrap__, __loader__
    imp.load_dynamic(__name__,__file__)
__bootstrap__()

