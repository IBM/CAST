/*******************************************************************************
 |    BBTagID.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "BBTagID.h"
#include "logging.h"

//
// BBTagID class
//

void BBTagID::dump(char* pSev) {
    stringstream l_Job;
    job.getStr(l_Job);
    if (!strcmp(pSev,"debug")) {
        LOG(bb,debug) << "TagID -> Job(" << l_Job.str() << ")"; \
        LOG(bb,debug) << hex << uppercase << setfill('0'); \
        LOG(bb,debug) << "    Tag: 0x" << setw(4) << tag; \
        LOG(bb,debug) << setfill(' ') << nouppercase << dec; \
    } else if (!strcmp(pSev,"info")) {
        LOG(bb,info) << "TagID -> Job(" << l_Job.str() << ")"; \
        LOG(bb,info) << hex << uppercase << setfill('0'); \
        LOG(bb,info) << "    Tag: 0x" << setw(4) << tag; \
        LOG(bb,info) << setfill(' ') << nouppercase << dec; \
    }

    return;
}
