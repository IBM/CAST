/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/database_helpers.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#ifndef _DATABASE_HELPERS_H_
#define _DATABASE_HELPERS_H_

namespace csm {
namespace daemon {
namespace helper {

/**
 * @brief Convert the tuples in dbRes to a vector of tuple
 * 
 * @param[in] dbRes A pointer to a DBResult
 * @param[out] tuples A vector of csm::db::DBTuple pointers
 * 
 * @return Return false if dbRes does not have success status
 */
inline bool GetTuplesFromDBResult(
        csm::db::DBResult_sptr dbRes, 
        std::vector<csm::db::DBTuple *>& tuples)
{
    if (dbRes->GetResStatus() != csm::db::DB_SUCCESS) 
        return false;
  
    int nrows = dbRes->GetNumOfTuples();

    tuples.resize(nrows);
    for (int i=0;i<nrows;i++) 
    {
        tuples[i] = dbRes->GetTupleAtRow(i);
    }
  
    return true;
}

/** @attention DOCUMENT! */
inline bool InspectDBResult(
    csm::db::DBRespContent &dbResp, 
    int &errcode, 
    std::string& errmsg)
{
    csm::db::DBResult_sptr dbRes = dbResp.GetDBResult();
    // dbRes == nullptr may no longer happen. The dbMgr will not forward nullptr to handlers
    // as it may be due to db connection. dbMgr will leave the request in the queue and try later.
    if (dbRes == nullptr || dbRes->GetResStatus() != csm::db::DB_SUCCESS)
    {
        errcode = CSMERR_DB_ERROR;
        if (dbRes == nullptr)
            errmsg.append( "No Database Connection in Local Daemon" );
        else
            errmsg.append( dbRes->GetErrMsg() );
        return false;
    }
    else
        return true;

}


/**  @attention UPDATE DOCUMENT!
 * @brief Check if aEvent (DBRespEvent) has accessed the DB successfully.
 * @note aEvent has to be a DBRespEvent
 * 
 * @param[in] aEvent a DBRespEvent
 * @param[out] errcode The errcode if dbRes is not valid
 * @param[out] errmsg The errmsg if dbRes is not valid
 * 
 * @return Return true if aEvent is a DBRespEvent and it contains a valid DB result.
 */
inline bool InspectDBResult(
        const csm::daemon::CoreEvent &aEvent, 
        int &errcode, std::string& errmsg)
{
    csm::daemon::DBRespEvent *dbevent = (csm::daemon::DBRespEvent *) &aEvent;
    csm::db::DBRespContent dbResp = dbevent->GetContent();
    return InspectDBResult(dbResp, errcode, errmsg);
}

} // End namespace helpers
} // End namespace daemon
} // End namespace csm

#endif
