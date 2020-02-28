/*******************************************************************************
 |    HandleInfo.h
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

#ifndef BB_HANDLEINFO_H_
#define BB_HANDLEINFO_H_

#include <sstream>
#include <vector>

#include <stdio.h>
#include <string.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "bbinternal.h"


namespace bfs = boost::filesystem;


/*******************************************************************************
 | Forward declarations
 *******************************************************************************/


/*******************************************************************************
 | Constants
 *******************************************************************************/
const char LOCK_HANDLE_BUCKET_FILENAME[] = "^lockfile";
const char HANDLEINFONAME[] = "^handleinfo_";
const int MAXIMUM_HANDLEINFO_LOADTIME = 10;     // In seconds


/*******************************************************************************
 | External methods
 *******************************************************************************/


/*******************************************************************************
 | External data
 *******************************************************************************/
extern thread_local int HandleBucketLockFd;
extern pthread_mutex_t* HandleBucketMutex;


/*******************************************************************************
 | Classes
 *******************************************************************************/

class HandleInfo
{
public:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& pArchive, const uint32_t pVersion)
    {
        pArchive & filename;
        pArchive & handles;

        return;
    }

    HandleInfo(const std::string pFileName) :
        filename(pFileName) {
        handles = vector<uint64_t>();
    }

    HandleInfo() :
        filename("") {
        handles = vector<uint64_t>();
    }

   /*
     * Static methods
     */
    static int createLockFile(const string& pFilePath);
    static int load(HandleInfo* &pHandleInfo, const bfs::path& pHandleInfoName);
    static int lockHandleBucket(const bfs::path& l_HandleInfoPath, const uint64_t pHandleBucketNumber);
    static void unlockHandleBucket(const uint64_t pHandleBucketNumber);
    static void unlockHandleBucket(const int pFd);

    /*
     * Inlined methods
     */
#if 0
    inline void dump(const char* pPrefix)
    {

    }
#endif

    /*
     * Non-static methods
     */
    int save();

    /*
     * Data members
     */
    std::string filename;
    vector<uint64_t> handles;
};

#endif /* BB_HANDLEINFO_H_ */
