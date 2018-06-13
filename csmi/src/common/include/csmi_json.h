/*================================================================================

    csmi/src/common/include/csmi_json.h

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_JSON_H__
#define __CSMI_JSON_H__

#include "csmi_struct_hash.h"
#include <string>

void csmiGenerateJSON(
    std::string& json,
    const char* format,
    void* target,
    const csmi_struct_mapping_t* mapping);


#endif
