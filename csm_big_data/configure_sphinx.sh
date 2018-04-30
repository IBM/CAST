#!/bin/bash
# encoding: utf-8
# ================================================================================
#
# configure_sphinx.sh
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
# ================================================================================

AUTHOR="IBM Corporation"
VERSION="0.2"
RELEASE="0.2"
PACKAGE_NAME="CSM_Big_Data"
FLAGS="--ext-autodoc --ext-viewcode --ext-githubpages --makefile --no-batchfile --ext-doctest --ext-coverage -q"
MASTER_DOC="README"
SUFFIX=".rst"
LANGUAGES="en"
DIRECTORY="Docs"

README_DOC=${MASTER_DOC}${SUFFIX}

# Clean up the directory.
mkdir -p ${DIRECTORY}
rm -rf ${DIRECTORY}/*

# Quickstart
sphinx-quickstart -p ${PACKAGE_NAME} -a "${AUTHOR}" -v ${VERSION} -r ${RELEASE} \
    ${FLAGS} --master=$MASTER_DOC --suffix=${SUFFIX} -l ${LANGUAGES}  ${DIRECTORY}

# Change to the directory.
cd ${DIRECTORY}

# Config file modification.
cat << EOF >> temp_conf.py
import os
import sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), 'Python/')))
autodoc_member_order = 'bysource'
latex_elements = { 
    'maxlistdepth' : 6,
    'preamble' : '''\usepackage{enumitem} \setlistdepth{2000} \renewlist{itemize}{itemize}{20}'''
}
EOF
sed -n '/^[ ]*\(#.*\|$\)/!p' conf.py >> temp_conf.py
sed -e 's:_build:Docs/:g' \
    -e 's:alabaster:classic:g' \
    -e "s:\(.*'\)\(_[st][^']*'.*\):\1Docs/\2:g" temp_conf.py > ../conf.py 

# Modify the Makefile.
sed 's:_build:Docs/_build:g' Makefile > ../Makefile

rm -f conf.py Makefile temp_conf.py ${README_DOC}

cd ..
# Tell the user it's all done:
echo "========================================================================"
echo "Documentation is now ready to be built by executing the make command at:"
pwd
echo 
echo "Build output will be sent to:"
echo $(pwd)/Docs
