/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasEventCreate.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMIRASEVENTCREATE_H__
#define __CSMIRASEVENTCREATE_H__

#include <memory>
#include <boost/thread.hpp>
#include <map>
#include <sstream>
#include "csmi_base.h"
#include <csmd/src/ras/include/RasMessageTypeRec.h>
#include <csmd/src/ras/include/RasEventHandlerChain.h>
#include <csmd/src/ras/include/RasEventHandlerAction.h>
#include <csmd/src/ras/include/RasEventHandlerVarSub.h>
#include <csmd/src/ras/include/RasEventPool.h>
#include <csmd/src/ras/include/RasEventThreshold.h>
#include <csmd/src/ras/include/RasEventLog.h>



class CSMIRasEventCreateContext 
{
public:
    CSMIRasEventCreateContext():
        _state(NO_STATE) {};

    enum {
        NO_STATE,
        READ_EVENT_TYPE,
        WRITE_RAS_EVENT
    };
    std::shared_ptr<RasEvent> _rasEvent;
    unsigned _state;
};


class CSMIRasEventCreate : public CSMI_BASE {

public:
  CSMIRasEventCreate(csm::daemon::HandlerOptions &options);

protected:
    void Process( const csm::daemon::CoreEvent &aEvent, 
                  std::vector<csm::daemon::CoreEvent*>& postEventList );
protected:
    void returnErrorMsg(csm::network::Address_sptr addr,
                        csm::network::Message& inMsg,
                        int errcode,
                        const std::string &errmsg,
                        std::vector<csm::daemon::CoreEvent*>& postEventList);
    RasRc decodeRasEvent(csm::network::MessageAndAddress content, 
                         RasEvent &rasEvent);

    std::string getMsgTypeSql(const std::string &msg_id, const std::string &location_name);

    RasRc getMsgTypeFromDbRec(csm::db::DBResult_sptr dbRes,
                              RasMessageTypeRec &rec,
                              std::string &node_state);  
 
    std::string trim(const std::string& str);

    std::string getRasCreateSql(RasEvent &rasEvent);

    void logSyslog(RasEvent &rasEvent);

    void onRasPoolExit(RasEvent &rasEvent,
                       const csm::daemon::CoreEvent &aEvent, 
                       std::vector<csm::daemon::CoreEvent*>& postEventList);

private:
    void init(); 
    //RasDataDbImpl _rasDataDb;       
    std::unique_ptr<RasEventHandlerChain> _rasHandlerChain;
    RasEventPool _rasEventPool;
    RasEventHandlerVarSub _handleVarSub;
    RasEventHandlerAction _handleAction;
    RasEventThreshold _rasEventThreshold;
    RasEventLog _rasEventLog;

    bool _initialized;

    csm::daemon::Configuration *_csmDaemonConfig;

    

    std::map<uint64_t, std::shared_ptr<CSMIRasEventCreateContext> > _contextMap;
    boost::mutex _ctxMutex;


public:
  virtual int GetRespData(const std::string& argument,
                            const uint32_t &argLen, 
	                    char **buf,
                            uint32_t& bufLen);
};

#endif

