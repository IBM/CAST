###########################################################
#     Dockerfile
# 
#     Copyright IBM Corporation 2020, 2020. All Rights Reserved
# 
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
# 
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

ARG oslvl=8
ARG image=centos:$oslvl
ARG mpi=openmpi
ARG artkey

#########
# Base Image

FROM $image

# Repeat ARG block (as FROM directive replaces it)
ARG oslvl=8
ARG image=centos:$oslvl
ARG mpi=openmpi
ARG artkey

LABEL description="Build container for CAST images"
LABEL maintainer="Tom Gooding"

##########
# Install epel

RUN yum -y install epel-release

ENV DEVRPMS createrepo gcc gcc-c++ git perl-XML-Simple cmake3 openssl-devel wget which zlib-devel rpm-devel rpm-build libuuid-devel bzip2-devel fuse fuse-devel fuse-libs perl-JSON kernel-devel libquadmath-devel libibverbs-devel postgresql-devel pam-devel initscripts graphviz doxygen rubygems java hostname iputils

ENV RHEL7RPMS python-devel 

ENV RHEL8RPMS make rsync python2 python2-devel python3-devel boost boost-devel boost-python3-devel

##########
# Basic development environment

RUN if [ "x$oslvl" = "x7" ] ; then \
    yum -y install $DEVRPMS $RHEL7RPMS ; else \
    dnf -y install $DEVRPMS $RHEL8RPMS; fi


##########
# Install boost and DCGM for CAST build

RUN echo $artkey > /etc/yum/vars/artkey
ADD $oslvl.repo /etc/yum.repos.d/.

RUN if [ "x$oslvl" = "x7" ] ; then yum install -y cast-boost; fi
RUN yum install -y datacenter-gpu-manager


##########
# Install MPI

ENV IBM_SPECTRUM_MPI_LICENSE_ACCEPT=yes
RUN if [ "x$mpi" = "xsmpi" ] ; then yum install -y libevent ibm_smpi ibm_smpi-devel ibm_smpi_gpusupport ibm_smpi-pami_devel ibm_smpi-libgpump ibm_jsm ibm_smpi_lic_s; /opt/ibm/spectrum_mpi/lap_se/bin/accept_spectrum_mpi_license.sh; fi
RUN if [ "x$mpi" = "xopenmpi" ] ; then yum install -y openmpi openmpi-devel; fi


##########
# Create build userid and environment

RUN useradd -ms /bin/bash build
USER build
WORKDIR /home/build

CMD ["/bin/echo", "Command not specified"]
