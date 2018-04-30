#! /bin/bash
#================================================================================
#
#    csmi/src/launch/examples/launch_env.sh
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

# Add Spectrum MPI bin location to PATH
export PATH=$PATH:/opt/ibm/spectrum_mpi/bin

# Add Spectrum PMIX bin location to PATH
export PATH=$PATH:/opt/ibm/spectrum_mpi/jsm_pmix/bin

# Add CSM API command bin location to PATH
# This path must be modified to point to the correct location in your sandbox
export PATH=$PATH:/opt/ibm/csm/sbin:/opt/ibm/csm/bin

# Add Spectrum MPI libraries to the LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ibm/spectrum_mpi/lib

# Add Spectrum PMIX libraries to the LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ibm/spectrum_mpi/jsm_pmix/lib

# Add CSM API library location to LD_LIBRARY_PATH
# This path must be modified to point to the correct location in your sandbox
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ibm/csm/lib

# Tell PMIx where to find Spectrum MPI
export OPAL_PREFIX=/opt/ibm/spectrum_mpi
export OPAL_LIBDIR=$OPAL_PREFIX/lib

# Configure MPI to Run over TCP only
export OMPI_MCA_pml=ob1
export OMPI_MCA_osc=^pami
export OMPI_MCA_btl=self,vader,sm,tcp

# PMIx Debug Flags
#export PTS_LOG_ENABLE=yes
#export PTS_LOG_INTERNAL=yes
#export PMIX_LOG_ENABLE=yes
#export PMIX_LOG_INTERNAL=yes

# Export CLUSTER variable to make the CSM VM document easier to copy and paste from
export CLUSTER=c931fXXpYY

# Add LSF environment configuration
. /ugadmin/csmvm/lsf/c931fXXpYY/conf/profile.lsf
