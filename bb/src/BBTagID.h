/*******************************************************************************
 |    BBTagID.h
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

#ifndef BB_BBTAGID_H_
#define BB_BBTAGID_H_

#include "BBJob.h"

/**
 * \class BBTagID
 * Provides the means to group a set of transfer definitions for a given job
 */
class BBTagID
{
  public:
    BBTagID() :
        job(BBJob()),
        tag(0) {
    }

    BBTagID(const BBJob pJob, const uint64_t pTag) :
        job(pJob),
        tag(pTag) {
    }

    BBTagID(const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pTag) :
        job(BBJob(pJobId, pJobStepId)),
        tag(pTag) {
    }

    void dump(char* pSev);

    inline bool operator== (const BBTagID& pOther) const {
        return (job == pOther.job && tag == pOther.tag);
    }

    inline bool operator!= (const BBTagID& pOther) const {
        return !(*this==pOther);
    }

    inline bool operator> (const BBTagID& pOther) const {
        return ((job.getJobId() == pOther.getJobId()) ? (job.getJobStepId() > pOther.getJobStepId()) : (job.getJobId() > pOther.getJobId()));
    }

    inline BBJob getJob() const {
        return job;
    }

    inline uint64_t getJobId() const {
        return job.getJobId();
    }

    inline uint64_t getJobStepId() const {
        return job.getJobStepId();
    }

    inline uint64_t getTag() const {
        return tag;
    }

    BBJob job;
    uint64_t tag;
};


/**
 * \class BBTagID_Compare
 * Defines a compare operator for BBTagID
 */
class BBTagID_Compare
{
  public:
    inline bool operator()(const BBTagID& x, const BBTagID& y) {
        return ((x.job == y.job) ? (x.tag > y.tag) :
                    (x.job.getJobId() == y.job.getJobId()) ? (x.job.getJobStepId() > y.job.getJobStepId()) :
                    (x.job.getJobId() > y.job.getJobId())); }
};

#endif /* BB_BBTAGID_H_ */
