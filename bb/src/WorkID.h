/*******************************************************************************
 |    WorkID.h
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

#ifndef BB_WORKID_H_
#define BB_WORKID_H_


#include "Uuid.h"

#include "LVKey.h"
#include "BBTagID.h"

/**
 * \class WorkID
 * Defines a unit of work when transfer by_extent
 */
class WorkID
{
  public:
    WorkID() :
        key(LVKey()),
        id(BBTagID()),
        lvinfo(0) {}

    WorkID(const LVKey& pKey, BBLV_Info* pLVInfo, const BBTagID& pId) :
        key(pKey),
        id(pId),
        lvinfo(pLVInfo) {}

    inline void dump(const char* pSev, const char* pPrefix=0)
    {
        if (!pPrefix)
        {
            if (!strcmp(pSev,"debug"))
            {
                LOG(bb,debug) << key << ", JobId: " << id.getJobId() << ", JobStepId: " << id.getJobStepId() << ", tag: " << id.getTag();
            }
            else if (!strcmp(pSev,"info"))
            {
                LOG(bb,info) <<  key << ", JobId: " << id.getJobId() << ", JobStepId: " << id.getJobStepId() << ", tag: " << id.getTag();
            }
        }
        else
        {
            if (!strcmp(pSev,"debug"))
            {
                LOG(bb,debug) << pPrefix << key << ", JobId: " << id.getJobId() << ", JobStepId: " << id.getJobStepId() << ", tag: " << id.getTag();
            }
            else if (!strcmp(pSev,"info"))
            {
                LOG(bb,info) << pPrefix << key << ", JobId: " << id.getJobId() << ", JobStepId: " << id.getJobStepId() << ", tag: " << id.getTag();
            }
        }

        return;
    }

    inline string getConnectionName() const {
        return key.first;
    }

    inline BBJob getJob() const {
        return id.getJob();
    }

    inline Uuid getLVUuid() const {
        return key.second;
    }

    inline uint64_t getTag() const {
        return id.getTag();
    }

    inline BBTagID getTagId() const {
        return id;
    }

    inline LVKey getLVKey() const {
        return key;
    }

    inline BBLV_Info* getLV_Info() const {
        return lvinfo;
    }

  private:
    LVKey           key;
    BBTagID         id;
    BBLV_Info*      lvinfo;
};

#endif /* BB_WORKID_H_ */
