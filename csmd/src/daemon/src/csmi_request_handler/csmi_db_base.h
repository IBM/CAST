/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_db_base.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_DB_BASE_H__
#define __CSMI_DB_BASE_H__

#include "csmi_base.h"

class EventContextDBBase : public csm::daemon::EventContext {

public:
   EventContextDBBase(void *aEventHandler, uint64_t aUid, csm::daemon::CoreEvent *aReqEvent = nullptr)
   : csm::daemon::EventContext(aEventHandler, aUid, aReqEvent)
   { _compareDataForPrivateCheckRes = false; }

   EventContextDBBase(const csm::daemon::EventContext_sptr aCtx)
   : csm::daemon::EventContext(aCtx->GetEventHandler(), aCtx->GetAuxiliaryId(), aCtx->GetReqEvent())
   { _compareDataForPrivateCheckRes = false; }
   
   virtual ~EventContextDBBase()
   { }

   void SetCompareDataForPrivateCheck(bool aInfo) { _compareDataForPrivateCheckRes = aInfo; }
   bool GetCompareDataForPrivateCheck() { return _compareDataForPrivateCheckRes; }
   
private:
   bool _compareDataForPrivateCheckRes;
};

typedef std::shared_ptr<EventContextDBBase> EventContextDBBase_sptr;

class CSMI_DB_BASE : public CSMI_BASE {

public:
  CSMI_DB_BASE(csmi_cmd_t cmd, csm::daemon::HandlerOptions& options)
  : CSMI_BASE(cmd,options)
  { }
  
private:
  
  // if error occurs when composing a SQL stmt, this call will return false and errcode
  // and errmsg, bool compareDataForPrivateCheckRes=false shall be set with error info
  // note: errcode must be defined in csmi/src/common/include/csmi_cmd_error.h
  //       compareDataForPrivateCheckRes will be ignored if API is not PRIVATE
  virtual bool CreateSqlStmt(const std::string& arguments, const uint32_t len,
                std::string &stmt, int &errcode, std::string &errmsg, bool compareDataForPrivateCheckRes=false) = 0;
    
    
  // return the error code defined in csmi/src/common/include/csmi_cmd_error.h
  // if success to pack the tuples, buf will have the byte array of the tuples and return CSMI_SUCCESS
  // if fail, buf will have the error info and return the errcode carried in the buf
  // note: compareDataForPrivateCheckRes will be ignored if API is not PRIVATE
  virtual int CreateByteArray(std::vector<csm::db::DBTuple *>&tuples, 
                                  char **buf, uint32_t &bufLen, bool compareDataForPrivateCheckRes=false) = 0;

  
protected:
  //if success to compose a SQL stmt from the reqContent, sqlStmt will have the SQL stmt
  //and nullptr will be returned. Otherwise, a Error Network Event will be returned containing
  //the error info.
  // note: aEvent has to be a NetworkEvent
  csm::daemon::NetworkEvent* CheckDBConnAndCreateSqlStmt(const csm::daemon::CoreEvent &aEvent,
                                                        std::string &sqlStmt,
                                                        bool isPrivateCheck=false, bool compareDataForPrivateCheckRes=false);
   
  // return the NetworkEvent from original req NetworkEvent in the DBRespEvent
  // note: dbRespEvent has to be DBRespEvent and its _ReqEvent has to be a NetworkEvent
  //       should call this only after calling InspectDBResult() == true
  csm::daemon::NetworkEvent *CreateNetworkEventFromDBRespEvent(
                        const csm::daemon::CoreEvent &dbRespEvent, bool compareDataForPrivateCheckRes=false);

  // RetrieveDataForPrivateCheck used for Private APIs. This function will decode the user data
  // in the payload and construct a SQL stmt to query the DB with the user data.
  // e.g. in allocation query detail case, the user data (i.e. payload) may contain the allocation
  // id, so the API needs to compose a sql query with the allocation id to get the owner of the
  // allocation from the DB first.
  
  // For non-PRIVATE APIs, it can just return false, which is the default implementation. For Private APIs,
  // if error occurs when composing a SQL stmt, this call will return false and errcode
  // and errmsg shall be set with error info
  // note: errcode must be defined in csmi/src/common/include/csmi_cmd_error.h
  virtual bool RetrieveDataForPrivateCheck(const std::string& arguments, const uint32_t len,
                std::string &stmt, int &errcode, std::string &errmsg)
  { return false; }

  // CompareDataForPrivateCheck used for Private APIs. It is to determine if the requester has own
  // the user data by checking the DB response.
  // e.g. in allocation query detail case, the dbResp has the result of the owner of the allocation id
  // in user data. The API need to compare the uid/gid in the original request Message against the dbResp.
  virtual bool CompareDataForPrivateCheck(const csm::db::DBRespContent& dbResp, const csm::network::Message &msg)
  { return false; }
    
  typedef enum
  {
    INITIAL_STATE = 0,
    SEND_DB_PRIVATE_CHECK,
    RCV_DB_PRIVATE_CHECK,
    SEND_DB,
    RCV_DB,
    DONE
  } db_access_stages;

public:

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
};

#endif

