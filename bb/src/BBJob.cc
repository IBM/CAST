/*******************************************************************************
 |    BBJob.cc
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

#include "BBJob.h"
#include "logging.h"

//
// BBJob class
//

void BBJob::dump(const char* pSev) {
    stringstream l_Job;
    getStr(l_Job);
    if (!strcmp(pSev,"debug"))
    {
//        LOG(bb,debug) << " sVrsn: " << serializeVersion << " oVrsn: << objectVersion;
        LOG(bb,debug) << l_Job.str();
    }
    else if (!strcmp(pSev,"info"))
    {
//        LOG(bb,info) << " sVrsn: " << serializeVersion << " oVrsn: << objectVersion;
        LOG(bb,info) << l_Job.str();
    }

    return;
}

void BBJob::getStr(stringstream& pSs) {
    pSs << "(" << jobid << "," << jobstepid << ")";
    return;
}

