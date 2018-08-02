/*******************************************************************************
 |    connections.h
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


#ifndef BB_CONNECTIONS_H_
#define BB_CONNECTIONS_H_

#include <string>
#include <semaphore.h>

#include "CnxSock.h"
#include "CnxSockUnix.h"
#include "CnxSockSSL.h"
#include "Msg.h"
#include "bbapi_types.h"

/*******************************************************************************
 | Constants
 *******************************************************************************/
const int DEFAULT_NUMBER_OF_THREADS = 8;
const std::string NO_CONFIG_VALUE = "none";


/*******************************************************************************
 | Enumerators
 *******************************************************************************/
#if BBPROXY
enum CONNECTION_SUSPEND_OPTION
{
    UNDEFINED           = 0xFFFF,
    NOT_SUSPENDED       = 0x0000,
    SUSPENDED           = 0x0001,
};
typedef enum CONNECTION_SUSPEND_OPTION CONNECTION_SUSPEND_OPTION;
#endif

/*******************************************************************************
 | Classes and structs
 *******************************************************************************/
class ResponseDescriptor
{
  public:
    
    volatile txp::Msg* reply;
    char attr[sizeof(txp::Attr_uint64)];
    std::string connName;
    ResponseDescriptor() : reply(NULL), msgid( txp::Id(0)) {sem_init(&semaphore,0,0); }
    txp::Id msgid;
    int sempost() {return sem_post(&semaphore);}
    int semwait() {return sem_wait(&semaphore);}
private:
    sem_t     semaphore;
};



/*******************************************************************************
 | External data and methods
 *******************************************************************************/
extern int versionCheck(const std::string& pClientVersion);
extern thread_local std::string bbconnectionName;

extern std::string          process_whoami;
extern std::string          process_instance;

extern std::map<txp::Connex*, uint32_t> contribIdMap;

extern int connectionExists(std::string whoami);
extern std::string getConnectionName(txp::Connex* pConnection);
extern txp::Connex*  getConnex(const std::string& pConnectionName);
extern int setupConnections(std::string whoami, std::string instance);
extern int makeConnection(const uint32_t contribid, const std::string& name, const std::string& alias=NO_CONFIG_VALUE);

std::string queryServer(BBServerQuery pBBServerQuery);
extern int makeInitialConnection2bbserver();
extern int sendMessage(const std::string& name, txp::Msg* msg);
extern int sendMessage(const std::string& name, txp::Msg* msg, ResponseDescriptor& reply, bool addUidGid=0);
extern int sendMessage2bbserver(const std::string& name, txp::Msg* msg, ResponseDescriptor& reply);
bool const MUSTADDUIDGID=true;
extern int sendMsgAndWaitForNonDataReply(const std::string& pConnectionName, txp::Msg* &pMsg);

typedef void(MessageHandlerFunc_t)(txp::Id id, const std::string& pConnectionName, txp::Msg* msg);
extern int registerMessageHandler(int32_t id, MessageHandlerFunc_t* func);

extern int addReply(txp::Msg* msg, txp::Msg* response_msg);
extern int expectReply(const std::string& pConnectionName, ResponseDescriptor& reply, txp::Msg* outgoing_msg);
extern int waitReply(ResponseDescriptor& reply, txp::Msg* &response_msg);
extern int waitReplyNoErase(ResponseDescriptor& reply, txp::Msg* &response_msg);
extern void waitReplyErase(ResponseDescriptor& reply);

extern int bbVersionToTree(std::string str, boost::property_tree::ptree& mytree);

extern void cleanupAllConnections();

extern std::string connectionNameFromAlias();
extern std::string readyBBserverList();
extern int useBackupConnection2bbserver();
extern int makeConnection2bbserver(const std::string& pName);
extern int makeActivebbserver(const std::string& pName);
extern std::string getBACKUP();
extern std::string getPRIMARY();
extern int closeConnectionFD(const std::string& name);
extern int takeActivebbserverOffline(const std::string& pName);
extern int makeInitialConnection2bbserver();
extern int openConnectionToBBserver();
#if BBPROXY
extern int countWaitReplyList(const std::string& pConnectionName);
extern CONNECTION_SUSPEND_OPTION getSuspendState(const std::string& pName);
void removeFromSuspendMap(txp::Connex* pConnection);
extern int updateSuspendMap(const std::string& pName, const CONNECTION_SUSPEND_OPTION pOption);
void updateSuspendMap(txp::Connex* pConnection, const CONNECTION_SUSPEND_OPTION pOption);
#endif

#endif /* BB_CONNECTIONS_H_ */

