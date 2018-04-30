#================================================================================
#
#    csmi/include/struct_generator/sql_extractor.sh
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

#!/bin/bash

OUTPUT_DIR="../csm_types/table_defs"
TARGET_FILE="../../../csmdb/sql/csm_create_tables.sql"
JSON_FILE="${OUTPUT_DIR}/tables.json"

./sql_extractor.awk ${TARGET_FILE} > ${JSON_FILE}

./sql_funct_generator.py ${JSON_FILE}













