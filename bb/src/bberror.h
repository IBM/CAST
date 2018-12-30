/*******************************************************************************
 |    bberror.h
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

#ifndef BB_BBERROR_H_
#define BB_BBERROR_H_

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "connections.h"
#include "tstate.h"
#include "bbras.h"


/*******************************************************************************
 | Macros
 *******************************************************************************/
#define addBBErrorToMsg(pMsg) \
    std::string bberrorstr = bberror.get(); \
    pMsg->addAttribute(txp::resultCode, bberrorstr.c_str(), bberrorstr.size()+1);

#define ADD(KEY,VALUE) { \
    char l_Key[512] = {'\0'}; \
    prepareKey(l_Key, KEY, sizeof(l_Key)); \
    errstate.put(l_Key, VALUE); \
}

#define ENTRY_NO_CLOCK(FILE,FUNC) { \
    bberror.addIncr(FILE, FUNC, "entry", 1, 0); \
}

#ifdef PROF_TIMING
#define ENTRY(FILE,FUNC) { \
    std::chrono::high_resolution_clock::time_point time_start = std::chrono::high_resolution_clock::now(); \
    ENTRY_NO_CLOCK(FILE,FUNC); \
}
#else
#define ENTRY(FILE,FUNC) { \
    ENTRY_NO_CLOCK(FILE,FUNC); \
}
#endif

#define EXIT_NO_CLOCK(FILE,FUNC) { \
    bberror.addIncr(FILE, FUNC, "exit", 1, 1); \
}

#ifdef PROF_TIMING
#define EXIT(FILE,FUNC) { \
    EXIT_NO_CLOCK(FILE,FUNC); \
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now(); \
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start; \
    LOG(bb,trace) << FILE <<":"<< __LINE__ << " " << FUNC << "() completed after: " << elapsed_microseconds.count() << " microseconds"; \
}
#else
#define EXIT(FILE,FUNC) { \
    EXIT_NO_CLOCK(FILE,FUNC); \
}
#endif


#ifdef PROF_TIMING
#define RESPONSE_AND_EXIT(FILE,FUNC) { \
    EXIT_NO_CLOCK(FILE,FUNC); \
    addBBErrorToMsg(response); \
    sendMessage(pConnectionName,response); \
    delete response; \
    response=NULL; \
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now(); \
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start; \
    LOG(bb,trace) << FILE <<":"<< __LINE__ << " " << FUNC << "() completed after: " << elapsed_microseconds.count() << " microseconds"; \
}
#else
#define RESPONSE_AND_EXIT(FILE,FUNC) { \
    EXIT_NO_CLOCK(FILE,FUNC); \
    addBBErrorToMsg(response); \
    sendMessage(pConnectionName,response); \
    delete response; \
    response=NULL; \
}
#endif

#define BAIL { \
    bberror << bailout; \
}

#define LOG_ERROR(TEXT) { \
    LOG(bb,error) << TEXT.str(); \
}

#define LOG_ERROR_RC(TEXT,RC) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << errloc(RC); \
}

#define LOG_ERROR_AND_BAIL(TEXT) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << bailout; \
}

#define LOG_ERROR_RC_WITH_EXCEPTION(FILE,FUNC,LINE,EXCEPTION,RC) { \
    try { \
        LOG(bb,error) << "Exception caught in file " << FILE << ", function " << FUNC << ", line " << LINE << ":" << EXCEPTION.what(); \
        bberror << err("rc", RC) << EXCEPTION; \
    } catch(std::exception& e) {} \
}

#define LOG_ERROR_RC_WITH_EXCEPTION_AND_RAS(FILE,FUNC,LINE,EXCEPTION,RC,RASID) { \
    try { \
        LOG(bb,error) << "Exception caught in file " << FILE << ", function " << FUNC << ", line " << LINE << ":" << EXCEPTION.what(); \
        bberror << err("rc", RC) << EXCEPTION << RAS(RASID); \
    } catch(std::exception& e) {} \
}

#define LOG_ERROR_WITH_EXCEPTION(FILE,FUNC,LINE,EXCEPTION) { \
    try { \
        LOG(bb,error) << "Exception caught in file " << FILE << ", function " << FUNC << ", line " << LINE << ":" << EXCEPTION.what(); \
        bberror << EXCEPTION; \
    } catch(std::exception& e) {} \
}

#define LOG_ERROR_WITH_EXCEPTION_AND_RAS(FILE,FUNC,LINE,EXCEPTION,RASID) { \
    try { \
        LOG(bb,error) << "Exception caught in file " << FILE << ", function " << FUNC << ", line " << LINE << ":" << EXCEPTION.what(); \
        bberror << EXCEPTION << RAS(RASID); \
    } catch(std::exception& e) {} \
}

#define LOG_ERROR_TEXT(TEXT) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.text", TEXT.str()); \
}

#define LOG_ERROR_TEXT_RC(TEXT,RC) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.text", TEXT.str()) << errloc(RC); \
}

#define LOG_ERROR_TEXT_AND_BAIL(TEXT) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.text", TEXT.str()) << bailout; \
}

#define LOG_ERROR_TEXT_AND_RAS(TEXT,RASID) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << RAS(RASID); \
}

#define LOG_ERROR_TEXT_ERRNO(TEXT,ERRNO) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.errno", ERRNO) << err("error.strerror", strerror(ERRNO)); \
    bberror << err("error.text", TEXT.str()) << errloc(ERRNO); \
}

#define LOG_ERROR_TEXT_ERRNO_AND_BAIL(TEXT,ERRNO) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.errno", ERRNO) << err("error.strerror", strerror(ERRNO)); \
    bberror << err("error.text", TEXT.str()) << errloc(ERRNO) << bailout; \
}

#define LOG_ERROR_TEXT_ERRNO_AND_RAS(TEXT,ERRNO,RASID) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.errno", ERRNO) << err("error.strerror", strerror(ERRNO)); \
    bberror << err("error.text", TEXT.str()) << errloc(ERRNO) << RAS(RASID); \
}

#define LOG_ERROR_TEXT_RC_AND_BAIL(TEXT,RC) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.text", TEXT.str()) << errloc(RC) << bailout; \
}

#define LOG_ERROR_TEXT_RC_AND_RAS(TEXT,RC,RASID) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.text", TEXT.str()) << errloc(RC) << RAS(RASID); \
}

#define LOG_ERROR_TEXT_RC_AND_RAS_AND_BAIL(TEXT,RC,RASID) { \
    LOG(bb,error) << TEXT.str(); \
    bberror << err("error.text", TEXT.str()) << errloc(RC) << RAS(RASID) << bailout; \
}

#define LOG_RC(RC) { \
    bberror << errloc(RC); \
}

#define LOG_RC_AND_BAIL(RC) { \
    bberror << errloc(RC) << bailout; \
}

#define LOG_RC_AND_RAS(RC,RASID) { \
    bberror << errloc(RC) << RAS(RASID); \
}

#define LOG_RC_AND_RAS_AND_BAIL(RC,RASID) { \
    bberror << errloc(RC) << RAS(RASID) << bailout; \
}

#define LOOP_COUNT(FILE,FUNC,CRUMB) { \
    bberror.addIncr(FILE, FUNC, CRUMB, 1, 1); \
}

#define UPDATE_TS(KEY) { \
    char l_Time[27] = {'\0'}; \
    getCurrentTime(l_Time, sizeof(l_Time)); \
    ADD(KEY, l_Time); \
}


/*******************************************************************************
 | Constants
 *******************************************************************************/

const std::vector<std::string> KEYS_TO_REMAIN = {"dft",
                                                 "env",
                                                 "error",
                                                 "id",
                                                 "in",
                                                 "out",
                                                 "rc",
                                                 "syscall"};


/*******************************************************************************
 | Classes
 *******************************************************************************/
class BBHandler : public TSHandler
{
    public:
        /**
         * \class BBHandlerId
         * Uniquely identifies all of the data added to bberror for a given transaction.
         */
        class BBHandlerId {
        public:
            // Non-static methods

            /**
             * \brief Get value.
             *
             * \return   int64_t Current id value
             */
            int64_t get() { return value; };

            /**
             * \brief Increments the id object.
             */
            int64_t incr() { while (++value == 0) {}; return value; };

            /**
             * \brief Locks id object.
             */
//            int lock() { return pthread_mutex_lock(&mutex); };
            int lock() { return 0; };

            /**
             * \brief Unlocks the id object.
             */
//            int unlock() { return pthread_mutex_unlock(&mutex); };
            int unlock() { return 0; };
            // Constructors

            /**
             * \brief Default constructor
             *
             * Sets the id value to zero.
             */
            BBHandlerId() :
                mutex(PTHREAD_MUTEX_INITIALIZER),
                value(0) {
            };

            /**
             * \brief Constructor with value.
             *
             * \param[in]   pValue Message number value
             */
            BBHandlerId(int64_t pValue) :
                mutex(PTHREAD_MUTEX_INITIALIZER),
                value(pValue) {
            };

            /**
             * \brief Destructor
             */
            virtual ~BBHandlerId() {};

            // Data members
            pthread_mutex_t mutex;      //! Mutex for id value
            int64_t         value;      //! Id value
        };

        // Constructor
        BBHandler() :
        TSHandler(),
        connectionName("")
        {
            ok2Clear=true;
            init();
        };

        // Destructor
        ~BBHandler() {};

        // Static data
        const char* NOMSGID = "None";

        // Static methods

        // Non-static methods
        int clear(){ return clear(std::string("")); }
        int clear(const char* name);
        int clear(const std::string& pConnectionName);
        // NOTE: Use forceClear() if you want to clear the contents of bberror and
        //       preserve the current ok2Clear value.
        void forceClear();
        void prune();
        int pruneChildren(pt::ptree& pTree, const std::string& pPath, int pDepth);
        bool ok2Clear;
        void setToNotClear(){ok2Clear=false;}
        void resetToClear(){ok2Clear=true;}

    public:
        int rc() const
        {
            return errstate.get("rc", -1);
        };

        int merge(txp::Msg* msg)
        {
            merge( (const char*)msg->retrieveAttrs()->at(txp::resultCode)->getDataPtr() );
            return rc();
        };

        int rc(txp::Msg* msg)
        {
            set( (const char*)msg->retrieveAttrs()->at(txp::resultCode)->getDataPtr() );
            return rc();
        };

        int addIncr(const char* file, const char* func, const char* crumb, signed long value, int updateExistingTS)
        {
            int rc = 0;
            char l_File[64] = {'\0'};
            char l_KeyCount[256] = {'\0'};
            char l_KeyTS[256] = {'\0'};
            char l_PreparedKey[512] = {'\0'};

            if (!connectionNameIsEmpty())
            {
                // Parse out the file name...
                char* x= strrchr(const_cast<char*>(file), '/');
                char* y = strchr(x, '.');
                memcpy(l_File, x+1, y-x-1);

                // Build the keys for the "count" and "ts" data items...
                snprintf(l_KeyCount, sizeof(l_KeyCount), "breadcrumbs.%s.%s.%s.count", l_File, func, crumb);
                snprintf(l_KeyTS, sizeof(l_KeyTS), "breadcrumbs.%s.%s.%s.ts", l_File, func, crumb);

                id.lock();

                try
                {
                    // See if the key for "count" exists...
                    prepareKey(l_PreparedKey, l_KeyCount, sizeof(l_PreparedKey));
                    signed long l_Value = errstate.get(l_PreparedKey, -1);
                    if (l_Value == -1)
                    {
                        // "count" does not exist for this key...  Add it and a "ts" data item...
                        errstate.put(l_PreparedKey, value);
                        UPDATE_TS(l_KeyTS);
                    }
                    else
                    {
                        // "count" exists for this key...
                        // Bump the value by the increment amount and update the "count" data item...
                        l_Value += value;
                        errstate.put(l_PreparedKey, l_Value);
                        // If we are to update the "ts" for an existing data item, update it now...
                        if (updateExistingTS)
                        {
                            UPDATE_TS(l_KeyTS);
                        }
                    }

//                    std::string result = get("json");
//                    LOG(bb, info) << "bberror add+: " << result;
                }
                catch(std::exception& e)
                {
                    rc = -1;
                }
                id.unlock();
            }
            else
            {
//                LOG(bb,info) << "addIncr(): Connection name is None: file=" << file << ", func=" << func << ", crumb=" << crumb << ", value=" << value << ", updateExistingTS=" << updateExistingTS;
            }

            return rc;
        };

        int connectionNameIsEmpty()
        {
            return (connectionName.empty());
        };

        void errdirect(const char* a, unsigned long b)
        {
            errstate.put(a, b);

            return;
        };

        void errdirect(const char* a, char* b)
        {
            errstate.put(a, b);

            return;
        };

        void errdirect(const char* a, std::string b)
        {
            errstate.put(a, b);

            return;
        };

        //  getCurrentTime is temporary...  Need to incorporate a more efficient timestamp...
        void getCurrentTime( char* pBuffer, const int32_t pSize )
        {
        	char l_Buffer[20] = {'\0'};

            timeval l_CurrentTime;
            gettimeofday(&l_CurrentTime, NULL);
            unsigned long l_Micro = l_CurrentTime.tv_usec;

            //localtime is not thread safe
//            strftime(l_Buffer, sizeof(l_Buffer), "%Y-%m-%d %H:%M:%S", localtime((const time_t*)&l_CurrentTime.tv_sec));
            strftime(l_Buffer, sizeof(l_Buffer), "%d %H:%M:%S", localtime((const time_t*)&l_CurrentTime.tv_sec));
            snprintf(pBuffer, pSize, "%s.%06lu", l_Buffer, l_Micro);

        	return;
        };

        void init()
        {
            clear();
        };

        // NOTE:  If the path already exists in errstate, this merge
        //        routine will replace the value at that path with
        //        the value from jsonstring.
        int merge(const std::string jsonstring)
        {
            if (id.get())
            {
//                LOG(bb, info) << "Before merge    : " << get();
//                LOG(bb, info) << "Input into merge: " << jsonstring;
                boost::property_tree::ptree l_Tree;
                std::istringstream result_stream(jsonstring);
                boost::property_tree::read_json(result_stream, l_Tree);
                BOOST_FOREACH(auto& e, l_Tree)
                {
                    //  NOTE: Don't merge the "id" data item...
                    //        Merge every other data item...
                    const char* l_Key = e.first.c_str();
//                    LOG(bb,info) << "merge: key=" << l_Key;
                    if ((strstr(l_Key, "id") == 0) || (strstr(l_Key, "id") != l_Key))
                    {
                        errstate.boost::property_tree::ptree::put_child(e.first, e.second);
                    }
                }
//                LOG(bb, info) << "After merge     : " << get();
            }

            return 0;
        };

    private:
        //*****************************************************************************
        //  Instance methods
        //*****************************************************************************
        int needToPrepareKey(char* targetkey, const char* sourcekey, size_t length)
        {
            std::string l_SourceKey = sourcekey;

            return needToPrepareKey(targetkey, l_SourceKey, length);
        }

        int needToPrepareKey(char* targetkey, const std::string sourcekey, size_t length)
        {
            int needToPrepare = 1;

            if (sourcekey.find(':') == std::string::npos)
            {
                // Not already prepared...  Now check for all first level keys...
                for (size_t i=0; needToPrepare && i<KEYS_TO_REMAIN.size(); ++i)
                {
                    if ((sourcekey.compare(KEYS_TO_REMAIN[i]) == 0))
                    {
                        // First level key...  Do not prepare...
                        needToPrepare = 0;
                    }
                }
            }
            else
            {
                // Already prepared...
                needToPrepare = 0;
            }

            return needToPrepare;
        }

        void prepareKey(char* targetkey, const char* sourcekey, size_t length)
        {
            if (needToPrepareKey(targetkey, sourcekey, length))
            {
                snprintf(targetkey, length, "%s:%s.%s", hostname, connectionName.c_str(), sourcekey);
            }
            else
            {
                strCpy(targetkey, sourcekey, length);
            }

            return;
        };

        void prepareKey(char* targetkey, const std::string sourcekey, size_t length)
        {
            if (needToPrepareKey(targetkey, sourcekey, length))
            {
                snprintf(targetkey, length, "%s:%s.%s", hostname, connectionName.c_str(), sourcekey.c_str());
            }
            else
            {
                strCpy(targetkey, sourcekey.c_str(), length);
            }

            return;
        };

        // Instance data (private)
        BBHandlerId id;
        std::string connectionName;
};


/*******************************************************************************
 | External data
 *******************************************************************************/
extern thread_local BBHandler bberror;

#endif /* BB_BBERROR_H_ */
