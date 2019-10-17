#================================================================================
#   
#    README.md
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

# CSM_TEST
Git repo for CSM automated regression testing

# Setup and Config
1) Create a file named "csm_fvt.csm_test.cfg" based on the csm_test.cfg.template file shipped with the CAST repo in the CAST/csm_fvt/ directory
2) Run tools/csm_uninstall.sh to uninstall any rpms currently installed on the nodes specified in the csm_fvt.csm_test.cfg file
3) Run tools/csm_install.sh to install the RPMs specificed in the csm_fvt.csm_test.cfg file on to the appropriate nodes as specified in the csm_fvt.csm_test.cfg file

# Results
The LOG_PATH parameter of csm_fvt.csm_test.cfg will indicate the parent results directory.  Within that directory, the following subdirectory structure is required
buckets/
buckets/basic/
buckets/advanced/
buckets/error_injection/
buckets/timing/
buckets/BDS/
setup/

# SQL Files
The sql files in the CAST/csm_fvt/include/sql/ directory are used to insert dummy data in to the database for several regression buckets.  These files must be copied to a postgres user accessible directory, specified by the SQL_DIR parameter in the csm_fvt.csm_test.cfg file.  Additionally ssd.sql must be editted so that node_name is the same as the SINGLE_COMPUTE parameter specified in the csm_fvt.csm_test.cfg file.  

# Running Regression
Running tools/complete_fvt.sh will start a complete regression run.  Additionally, individual buckets can be run by simply running buckets/<bucket type>/<bucket name>.sh.  NOTE: Certain buckets are currently dependent on the exit conditions of other buckets, which can lead to false-negative failures.  Refer to the tools/complete_fvt.sh script for the "correct" ordering of buckets
