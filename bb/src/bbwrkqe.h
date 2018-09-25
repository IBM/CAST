/*******************************************************************************
 |    bbwrkqe.h
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


#ifndef BB_BBWRKQE_H_
#define BB_BBWRKQE_H_

#include <string.h>

#include <queue>
#include <vector>

#include "bbinternal.h"
#include "LVKey.h"
#include "WorkID.h"

using namespace std;


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class BBLV_Info;
class BBTransferDef;
class Extent;
class ExtentInfo;


/*******************************************************************************
 | Constants
 *******************************************************************************/
const int DEFAULT_DUMP_QUEUE_ON_REMOVE_WORK_ITEM = 0;
const bool DO_NOT_VALIDATE_WORK_QUEUE = false;
const bool VALIDATE_WORK_QUEUE = true;


/*******************************************************************************
 | Classes
 *******************************************************************************/

//
// WRKQE class
//

class WRKQE
{
  public:
    /**
     * \brief Default Constructor
     */
    WRKQE() :
        lvKey(std::pair<std::string, Uuid>("", Uuid())),
        jobid(UNDEFINED_JOBID),
        rate(0),
        bucket(0),
        suspended(0),
        dumpOnRemoveWorkItem(DEFAULT_DUMP_QUEUE_ON_REMOVE_WORK_ITEM),
        numberOfWorkItems(0),
        numberOfWorkItemsProcessed(0) {
        init();
    };

    /**
     * \brief Constructor
     */
    WRKQE(const LVKey* pLVKey, const uint64_t pJobId) :
        lvKey(*pLVKey),
        jobid(pJobId),
        rate(0),
        bucket(0),
        suspended(0),
        dumpOnRemoveWorkItem(DEFAULT_DUMP_QUEUE_ON_REMOVE_WORK_ITEM),
        numberOfWorkItems(0),
        numberOfWorkItemsProcessed(0) {
        init();
    };

    /**
     * \brief Destructor
     */
    virtual ~WRKQE() {
        if (wrkq)
        {
            delete wrkq;
            wrkq = 0;
        }
    };

    // Inline methods
    inline int64_t getBucket()
    {
        return bucket;
    };

    inline int getDumpOnRemoveWorkItem()
    {
        return dumpOnRemoveWorkItem;
    };

    inline int64_t getJobId()
    {
        return jobid;
    };

    inline LVKey* getLVKey()
    {
        return &lvKey;
    };

    inline uint64_t getNumberOfWorkItems()
    {
        return numberOfWorkItems;
    }

    inline uint64_t getNumberOfWorkItemsProcessed()
    {
        return numberOfWorkItemsProcessed;
    }

    inline uint64_t getRate()
    {
        return rate;
    };

    inline void incrementNumberOfWorkItemsProcessed()
    {
        ++numberOfWorkItemsProcessed;

        return;
    };

    inline void incrementNumberOfWorkItems()
    {
        ++numberOfWorkItems;

        return;
    };

    inline void init()
    {
        wrkq = new queue<WorkID>;

        return;
    };

    inline int isSuspended()
    {
        return suspended;
    };

    inline queue<WorkID>* getWrkQ()
    {
        return wrkq;
    };

    inline size_t getWrkQ_Size()
    {
        return wrkq->size();
    };

    inline void setDumpOnRemoveWorkItem(const int pValue)
    {
        dumpOnRemoveWorkItem = pValue;

        return;
    };

    inline void setRate(const uint64_t pRate)
    {
        rate = pRate;

        return;
    };

    inline void setSuspended(const int pValue)
    {
        suspended = pValue;

        return;
    };

    // Methods
    void addWorkItem(WorkID& pWorkItem, const bool pValidateQueue);
    void dump(const char* pSev, const char* pPrefix);
    void removeWorkItem(WorkID& pWorkItem, const bool pValidateQueue);
    void loadBucket();
    double processBucket(BBLV_Info* pLV_Info, BBTagID& pTagId, ExtentInfo& pExtentInfo);

    // Data members
    LVKey               lvKey;
    uint64_t            jobid;
    uint64_t            rate;       // bytes/sec
    int64_t             bucket;
    int                 suspended;
    int                 dumpOnRemoveWorkItem;
    uint64_t            numberOfWorkItems;
    uint64_t            numberOfWorkItemsProcessed;
    queue<WorkID>*      wrkq;
};

#endif /* BB_BBWRKQE_H_ */
