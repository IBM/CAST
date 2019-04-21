/*******************************************************************************
 |    BBJob.h
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

#ifndef BB_BBJOB_H_
#define BB_BBJOB_H_

#include <iostream>
#include <sstream>

#include <boost/serialization/serialization.hpp>

#include "bbinternal.h"

using namespace std;

/*******************************************************************************
 | Constants
 *******************************************************************************/
const uint32_t ARCHIVE_BBJOB_VERSION = 1;

/**
 * \class BBJob
 * Provides the grouping for a jobid and jobstepid
 */
class BBJob
{
  public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        serializeVersion = pVersion;
        pArchive & serializeVersion;
        pArchive & objectVersion;
        pArchive & jobid;
        pArchive & jobstepid;

        return;
    }

    BBJob() :
        serializeVersion(0),
        objectVersion(ARCHIVE_BBJOB_VERSION),
        jobid(UNDEFINED_JOBID),
        jobstepid(UNDEFINED_JOBSTEPID) {
    }

    BBJob(const uint64_t pJobId) :
        serializeVersion(0),
        objectVersion(ARCHIVE_BBJOB_VERSION),
        jobid(pJobId),
        jobstepid(UNDEFINED_JOBSTEPID) {
    }

    BBJob(const uint64_t pJobId, const uint64_t pJobStepId) :
        serializeVersion(0),
        objectVersion(ARCHIVE_BBJOB_VERSION),
        jobid(pJobId),
        jobstepid(pJobStepId) {
    }

    BBJob(const BBJob& src) :
        serializeVersion(src.serializeVersion),
        objectVersion(src.objectVersion),
        jobid(src.jobid),
        jobstepid(src.jobstepid) {
    }

    void dump(const char* pSev);
    void getStr(stringstream& pSs);

    inline bool operator== (const BBJob& pOther) const {
        return (jobid == pOther.jobid && jobstepid == pOther.jobstepid);
    }

    inline bool operator!= (const BBJob& pOther) const {
        return !(*this==pOther);
    }

    inline uint64_t getJobId() const {
        return jobid;
    }

    inline uint64_t getJobStepId() const {
        return jobstepid;
    }

    inline uint64_t getObjectVersion() {
        return objectVersion;
    }

    inline uint64_t getSerializeVersion() {
        return serializeVersion;
    }

    uint32_t serializeVersion;
    uint32_t objectVersion;
    uint64_t jobid;
    uint64_t jobstepid;
};

#endif /* BB_BBJOB_H_ */
