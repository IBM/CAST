/*******************************************************************************
 |    connections.cc
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


#include <iostream>
#include <list>
#include <map>
#include <string>
#include <semaphore.h>
#include <stdio.h>
#include <signal.h>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <sys/stat.h>

#if BBPROXY
#include "bbproxy_flightlog.h"
#include "bbproxyConn2bbserver.h"
#include "identity.h"
#include "tracksyscall.h"
#elif BBSERVER
#include "bbserver_flightlog.h"
#include "identity.h"
#include "tracksyscall.h"
#elif BBAPI
#include "bbapi_flightlog.h"
#endif

#include "connections.h"
#include "bberror.h"
#include "bbconndata.h"
#include "bbapi_version.h"

using namespace std;
using namespace txp;
namespace po = boost::program_options;

#include "bbinternal.h"
#include "logging.h"

thread_local BBHandler bberror;
thread_local std::string bbconnectionName;
thread_local uid_t threadLocaluid=-1;
thread_local gid_t threadLocalgid=-1;


int listen_socket = 0;
int ssl_listen_socket = 0;
int unix_listen_socket = 0;

// NOTE:  If connections_io_mutex is to be acquired with connection_map_mutex,
//        connection_map_mutex MUST be acquired after connections_io_mutex and
//        released before connections_io_mutex is released.
pthread_mutex_t connections_io_mutex;
thread_local bool connections_io_mutex_ownership;
map<int, txp::Connex*> connections;

// NOTE:  If connection_map_mutex is to be acquired with connections_io_mutex,
//        connection_map_mutex MUST be acquired after connections_io_mutex and
//        released before connections_io_mutex is released.
pthread_mutex_t connection_map_mutex;
thread_local bool connections_map_mutex_ownership;
map<txp::Connex*, string> connections2name;
map<string, txp::Connex*> name2connections;
map<string, txp::Connex*> bbserverName2ReadyConnections;
#if BBPROXY
map<txp::Connex*, CONNECTION_SUSPEND_OPTION> connection2suspendState;
#endif

map<txp::Connex*, uint32_t> contribIdMap;

pthread_mutex_t replyWaitersLock = PTHREAD_MUTEX_INITIALIZER;
typedef map<ResponseDescriptor*, bool> mapResponseDescriptor;
map<string, mapResponseDescriptor > replyWaiters;

void releaseReplyWaiters(const std::string& pName){
    pthread_mutex_lock(&replyWaitersLock);
    {
#if BBPROXY
        if (replyWaiters[pName].size())
        {
            LOG(bb,info) << "releaseReplyWaiters() entry:  pName " << pName << ", replyWaiters[pName].size() " << replyWaiters[pName].size();
        }
#endif
        for(auto waiters : replyWaiters[pName])
        {
            LOG(bb,error) << "Notifying reply of connection close";
            waiters.first->reply = NULL;
            waiters.first->sempost();
        }
        replyWaiters.erase(pName);
    }
    pthread_mutex_unlock(&replyWaitersLock);
}
//for gdb, print dumpReplyWaiters()
extern int dumpReplyWaiters();
int dumpReplyWaiters(){
    pthread_mutex_lock(&replyWaitersLock);
    int i=0;
    char l_MsgIdStr[64] = {'\0'};
    for(auto& list : replyWaiters){
        printf("connName=%s \n",list.first.c_str() );
        int j=0;
        for (auto& waiters: list.second){
            j++;
            l_MsgIdStr[64] = {'\0'};
            txp::Msg::msgIdToChar(waiters.first->msgid, l_MsgIdStr, sizeof(l_MsgIdStr));
            printf("%u msgid=%u (%s) \n",j,waiters.first->msgid,l_MsgIdStr);
            i++;
        }
        if (!j) printf("\t waiters=%u \n",j);
    }
    pthread_mutex_unlock(&replyWaitersLock);
    return i;
}


map<int32_t, MessageHandlerFunc_t*> ID_handler_table;

sem_t           connection_sem;
int             connection_doorbell[2];
//txp::Connex*    newconnection_sock;

string          process_whoami;
string          process_instance;

class threadState
{
    public:
        sem_t         workAvailable;
        txp::Msg*     msg;
        std::string   connectionName;
#if BBPROXY
        uid_t uid;
        gid_t gid;
#endif
     threadState(){
      msg=NULL;
#if BBPROXY
      uid=-1;

      gid=-1;
#endif
    }
    ~threadState(){ if (msg) delete msg; }
};

pthread_mutex_t                 threadFreePool_mutex;
list<pthread_t>                 threadFreePool;
list<threadState*>              threadBacklog;
map<pthread_t, threadState*>    threadPool;

bool responseThreadRunning = false;

static void* workerThread(void* ptr);
static void* responseThread(void* ptr);

FL_SetName(FLConn, "Connection Flightlog")
FL_SetSize(FLConn, 4096)


bool bberror_flat_compare(const std::pair<std::string,std::string>& first, const std::pair<std::string,std::string>& second)
{
    return (first.first.compare(second.first) < 0);
}

int bbVersionToTree(string str, boost::property_tree::ptree& mytree)
{
    replace( str.begin(), str.end(), '^', '"');
    std::istringstream stream(str);
    boost::property_tree::read_json(stream, mytree);
    return 0;
}

void addToConnectionMap(const int pFD, txp::Connex* pConnection)
{
    // NOTE: connection_map_mutex must be held when invoking this routine
    assert(connections_map_mutex_ownership);
    LOG(bb,debug) << "connections: ADD: fd=" << pFD << " => " << static_cast<void*>(pConnection);
    connections[pFD] = pConnection;
}

void removeFromConnectionMap(const int pFD)
{
    // NOTE: connections_io_mutex and connection_map_mutex must be held when invoking this routine
    assert(connections_io_mutex_ownership);
    assert(connections_map_mutex_ownership);
    LOG(bb,debug) << "connections: REMOVE: fd=" << pFD;
    auto l_connection=connections[pFD];
    connections.erase(pFD);
    delete l_connection;
    return;
}

void addToNameConnectionMaps(txp::Connex* pConnection, const string& pName)
{
    // NOTE: connection_map_mutex must be held when invoking this routine
    assert(connections_map_mutex_ownership);
    LOG(bb,debug) << "name2connections: ADD: name=" << pName << " => " << static_cast<void*>(pConnection);

    connections2name[pConnection] = pName;
    name2connections[pName] = pConnection;
    addConnData(pName);

    return;
}

void addToNameConnectionMaps(txp::Connex* pConnection)
{
    // NOTE: connection_map_mutex must be held when invoking this routine
    assert(connections_map_mutex_ownership);
    std::string l_newconnection_name = pConnection->getConnectName();
    std::string l_newconnection_alias = pConnection->getConnectNameAlias();
    LOG(bb,info) << "name2connections: ADD: name=" << l_newconnection_name << " => " << static_cast<void*>(pConnection)<<"alias=" << l_newconnection_alias;

    connections2name[pConnection] = l_newconnection_name;
    name2connections[l_newconnection_name] = pConnection;

    if(l_newconnection_alias == DEFAULT_SERVER_ALIAS)
    {
        name2connections[DEFAULT_SERVER_ALIAS] = pConnection;
    }
    else {
        bbserverName2ReadyConnections[l_newconnection_name] = pConnection;
    }

    addConnData(l_newconnection_name);
    return;
}

void removeFromNameConnectionMaps(txp::Connex* pConnection)
{
    // NOTE: connections_io_mutex and connection_map_mutex must be held when invoking this routine
    assert(connections_io_mutex_ownership);
    assert(connections_map_mutex_ownership);
    string l_Name;

    l_Name = connections2name[pConnection];
    std::string alias = pConnection-> getConnectNameAlias( );
    LOG(bb,debug) << "name2connections: REMOVE: name=" << l_Name << " => " << static_cast<void*>(pConnection);
#if BBPROXY
    removeFromSuspendMap(pConnection);
    bbserverName2ReadyConnections.erase(l_Name);
#endif
    name2connections.erase(l_Name);
    name2connections.erase(alias);
    rmvConnData(l_Name);
    connections2name.erase(pConnection);
    return;
}

void addToContribIdMap(txp::Connex* pConnection, const char* pWhoAmI, const char* pInstance, const uint32_t pContribId)
{
    // NOTE: connection_map_mutex must be held when invoking this routine
    assert(connections_map_mutex_ownership);
    LOG(bb,debug) << "contribIdMap: ADD: whoami=" << pWhoAmI << ", instance=" << pInstance << ", connection=" << pConnection << " => contribid=" << pContribId;
    contribIdMap[pConnection] = pContribId;

    return;
}

void removeFromContribIdMap(txp::Connex* pConnection)
{
    // NOTE: connections_io_mutex and connection_map_mutex must be held when invoking this routine
    assert(connections_io_mutex_ownership);
    assert(connections_map_mutex_ownership);
    LOG(bb,debug) << "contribIdMap: REMOVE: " << static_cast<void*>(pConnection);
    contribIdMap.erase(pConnection);

    return;
}

void lockConnectionMaps(const char* pMethod)
{
    //LOG(bb,debug) << "CONN_MAPS:   LOCK -> (" << static_cast<void*>(&connection_map_mutex) << "), " << pMethod;
    pthread_mutex_lock(&connection_map_mutex);
    connections_map_mutex_ownership = true;
}

void unlockConnectionMaps(const char* pMethod)
{
    //LOG(bb,debug) << "CONN_MAPS: UNLOCK <- (" << static_cast<void*>(&connection_map_mutex) << "), " << pMethod;
    connections_map_mutex_ownership = false;
    pthread_mutex_unlock(&connection_map_mutex);
}

void lockConnectionWrite(const char* pMethod)
{
    LOG(bb,debug) << "CONN_IO:   LOCK -> (" << static_cast<void*>(&connections_io_mutex) << "), " << pMethod;
    pthread_mutex_lock(&connections_io_mutex);
    connections_io_mutex_ownership = true;
}

void unlockConnectionWrite(const char* pMethod)
{
    LOG(bb,debug) << "CONN_IO: UNLOCK <- (" << static_cast<void*>(&connections_io_mutex) << "), " << pMethod;
    connections_io_mutex_ownership = false;
    pthread_mutex_unlock(&connections_io_mutex);
}
#ifdef BBSERVER
static int newconnection_name_sequence_number=0;
#endif

/**
   \brief Connection authentication response handler
 */
int versionCheck(const std::string& pReceivedVersion){
    boost::property_tree::ptree receivedV;
    boost::property_tree::ptree myV;

    bbVersionToTree(pReceivedVersion, receivedV);
    bbVersionToTree(BBAPI_CLIENTVERSIONSTR, myV);

    if(myV.get("version.major", "abc") != receivedV.get("version.major", "xyz"))
    {
        stringstream errorText;
        errorText << "Version mismatch.  Received version.major="<<pReceivedVersion<<" My version="<<BBAPI_CLIENTVERSIONSTR;
        bberror << err("error.receivedversion", pReceivedVersion);
        bberror << err("error.myversion",BBAPI_CLIENTVERSIONSTR);
        bberror << err("error.whoami", process_whoami);
        LOG_RC_AND_RAS_AND_BAIL(-1, bb.cfgerr.versionmismatch);
    }
    if(myV.get("gitcommit", "abc") != receivedV.get("gitcommit", "xyz"))
        LOG(bb,info)<<"gitcommit levels are different,  received="<<pReceivedVersion<<" my Version="<<BBAPI_CLIENTVERSIONSTR;
    return 0;
}

void connection_authenticate(txp::Id id, txp::Connex* conn, txp::Msg*& msg)
{
    const char* receivedVersionStr = NULL;
    stringstream errorText;
    int rc = -1;
    uint32_t contribid;
    const char* receivedFromWhoami = NULL;
    const char* instance = NULL;

    try
    {
        receivedVersionStr = (const char*)msg->retrieveAttrs()->at(txp::version)->getDataPtr();
        contribid = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        receivedFromWhoami   = (const char*)msg->retrieveAttrs()->at(txp::whoami)->getDataPtr();
        instance = (const char*)msg->retrieveAttrs()->at(txp::instance)->getDataPtr();
        LOG(bb,debug)<<"receivedFromWhoami="<<receivedFromWhoami<<" instance="<<instance<< " receivedVersionStr="<<receivedVersionStr<<" process_whoami="<<process_whoami.c_str()<<" version="<<BBAPI_CLIENTVERSIONSTR;
        std::string receivedVersionString = receivedVersionStr;
        rc = versionCheck(receivedVersionString);
    }
    catch (ExceptionBailout& e)
    {
        LOG(bb,always)<<"Authentication Error.  receivedFromWhoami="<<receivedFromWhoami<<" instance="<<instance<< " receivedVersionStr="<<receivedVersionStr<<" process_whoami="<<process_whoami.c_str()<<" version="<<BBAPI_CLIENTVERSIONSTR;

        bberror << err("error.authenticate.version", "ExceptionBailout");
    }
    catch (out_of_range& e)
    {
        errorText << "out_of_range exception map::at";
        bberror << err("error.authenticate.version", "out_of_range");
        errno = ELNRNG;
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.net.authrange);
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    catch (exception& e)
    {
        bberror << err("error.authenticate.version", "exception");
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    if (!rc)
    {
	    lockConnectionWrite("connection_authenticate");
	    {
		lockConnectionMaps("connection_authenticate");
		{
		    // NOTE: Keep name of bbapi app consistent (no sequence number).  Ensure unique name of connection
		    //       for duration of socket connection between bbproxy and bbserver (use .sequencenumber).
#ifdef BBSERVER
		    std::string l_newconnection_name = string(receivedFromWhoami) + string(instance) + string(".") + to_string(++newconnection_name_sequence_number);
#else
		    std::string l_newconnection_name = string(receivedFromWhoami) + string(instance);
#endif

		    LOG(bb,debug) << "Adding connection in connection_authenticate '" << l_newconnection_name;
		    addToContribIdMap(conn, receivedFromWhoami, instance, contribid);
		    FL_Write(FLConn, FL_NewConnectAuth, "Adding new connection to poll loop",0,0,0,0);
		    connections[conn->getSockfd()] = conn;
            conn->setConnectName(l_newconnection_name);
		    addToNameConnectionMaps(conn, l_newconnection_name);

		    bberror.clear(l_newconnection_name);
		}
		unlockConnectionMaps("connection_authenticate");
	    }
	    unlockConnectionWrite("connection_authenticate");
    }
    txp::Msg* response;
    msg->buildResponseMsg(response);
    addReply(msg, response);
    txp::Attr_int32 resultcode(txp::resultCode, rc);
    response->addAttribute(&resultcode);
    if (rc) {addBBErrorToMsg(response);}
    conn->write(response);
    delete msg;
    msg = NULL;
    delete response;
    if (rc) conn->disconnect();

    return;
}

std::string getConnectionName(txp::Connex* pConnection)
{
    std::string l_Name = "";
    lockConnectionMaps("getConnectionName");
    {
        l_Name = connections2name[pConnection];
    }
    unlockConnectionMaps("getConnectionName");

    return l_Name;
}

txp::Connex* getConnex(const std::string& pConnectionName)
{
    txp::Connex* l_connex= NULL;
    lockConnectionMaps("getConnex");
    {
        auto it = name2connections.find(pConnectionName);
        if (it!= name2connections.end())
        {
          l_connex = it->second;
        }
    }
    unlockConnectionMaps("getConnex");

    return l_connex;
}

bool isLocalRemoteNotSameAddress(const std::string& pConnectionName)
{
    txp::Connex* l_connex= NULL;
    bool answer=false;
    lockConnectionMaps("getConnex");
    {
        auto it = name2connections.find(pConnectionName);
        if (it!= name2connections.end())
        {
            l_connex = it->second;
            answer=l_connex->remoteAndLocalAddressNotSame();
        }
    }
    unlockConnectionMaps("getConnex");

    return answer;
}

int takeActivebbserverOffline(const std::string& pName){
    int rc=EINVAL;
    txp::Connex* active_connex= NULL;
    lockConnectionMaps("takeActivebbserverOffline");
    auto it = name2connections.find(DEFAULT_SERVER_ALIAS);
    if (it!= name2connections.end())
    {
        active_connex = it->second;
        if (active_connex){
            bbserverName2ReadyConnections[active_connex->getConnectName()]=active_connex;
            active_connex->setConnectNameAlias(active_connex->getConnectName() );
            name2connections.erase(DEFAULT_SERVER_ALIAS);
            rc=0;
        }

    }
    unlockConnectionMaps("takeActivebbserverOffline");
    return rc;
}

int makeActivebbserver(const std::string& pName){
    int rc=0;
    txp::Connex* candidate_connex= NULL;
    txp::Connex* active_connex= NULL;
    lockConnectionMaps("makeActivebbserver");
    auto it = name2connections.find(DEFAULT_SERVER_ALIAS);
    if (it!= name2connections.end())
    {
        active_connex = it->second;
        if (!active_connex)
        {
            name2connections.erase(it);
        }
    }
    auto it2 = bbserverName2ReadyConnections.find(pName);
    if (it2!= bbserverName2ReadyConnections.end())
    {
        candidate_connex = it2->second;
        if (!candidate_connex) {
            bbserverName2ReadyConnections.erase(pName);
            LOG(bb,always) << "found a bad bbserverName2ReadyConnections entry for pName="<<pName;
        }
        else if (candidate_connex!=active_connex)
        {
            name2connections[DEFAULT_SERVER_ALIAS] = candidate_connex;
            candidate_connex->setConnectNameAlias(DEFAULT_SERVER_ALIAS);
            bbserverName2ReadyConnections.erase(pName);
            if (active_connex){
                bbserverName2ReadyConnections[active_connex->getConnectName()]=active_connex;
                active_connex->setConnectNameAlias(active_connex->getConnectName() );
            }
        }
    }
    else {
        rc=ENOTCONN;
    }
    unlockConnectionMaps("makeActivebbserver");
    return rc;
}

std::string readyBBserverList()
{
    std::string l_list;
    lockConnectionMaps("readyBBserverList");
    for(auto& iter : bbserverName2ReadyConnections)
    {
        l_list += iter.first;
        l_list += " ";
    }
    unlockConnectionMaps("readyBBserverList");
    return l_list;
}
/**
   \brief Registers a message handler to be called from a worker thread.
   \note  All handlers must be registered before starting the response handler.
   This is so that no thread locking is needed to access the ID_handler_table[].

   \param[in] id Message identifier.  txp::Msgid()->value
   \param[in] func Function pointer to call when matching msgid is received.
 */
int registerMessageHandler(int32_t id, MessageHandlerFunc_t* func)
{
    if (responseThreadRunning)
        return -1;

    ID_handler_table[id] = func;

    return 0;
}

/**
   \brief Cleanup a Connection
   \param[in] pConnection Connection
   \param[in] pFD File descriptor
 */
std::string cleanupConnection(int pFD)
{
    std::string nameRemoved;
    lockConnectionWrite("cleanupConnection");
    {
        lockConnectionMaps("cleanupConnection");
        {
            txp::Connex* pConnection = connections[pFD];
            nameRemoved = pConnection->getConnectName();
            releaseReplyWaiters(nameRemoved);
            removeFromContribIdMap(pConnection);
            removeFromNameConnectionMaps(pConnection);
            removeFromConnectionMap(pFD); //deletes connection
        }
        unlockConnectionMaps("cleanupConnection");
    }
    unlockConnectionWrite("cleanupConnection");

    LOG(bb,debug) << "Closing connection '" << nameRemoved << "'";

    return nameRemoved;
}

/**
   \brief Cleanup All Connection
 */
void cleanupAllConnections()
{
    lockConnectionWrite("cleanupAllConnections");
    {
        lockConnectionMaps("cleanupAllConnections");
        for (auto& iter : connections)
        {
            int fd=iter.first;
            txp::Connex* pConnection = connections[fd];
            std::string nameRemoved = pConnection->getConnectName();
            releaseReplyWaiters(nameRemoved);
            removeFromContribIdMap(pConnection);
            removeFromNameConnectionMaps(pConnection);
            removeFromConnectionMap(fd); //deletes connection
        }
        for (auto& iter : name2connections) name2connections.erase(iter.first);
        unlockConnectionMaps("cleanupAllConnections");
    }
    unlockConnectionWrite("cleanupAllConnections");


    return;
}

/**
   \brief Does connection exist
   \param[in] name Name of the connection
 */
int connectionExists(string name)
{
    lockConnectionMaps("connectionExists");
    int rc = (name2connections.find(name) != name2connections.end() ? 1 : 0);
    unlockConnectionMaps("connectionExists");

    return rc;
}

std::string connectionNameFromAlias()
{
    std::string connectionName;
    lockConnectionMaps("connectionNameFromAlias");
    auto it = name2connections.find(DEFAULT_SERVER_ALIAS);
    if (it != name2connections.end() )
    {
        connectionName = it->second->getConnectName();
    }
    unlockConnectionMaps("connectionNameFromAlias");

    return connectionName;
}

/**
   \brief Send a message to a named connection
   \param[in] name Name of the connection
   \param[in] msg  Transport message to send
 */
int sendMessage(const string& name, txp::Msg* msg)
{
    int rc = -1;

    lockConnectionWrite("sendMessage(string, txp::Msg*)");
    {
        auto iter = name2connections.find(name);

        if(  (iter != name2connections.end() ) && (iter->second != NULL ) )
        {
            // We log all messages in the flight log...
            int32_t msgid = (int32_t)msg->getMsgId();
            FL_Write6(FLConn, FL_SendMsg, "sendMessage id=%ld(0x%08X), number=%ld, request=%ld, len=%ld",
                      msgid, msgid, msg->getMsgNumber(), msg->getRequestMsgNumber(), msg->getMsgLengthWithDataValues(), 0);

            stringstream l_Text;
            txp::Id l_MsgId = msg->getMsgId();
            char l_MsgIdStr[64] = {'\0'};
            msg->msgIdToChar(l_MsgId, l_MsgIdStr, sizeof(l_MsgIdStr));
            l_Text << "==> Sending msg to " << name.c_str() << ": " << l_MsgIdStr << ", msg#=" << msg->getMsgNumber() << ", rqstmsg#=" << msg->getRequestMsgNumber();

            // Not all messages are logged as 'info' in the console log...
            if (txp::isMsgIdToLogAsInfo(l_MsgId))
            {
                LOG(bb,info) << l_Text.str();
            }
            else
            {
                LOG(bb,debug) << l_Text.str();
            }

            txp::Connex* cnx = iter->second;
            rc = cnx->write(msg);
            rc = rc > 0 ? 0 : rc;
            if (rc)
            {
               FL_Write(FLConn, FL_SendMsgErr1, "sendMessage error for id=%ld, number=%ld,  rc=%ld, errno=%ld",
                        msg->getMsgId(), msg->getMsgNumber(),  rc, errno);
               LOG(bb,error) << " sendMessage:Error, " << l_MsgIdStr << " Msg#=" << msg->getMsgNumber() << ", RMsg#=" << msg->getRequestMsgNumber() \
                             << ", #Attrs=" << msg->getNumberOfAttributes() << ", RC=" << rc << ", errno=" << errno << "(" << strerror(errno) << ")";
            }
        }
        else
        {
            if(  (iter != name2connections.end() ) && (iter->second == NULL ) )
            {
                LOG(bb,always) << "SendMessageWithReply--found NULL connection to '" << name << "'";
                name2connections.erase(iter);
            }
            LOG(bb,error) << "SendMessage--Unable to find connection to '" << name << "' msg="<<msg<< " Msg#=" << msg->getMsgNumber() << ", RMsg#=" << msg->getRequestMsgNumber();
        }
    }
    unlockConnectionWrite("sendMessage(string, txp::Msg*)");

    return rc;
}

int sendMessage(const string& name, txp::Msg* msg, ResponseDescriptor& reply, bool addUidGid)
{
    int rc = -1;

    lockConnectionWrite("sendMessage(string, txp::Msg*, ResponseDescriptor&)");
    {
        lockConnectionMaps("sendMessage(string, txp::Msg*, ResponseDescriptor&)");
        {
            auto iter = name2connections.find(name);

            if((iter != name2connections.end()) && (iter->second != NULL ))
            {

                txp::Connex* cnx = iter->second;
                std::string realName = cnx->getConnectName();

                if ( (addUidGid) || (name.find(DEFAULT_SERVER_ALIAS) != std::string::npos) )
                {
                    LOG(bb,debug) << "connections: sendMessage l_Owner=" << threadLocaluid << ", l_Group=" << threadLocalgid;
                    msg->addAttribute(txp::uid, (uint32_t)threadLocaluid);
                    msg->addAttribute(txp::gid, (uint32_t)threadLocalgid);
                }

                if (msg->retrieveAttrs()->find(txp::responseHandle) == msg->retrieveAttrs()->end())
                {
                    /* Warning:
                       The following uses new-with-placement using storage already allocated in ResponseDescriptor.
                       This means that no malloc/mmap was performed to acquire the storage, so do not call C++ destructors or
                       attempt to deallocate this storage.  Deallocation will occur when ResponseDescriptor goes out of scope.
                    */
                    txp::Attr_uint64* myattr = new(&reply.attr) txp::Attr_uint64(txp::responseHandle, (uint64_t)&reply);
                    msg->addAttribute(myattr);
                }
                reply.connName = realName;
                reply.msgid = msg->getMsgId();

                // We log all messages in the flight log...
                FL_Write(FLConn, FL_SendMsgWReply, "Send message id=%ld, number=%ld, request=%ld, len=%ld",
                         (int32_t)msg->getMsgId(), msg->getMsgNumber(), msg->getRequestMsgNumber(), msg->getMsgLengthWithDataValues());

                stringstream l_Text;
                txp::Id l_MsgId = msg->getMsgId();
                char l_MsgIdStr[64] = {'\0'};
                msg->msgIdToChar(l_MsgId, l_MsgIdStr, sizeof(l_MsgIdStr));
	            size_t l_MessageToServer = name.find(DEFAULT_SERVER_ALIAS);
                if (l_MessageToServer != std::string::npos)
                {
                    l_Text << "==> Sending msg to " << realName.c_str() << ": " << l_MsgIdStr << ", msg#=" << msg->getMsgNumber() << ", rqstmsg#=" << msg->getRequestMsgNumber() \
                           << ", uid=" << (uid_t)((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::uid))->getData() \
                           << ", gid=" << (gid_t)((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::gid))->getData();
                }
                else
                {
                    l_Text << "==> Sending msg to " << realName.c_str() << ": " << l_MsgIdStr << ", msg#=" << msg->getMsgNumber() << ", rqstmsg#=" << msg->getRequestMsgNumber();
                }

                // Not all messages are logged as 'info' in the console log...
                if (txp::isMsgIdToLogAsInfo(l_MsgId))
                {
                    LOG(bb,info) << l_Text.str();
                }
                else
                {
                    LOG(bb,debug) << l_Text.str();
                }

                rc = cnx->write(msg);
                rc = rc > 0 ? 0 : rc;
                if (rc)
                {
                   FL_Write(FLConn, FL_SendMsgErr2, "sendMessage error for id=%ld, number=%ld,  rc=%ld, errno=%ld",
                            msg->getMsgId(), msg->getMsgNumber(),  rc, errno);
                   LOG(bb,error) << " sendMessage:Error, " << l_MsgIdStr << " Msg#=" << msg->getMsgNumber() << ", RMsg#=" << msg->getRequestMsgNumber() \
                                 << ", #Attrs=" << msg->getNumberOfAttributes() << ", RC=" << rc << ", errno=" << errno << "(" << strerror(errno) << ")";
                }
            }
            else
            {
                if(  (iter != name2connections.end() ) && (iter->second == NULL ) )
                {
                    LOG(bb,always) << "SendMessageWithReply--found NULL connection to '" << name << "'";
                    name2connections.erase(iter);
                }
                LOG(bb,error) << "SendMessageWithReply--Unable to find connection to '" << name << "'";
            }
        }
        unlockConnectionMaps("sendMessage(string, txp::Msg*, ResponseDescriptor&)");
    }
    unlockConnectionWrite("sendMessage(string, txp::Msg*, ResponseDescriptor&)");

    return rc;
}

int sendMessage2bbserver(const string& name, txp::Msg* msg, ResponseDescriptor& reply)
{
    return sendMessage(name, msg, reply, true);
}

int sendMsgAndWaitForReturnCode(const std::string& pConnectionName, txp::Msg* &pMsg)
{
    int rc = 0;
    ResponseDescriptor reply;

    expectReply(pConnectionName, reply, pMsg);

    // Send the message to bbserver
    rc = sendMessage(pConnectionName, pMsg, reply);

    if (!rc)
    {
        // Wait for the response
        txp::Msg* l_ReplyMsg = 0;
        rc = waitReply(reply, l_ReplyMsg);
        txp::Attribute* l_Attribute = l_ReplyMsg->retrieveAttr(txp::returncode);
        if (l_Attribute)
        {
            rc = (int)(*((int32_t*)(l_Attribute->getDataPtr())));
        }

        delete l_ReplyMsg;
    }

    return rc;
}

int expectReply(const std::string& pConnectionName, ResponseDescriptor& reply, txp::Msg* outgoing_msg)
{
    reply.connName=pConnectionName;
    reply.msgid = outgoing_msg->getMsgId();

    /* Warning:
       The following uses new-with-placement using storage already allocated in ResponseDescriptor.
       This means that no malloc/mmap was performed to acquire the storage, so do not call C++ destructors or
       attempt to deallocate this storage.  Deallocation will occur when ResponseDescriptor goes out of scope.
     */
    txp::Attr_uint64* myattr = new(&reply.attr) txp::Attr_uint64(txp::responseHandle, (uint64_t)&reply);
    outgoing_msg->addAttribute(myattr);

    return 0;
}

//map<string, map<ResponseDescriptor*, bool> > replyWaiters;
int countWaitReplyList(const std::string& pConnectionName){
    int count=-1;
    pthread_mutex_lock(&replyWaitersLock);
    auto itReplyWaiter = replyWaiters.find(pConnectionName);
    if (itReplyWaiter != replyWaiters.end() ){
        count = itReplyWaiter->second.size(); //size of responseDescriptor map
        for(auto waiters : replyWaiters[pConnectionName]){
            uint64_t l_value = waiters.first->msgid;
            LOG(bb,info) << "pConnectionName="<<name<<" has msgid="<<l_value<<" out of count="<<count;
            break;
        }
    }
    else bberror<<err("out.notfound",pConnectionName);
    pthread_mutex_unlock(&replyWaitersLock);
    return count;
}

int waitReply(ResponseDescriptor& reply, txp::Msg*& response_msg)
{
    char l_MsgId[64] = {'\0'};
    txp::Msg::msgIdToChar(reply.msgid, l_MsgId, sizeof(l_MsgId));

#if BBPROXY
    pthread_mutex_lock(&replyWaitersLock);
    {
        size_t l_Before = replyWaiters[reply.connName].size();
        replyWaiters[reply.connName][&reply] = true;
        LOG(bb,debug) << "waitReply:    Add [" << reply.connName << "][" << &reply << "] for " << l_MsgId << ", wfr_count " << l_Before << "->" << replyWaiters[reply.connName].size();
    }
    pthread_mutex_unlock(&replyWaitersLock);
#endif

    reply.semwait();

#if BBPROXY
    pthread_mutex_lock(&replyWaitersLock);
    {
        size_t l_Before = replyWaiters[reply.connName].size();
        replyWaiters[reply.connName].erase(&reply);
        LOG(bb,debug) << "waitReply:  Erase [" << reply.connName << "][" << &reply << "] for " << l_MsgId << ", wfr_count " << l_Before << "->" << replyWaiters[reply.connName].size();
    }
    pthread_mutex_unlock(&replyWaitersLock);
#endif

    response_msg = (txp::Msg*)reply.reply;

    if(reply.reply == NULL)
    {
        bberror << err("error.text", "Connection closed waiting for the reply");
        return -1;
    }

    return 0;
}

int waitReplyNoErase(ResponseDescriptor& reply, txp::Msg*& response_msg)
{
    char l_MsgId[64] = {'\0'};
    txp::Msg::msgIdToChar(reply.msgid, l_MsgId, sizeof(l_MsgId));

    pthread_mutex_lock(&replyWaitersLock);
    {
        size_t l_Before = replyWaiters[reply.connName].size();
        replyWaiters[reply.connName][&reply] = true;
        LOG(bb,debug) << "waitReplyNoErase: [" << reply.connName << "][" << &reply << "] for " << l_MsgId << ", wfr_count " << l_Before << "->" << replyWaiters[reply.connName].size();
    }
    pthread_mutex_unlock(&replyWaitersLock);

    reply.semwait();

    pthread_mutex_lock(&replyWaitersLock);
    {
        response_msg = (txp::Msg*)reply.reply;
        reply.reply =NULL; //lose the message reference to response_msg
    }
    pthread_mutex_unlock(&replyWaitersLock);

    if(response_msg == NULL)
    {
        bberror << err("error.text", "Connection closed waiting for the reply");
        return -1;
    }

    return 0;
}

void waitReplyErase(ResponseDescriptor& reply)
{
    char l_MsgId[64] = {'\0'};
    txp::Msg::msgIdToChar(reply.msgid, l_MsgId, sizeof(l_MsgId));

    pthread_mutex_lock(&replyWaitersLock);
    {
        size_t l_Before = replyWaiters[reply.connName].size();
        replyWaiters[reply.connName].erase(&reply);
        LOG(bb,debug) << "waitReply:  Erase [" << reply.connName << "][" << &reply << "] for " << l_MsgId << ", wfr_count " << l_Before << "->" << replyWaiters[reply.connName].size();
    }
    pthread_mutex_unlock(&replyWaitersLock);
}

int addReply(txp::Msg* msg, txp::Msg* response_msg)
{
    response_msg->addAttribute(msg->retrieveAttrs()->at(txp::responseHandle));

    return 0;
}

int closeConnectionFD(const string& name)
{
    int noconfig=0;
    lockConnectionWrite("closeConnectionFD");
    {
        lockConnectionMaps("closeConnectionFD");
        {
            auto it=name2connections.find(name);
            if (it != name2connections.end())
            {
                txp::Connex* cnx = it->second;
                if (cnx)
                {
                    releaseReplyWaiters(name);
                    removeFromContribIdMap(cnx);
                    removeFromNameConnectionMaps(cnx);
                    int fd = cnx->getSockfd();
                    removeFromConnectionMap(fd); //deletes connection
                }
                else
                {
                    releaseReplyWaiters(name);
                    name2connections.erase(it);
                }

            }
            //check if in config
            else if (NO_CONFIG_VALUE != config.get(name + ".ssladdress", NO_CONFIG_VALUE) ){ }
            else if (NO_CONFIG_VALUE != config.get(name + ".address", NO_CONFIG_VALUE) ){ }
            else{
                noconfig=1;
            }


        }
        unlockConnectionMaps("closeConnectionFD");
    }
    unlockConnectionWrite("closeConnectionFD");
    LOG(bb,debug) << "Closing connection '" << name << "'";
    if (!noconfig){
      int tmp = 0;
      write(connection_doorbell[1], &tmp, sizeof(tmp));
      sem_wait(&connection_sem);
      return 0;
    }
    return -1;

}

void addToConnectionMapsWithDoorbell(txp::Connex* pConnection_sock){
    lockConnectionMaps("addToConnectionMaps");
    {
        connections[pConnection_sock->getSockfd()] = pConnection_sock;
        addToNameConnectionMaps(pConnection_sock);
#if BBPROXY
        updateSuspendMap(pConnection_sock, NOT_SUSPENDED);
#endif
    }
    unlockConnectionMaps("addToConnectionMaps");
    int tmp = 0;
    write(connection_doorbell[1], &tmp, sizeof(tmp));//note:  what if two writers to doorbell????
    sem_wait(&connection_sem);
}

#if BBPROXY

static string bkupServerCfg; //bbproxy backup connection
static string serverCfg; //bbproxy primary connection

std::string getBACKUP(){return bkupServerCfg;}
std::string getPRIMARY(){return serverCfg;}

int doAuthenticate(const string& name);
int bbproxy_SayHello(const string& pConnectionName);
int xchgWithBBserver(const string& name)
{
    int rc=doAuthenticate(name);
    if(rc)
    {
        LOG(bb,error) << "Authenication failed for bbserver="<<name;
        return -2;
    }
    rc = bbproxy_SayHello(name);
    if(rc)
    {
        LOG(bb,error) << "Hello exchange with bbserver failed";
        return -2;
    }
    return rc;
}

int makeInitialConnection2bbserver()
{
    serverCfg = config.get(ProcessId + ".servercfg", NO_CONFIG_VALUE);
    txp::Connex* newconnection_sock = createConnection2bbserver(serverCfg);
    bkupServerCfg = config.get(ProcessId + ".backupcfg", NO_CONFIG_VALUE);
    LOG(bb,always) << "makeInitialConnection2bbserver: serverCfg="<<getPRIMARY()<<" bkupServerCfg="<<bkupServerCfg;
    if (!newconnection_sock) {
        return ENOTCONN;
    }
    newconnection_sock-> setConnectNameAlias( DEFAULT_SERVER_ALIAS);
    addToConnectionMapsWithDoorbell(newconnection_sock);
    std::string l_newconnection_name = newconnection_sock->getConnectName();
    int rc=xchgWithBBserver(l_newconnection_name);
    return rc;
}

int useBackupConnection2bbserver()
{
    if (bkupServerCfg==NO_CONFIG_VALUE) return ENOENT;
    txp::Connex* newconnection_sock = createConnection2bbserver(bkupServerCfg);
    if (!newconnection_sock) {
        return ENOTCONN;
    }
    newconnection_sock-> setConnectNameAlias( DEFAULT_SERVER_ALIAS);
    addToConnectionMapsWithDoorbell(newconnection_sock);
    std::string l_newconnection_name = newconnection_sock->getConnectName();
    int rc=xchgWithBBserver(l_newconnection_name);
    return rc;
}

int makeConnection2bbserver(const std::string& pName)
{
    txp::Connex* cnx = getConnex(pName);
    if (cnx){
        //already exists
        return 0;
    }
    txp::Connex* newconnection_sock = createConnection2bbserver(pName);
    if (!newconnection_sock) {
        return ENOTCONN;
    }
    addToConnectionMapsWithDoorbell(newconnection_sock);
    std::string l_newconnection_name = newconnection_sock->getConnectName();
    int rc=xchgWithBBserver(l_newconnection_name);
    return rc;
}

int start_bbserverRecoveryThread(const string& name);
int openConnectionToBBserver()
{
    int rc = makeInitialConnection2bbserver();
    if(rc)
    {
        LOG(bb,error) << "Opening connection to bbserver failed";
        rc = useBackupConnection2bbserver();
        if (rc) {
            LOG(bb,error) << "Opening connection to backup bbserver failed";
        }
    }
    if (rc) {
       //start_bbserverRecoveryThread(serverCfg);
    }
    return rc;
}

void* bbserverRecoveryThread(void* ptr)
{
    std::string* connectionName2Recover = (std::string *)ptr;
    txp::Connex* cnxDEFAULT = getConnex(DEFAULT_SERVER_ALIAS);
    //pthread_t myid        = pthread_self();
    LOG(bb,info) << "StartingbbserverRecoveryThread thread connection name="<<*connectionName2Recover;
    try
    {
        int rc = makeConnection2bbserver(*connectionName2Recover); //attempt to recover connection lost
        if  (cnxDEFAULT){
            delete connectionName2Recover;
            return 0;
        }
        if (!rc)    rc = makeActivebbserver(*connectionName2Recover);
        delete connectionName2Recover;
        while(rc){
            if (serverCfg!=NO_CONFIG_VALUE){
                rc = makeConnection2bbserver(serverCfg);
                if (!rc) rc = makeActivebbserver(serverCfg);
                if (!rc){
                    LOG(bb,info) << "Using active primary bbserver="<<serverCfg;
                    break;
                }
            }
            if (bkupServerCfg!=NO_CONFIG_VALUE){
                rc = makeConnection2bbserver(bkupServerCfg);
                if (!rc) rc = makeActivebbserver(bkupServerCfg);
                if (!rc){
                    LOG(bb,info) << "Using active backup bbserver="<<bkupServerCfg;
                    break;
                }
            }
            sleep(5);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        LOG(bb,error) << "Exception thrown in bbserverRecoveryThread thread: " << e.what();
    }

    return NULL;
}

int start_bbserverRecoveryThread(const string& name)
{
    int rc=0;
    if ( (serverCfg==name) || (bkupServerCfg==name) )
    {
        pthread_t      tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        std::string * connectionName2Recover = new std::string(name);
        pthread_create(&tid, &attr, bbserverRecoveryThread, connectionName2Recover);
        rc=tid;
    }
    return rc;
}

CONNECTION_SUSPEND_OPTION getSuspendState(const std::string& pName)
{
    CONNECTION_SUSPEND_OPTION rc = UNDEFINED;

    txp::Connex* l_Connection = getConnex(pName);
    if (l_Connection)
    {
        map<txp::Connex*, CONNECTION_SUSPEND_OPTION>::iterator it;
        lockConnectionMaps("isSuspended");
        {
            it = connection2suspendState.find(l_Connection);
            if (it != connection2suspendState.end())
            {
                rc = connection2suspendState[l_Connection];
            }
        }
        unlockConnectionMaps("isSuspended");
    }

    return rc;
}

void removeFromSuspendMap(txp::Connex* pConnection)
{
    assert(connections_map_mutex_ownership);
    connection2suspendState.erase(pConnection);

    return;
}

int updateSuspendMap(const std::string& pName, const CONNECTION_SUSPEND_OPTION pOption)
{
    int rc = 0;

    txp::Connex* l_Connection = getConnex(pName);
    if (l_Connection)
    {
        map<txp::Connex*, CONNECTION_SUSPEND_OPTION>::iterator it;
        lockConnectionMaps("updateSuspendMap");
        {
            it = connection2suspendState.find(l_Connection);
            if (it != connection2suspendState.end())
            {
                updateSuspendMap(l_Connection, pOption);
            }
            else
            {
                rc = -1;
                LOG(bb,error) << "Connection not found when attemping to update the suspend state for " << pName << " to " << pOption;
            }
        }
        unlockConnectionMaps("updateSuspendMap");
    }
    else
    {
        rc = -1;
        LOG(bb,error) << "Failure when attemping to update the suspend state for " << pName << " to " << pOption;
    }

    return rc;
}

void updateSuspendMap(txp::Connex* pConnection, const CONNECTION_SUSPEND_OPTION pOption)
{
    assert(connections_map_mutex_ownership);
    connection2suspendState[pConnection] = pOption;

    return;
}
#endif


#ifdef BBAPI
/**
   \brief Establish a connection for api to proxy
   \param[in] name Name of the remote node
 */
int makeConnection(const uint32_t contribid, const string& name, const string& alias)
{
    int connect_rc=0;
    LOG(bb,debug) << "makeConnection(): contribid=" << contribid << ", name=" << name << ", alias=" << alias << ", ProcessId=" << ProcessId;
    lockConnectionWrite("makeConnection");
    {
        lockConnectionMaps("makeConnection");
        {
            //check if already connected
            if(name2connections.find(name) != name2connections.end())
            {
                unlockConnectionMaps("makeConnection - failure already connected");
                unlockConnectionWrite("makeConnection - failure already connected");
                return -1;
            }

            // If connecting to the proxy, use a unix socket...
            if (name.find("bb.proxy") != std::string::npos)
            {
                string proxypath = config.get("bb.unixpath", DEFAULT_UNIXPATH);
                if(proxypath != NO_CONFIG_VALUE )
                {
                    txp::CnxSockUnix * usock;
                    usock = new txp::CnxSockUnix();
                    usock->setAddr(proxypath);

                    if (usock->openCnxSock() < 0)
                    {
                        unlockConnectionMaps("makeConnection - failure openCnxSock()");
                        unlockConnectionWrite("makeConnection - failure openCnxSock()");
                        LOG(bb,error) << "Connecting to proxy failed: openCnxSock(), errno = " << errno << ", (" << strerror(errno) << ")";
                        return -errno;
                    }
                    if (usock->connect2Remote())
                    {
                        unlockConnectionMaps("makeConnection - failure connect2Remote()");
                        unlockConnectionWrite("makeConnection - failure connect2Remote()");
                        LOG(bb,error) << "Connecting to proxy failed: connect2Remote(), errno = " << errno << ", (" << strerror(errno) << ")";
                        return -errno;
                    }

                    unlockConnectionMaps("makeConnection - bbproxy");

                    txp::Connex* newconnection_sock = usock;
                    usock->setConnectName(name);
                    usock->setConnectNameAlias(alias);

                    addToConnectionMapsWithDoorbell(newconnection_sock);

                    txp::Msg* msg = 0;
                    txp::Msg::buildMsg(txp::CORAL_AUTHENTICATE, msg);
                    ResponseDescriptor resp;

                    LOG(bb,info) << "==> Sending msg to " << name.c_str() << ": CORAL_AUTHENTICATE(UNIX), contribid=" << contribid \
                                 << ", process_whoami=" << process_whoami.c_str() << ", process_instance=" << process_instance.c_str() \
                                 << ", msg#=" << msg->getMsgNumber() << ", rqstmsg#=" << msg->getRequestMsgNumber();

                    expectReply(name, resp, msg);
                    txp::AttrPtr_char_array whoami(txp::whoami, process_whoami.c_str(), process_whoami.length()+1);
                    txp::AttrPtr_char_array instance(txp::instance, process_instance.c_str(), process_instance.length()+1);
                    msg->addAttribute(txp::contribid, contribid);
                    msg->addAttribute(&whoami);
                    msg->addAttribute(&instance);
                    msg->addAttribute(txp::version, BBAPI_CLIENTVERSIONSTR, strlen(BBAPI_CLIENTVERSIONSTR)+1);
                    usock->write(msg);
                    delete msg;

                    waitReply(resp, msg);
                    connect_rc = ((txp::Attr_int32*)msg->retrieveAttrs()->at(txp::resultCode))->getData();
                    if (connect_rc) LOG(bb,error) << "Connected with result code " << connect_rc;

                    delete msg;
                }
                else
                {
                    unlockConnectionMaps("makeConnection - failure no unix path");
                    unlockConnectionWrite("makeConnection - failure no unix path");
                    LOG(bb,error) << "Connecting to proxy failed: Path to unix socket not found";
                    return -1;
                }
            } else {
                connect_rc=-ENODATA;//should never land here
                unlockConnectionMaps("makeConnection - ENODATA");
                unlockConnectionWrite("makeConnection - ENODATA");
                LOG(bb,error) << "Connecting to proxy--ENODATA";
                return -1;
            }
        }
    }
    unlockConnectionWrite("makeConnection - CLEAR");

    return connect_rc;
}
#endif

int setupConnections(string whoami, string instance)
{
    int rc;
    in_addr_t      iplocal;
    txp::CnxSock*  sock;
    txp::CnxSockUnix* unixSock;
    txp::CnxSockSSL* sslSock;

    process_whoami   = whoami;
    process_instance = instance;

    LOG(bb,always) << "setupConnections(): whoami=" << whoami << ", ProcessId=" << ProcessId;

    sem_init(&connection_sem, 0, 0);
    pthread_mutex_init(&connections_io_mutex, NULL);
    pthread_mutex_init(&connection_map_mutex, NULL);
    pthread_mutex_init(&threadFreePool_mutex, NULL);
    pipe2(connection_doorbell, O_CLOEXEC);

    string ipaddr;
    string url = config.get(whoami + ".address", NO_CONFIG_VALUE);
    string unixPort = config.get("bb.unixpath", DEFAULT_UNIXPATH);
    string sslurl = config.get(whoami + ".ssladdress", NO_CONFIG_VALUE);
    if ( (url == NO_CONFIG_VALUE) && (sslurl == NO_CONFIG_VALUE) && (unixPort == NO_CONFIG_VALUE) ){
        LOG(bb,always) << "whoami=" << whoami << " url=" << url << " unixPort=" << unixPort << " sslurl=" << sslurl;
    }

    lockConnectionMaps("setupConnections");
    {
        if (url != NO_CONFIG_VALUE)
        {
            uint16_t port;
            sock = new txp::CnxSock(AF_INET,SOCK_STREAM);
            getIPPort(url, ipaddr, port);
            if (inet_pton(AF_INET, ipaddr.c_str(), &iplocal) == 0)
            {
                unlockConnectionMaps("setupConnections - failure, unable to build local address");
                LOG(bb,error) << "Unable to build local address";
                delete sock;
                return -1;
            }

            sock->setLocalAddr(iplocal, port);
            sock->openCnxSock();
            rc = sock->bindCnxSock();
            if (rc)
            {
                LOG(bb,always) << "bind failed for " << sock->getSockfd() << ", ip=" << ipaddr << ", port=" << port;
                unlockConnectionMaps("setupConnections - failure, sock->bindCnxSock()");
                delete sock;
                return -1;
            }
            rc = sock->listen4remote();
            if (rc)
            {
                unlockConnectionMaps("setupConnections - failure, sock->listen4remote()");
                delete sock;
                return -1;
            }

            listen_socket = sock->getSockfd();
            bool l_turnOnKeepAlive = config.get("bb.usekeepalive", true);
            sock->setKeepAliveBool(l_turnOnKeepAlive);
            if (l_turnOnKeepAlive){
                int l_keepAliveIdle   = config.get("bb.keepaliveidle", 60);
                int l_keepAliveCount  = config.get("bb.keepalivecount", 12);
                int l__keepAliveIntvl = config.get("bb.keepaliveinterval", 5);
                sock->setKeepAliveParms(l__keepAliveIntvl, l_keepAliveIdle, l_keepAliveCount);
            }

            connections[sock->getSockfd()] = sock;
            addToNameConnectionMaps(sock, "listener");

            LOG(bb,info) << "Listening for connections on fd " << sock->getSockfd() << ", ip=" << ipaddr << ", port=" << port;
        }

        if(sslurl != NO_CONFIG_VALUE)
        {
            uint16_t port;
            string cert = config.get(whoami + ".sslcertif", NO_CONFIG_VALUE);
            string key = config.get(whoami + ".sslpriv", NO_CONFIG_VALUE);

            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_all_algorithms();

            sslSock = new txp::CnxSockSSL(AF_INET,SOCK_STREAM);
            getIPPort(sslurl, ipaddr, port);
            if (inet_pton(AF_INET, ipaddr.c_str(), &iplocal) == 0)
            {
                unlockConnectionMaps("setupConnections - failure, SSL unable to build local address");
                LOG(bb,error) << "SSL - Unable to build local address";
                delete sslSock;
                return -1;
            }

            sslSock->setLocalAddr(iplocal, port);
            try{
                sslSock->loadCertificates(cert,key);
            }
            catch (std::runtime_error e){
                LOG(bb,always) << "loadCertificates failed for (SSL) what=" << e.what();
                unlockConnectionMaps("setupConnections - failure, sslSock->bindCnxSock()");
                delete sslSock;
                rc=-1;
                bberror << errloc(rc);
                bberror << err("error.loadCertificates",e.what() );
                bberror << RAS(bb.cfgerr.cantloadCert);
                return rc;
            }
            sslSock->openCnxSock();
            rc = sslSock->bindCnxSock();
            if(rc)
            {
                LOG(bb,always) << "bind failed for (SSL) " << sslSock->getSockfd() << ", ip=" << ipaddr << ", port=" << port;
                unlockConnectionMaps("setupConnections - failure, sslSock->bindCnxSock()");
                delete sslSock;
                return -1;
            }
            rc = sslSock->listen4remote();
            if(rc)
            {
                unlockConnectionMaps("setupConnections - failure, sslSock->listen4remote()");
                delete sslSock;
                return -1;
            }

            ssl_listen_socket = sslSock->getSockfd();
            bool l_turnOnKeepAlive = config.get("bb.usekeepalive", true);
            sslSock->setKeepAliveBool(l_turnOnKeepAlive);
            if (l_turnOnKeepAlive){
                int l_keepAliveIdle   = config.get("bb.keepaliveidle", 60);
                int l_keepAliveCount  = config.get("bb.keepalivecount", 12);
                int l__keepAliveIntvl = config.get("bb.keepaliveinterval", 5);
                sslSock->setKeepAliveParms(l__keepAliveIntvl, l_keepAliveIdle, l_keepAliveCount);
            }


            connections[sslSock->getSockfd()] = sslSock;
            addToNameConnectionMaps(sslSock, "ssllistener");

            LOG(bb,info) << "Listening for connections on fd (SSL) " << sslSock->getSockfd() << ", ip=" << ipaddr << ", port=" << port;
        }

        if (process_whoami.find("bb.proxy") != std::string::npos)
        {
            if (unixPort != NO_CONFIG_VALUE)
            {
                unixSock = new txp::CnxSockUnix(PF_UNIX, SOCK_STREAM,0);

                unixSock->setAddr(unixPort);
                unixSock->openCnxSock();
                rc = unixSock->bindCnxSock();
                if(rc)
                {
                    unlockConnectionMaps("setupConnections - failure, could not bind unix socket");
                    LOG(bb,error) <<  "Could not bind unix socket " << strerror(errno);
                    delete unixSock;
                    return -1;
                }
                rc = unixSock->listen4remote();
                if(rc)
                {
                    unlockConnectionMaps("setupConnections - failure, unixSock->listen4remote()");
                    LOG(bb,error) <<  "could not listen unix socket " << strerror(errno);
                    delete unixSock;
                    return -1;
                }

                //changing permission of the file to avoid unreadable file.
                rc= chmod(unixPort.c_str(), 0777 );
                if (rc)
                {
                    unlockConnectionMaps("setupConnections - failure, could not change permission of unix socket file");
                    LOG(bb,error) <<  "Could not change permission of unix socket file " << strerror(errno);
                    delete unixSock;
                    return -1;
                }

                unix_listen_socket = unixSock->getSockfd();

                connections[unixSock->getSockfd()] = unixSock;
                addToNameConnectionMaps(unixSock, "ulistener");

                LOG(bb,info) << "Listening for connections on fd " << unixSock->getSockfd() << ", path=" << unixPort;
            }
            else
            {
                unlockConnectionMaps("setupConnections - failure no unix path for bb.proxy process");
                LOG(bb,error) << "setupConnections(): No unix path for bb.proxy process";
                return -1;
            }
        }
    }
    unlockConnectionMaps("setupConnections");

    int x;
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int numthreads = 0;
    if (process_whoami.find("bb.api") == std::string::npos)
    {
        //  Not bb.api...
        numthreads = config.get(process_whoami + ".numRequestThreads", DEFAULT_NUMBER_OF_THREADS);
        LOG(bb,info) << "Using " << numthreads << " worker threads";
    }

    for(x=0; x<numthreads; x++)
    {
        pthread_create(&tid, &attr, workerThread, NULL);
    }

    pthread_create(&tid, &attr, responseThread, NULL);
    pthread_attr_destroy(&attr);

    return 0;
}

void msgCleanUp(threadState* &pThreadState, threadState* &pwuThreadState, bool pWasBacklog)
{
    if (pThreadState->msg)
    {
        auto msgId = pThreadState->msg->getMsgId();
        FL_Write6(FLConn, FL_MsgCleanUp, "connections.cc msgCleanup Msg id=%ld(0x%08x), number=%ld, request=%ld, len=%ld",msgId, msgId,pThreadState->msg->getMsgNumber(), pThreadState->msg->getRequestMsgNumber(),pThreadState->msg->getSerializedLen(), 0);
        delete pThreadState->msg;
        pThreadState->msg = NULL;
    }

    if (pWasBacklog && pwuThreadState)
    {
        delete pwuThreadState;
        pwuThreadState = NULL;
    }

    return;
}

/**
   \brief Connections worker thread
   \note There may be many worker threads running in parallel.
   If a thread is available to take more work, it will add itself to the threadFreePool.
   The worker thread also monitors the backlog queue and pulls work from there.
   The backlog queue is only used if there are no available worker threads to handle an incomming message.

   \param[in] ptr Not used, required by pthread_create calling convention
 */
void* workerThread(void* ptr)
{
    pthread_t myid        = pthread_self();
    threadState* mystate  = new threadState;
    threadState* wu       = NULL;
    bool was_backlog      = false;
#if (BBSERVER || BBPROXY)
    threadLocalTrackSyscallPtr = getSysCallTracker();
#endif
    try
    {
        sem_init(&mystate->workAvailable, 0, 0);


        pthread_mutex_lock(&threadFreePool_mutex);
        {
            threadPool[myid] = mystate;
        }
        pthread_mutex_unlock(&threadFreePool_mutex);

        while(1)
        {
#if (BBSERVER || BBPROXY)
        	becomeUser(0,0);
#endif
            pthread_mutex_lock(&threadFreePool_mutex);
            {
                if(threadBacklog.size() > 0)
                {
                    wu = threadBacklog.front();
                    threadBacklog.pop_front();
                    was_backlog = true;
                }
                else
                {
                    wu = mystate;
                    threadFreePool.push_back(myid);
                    was_backlog = false;
                }
            }
            pthread_mutex_unlock(&threadFreePool_mutex);

            if(was_backlog == false)
            {
                sem_wait(&mystate->workAvailable);
            }

            // We log all messages in the flight log...
            int32_t msgid = (int32_t)wu->msg->getMsgId();
            FL_Write6(FLConn, FL_ProcessMsg, "Processing message for msgid %ld(0x%08X), request=0x%lx, response=0x%lx, length=%ld  backlog=%ld",
                      msgid, msgid, wu->msg->getMsgNumber(), wu->msg->getRequestMsgNumber(), wu->msg->getSerializedLen(), was_backlog);

            stringstream l_Text;
            txp::Id l_MsgId = wu->msg->getMsgId();
            char l_MsgIdStr[64] = {'\0'};
            wu->msg->msgIdToChar(l_MsgId, l_MsgIdStr, sizeof(l_MsgIdStr));
#if BBSERVER
        	l_Text << "<== Processing msg " << l_MsgIdStr << ", msg#=" << wu->msg->getMsgNumber() << ", rqstmsg#=" << wu->msg->getRequestMsgNumber() \
                   << ", uid=" << (uid_t)((txp::Attr_uint32*)wu->msg->retrieveAttrs()->at(txp::uid))->getData() \
                   << ", gid=" << (gid_t)((txp::Attr_uint32*)wu->msg->retrieveAttrs()->at(txp::gid))->getData();
#else
            l_Text << "<== Processing msg " << l_MsgIdStr << ", msg#=" << wu->msg->getMsgNumber() << ", rqstmsg#=" << wu->msg->getRequestMsgNumber();
#endif

            // Not all messages are logged as 'info' in the console log...
            if (txp::isMsgIdToLogAsInfo(l_MsgId))
            {
                LOG(bb,info) << l_Text.str();
            }
            else
            {
                LOG(bb,debug) << l_Text.str();
            }

            try
            {
                bbconnectionName = wu->connectionName;
#if BBPROXY
                threadLocaluid = wu->uid;
                threadLocalgid = wu->gid;
#endif
                // NOTE:  bberror is cleared at the beginning of each handler table entry point.
                (*ID_handler_table.at(msgid))(wu->msg->getMsgId(), wu->connectionName, wu->msg);
                bbconnectionName = "";
#if BBPROXY
                threadLocaluid = -1;
                threadLocalgid = -1;
#endif
            }
            catch(exception& e)
            {
                LOG(bb,error) << "Error calling message handler.  Reason=" << e.what();
                LOG_ERROR_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e);
                throw;
            }

            msgCleanUp(mystate, wu, was_backlog);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        LOG(bb,error) << "Exception thrown in worker thread: " << e.what();
    }

    msgCleanUp(mystate, wu, was_backlog);

    // Any worker that gets here brings down the entire process...
    kill(getpid(), SIGTERM);

    return NULL;
}

/**
   \brief Handler for transport response messages, connection requests and shutdowns

   \param[in] ptr Not used, required by pthread_create calling convention
 */
void* responseThread(void* ptr)
{
    int rc;
    while(1)
    {
        unsigned int idx        = 0;
        unsigned int conncount  = connections.size() + 1;
        struct pollfd* pollinfo = (struct pollfd*)malloc(conncount * sizeof(struct pollfd));

        pollinfo[idx].fd        = connection_doorbell[0];
        pollinfo[idx].events    = POLLIN | POLLERR | POLLHUP | POLLNVAL;
        pollinfo[idx++].revents = 0;

        for (map<int,txp::Connex*>::iterator it=connections.begin(); it != connections.end(); ++it)
        {
            pollinfo[idx].fd        = (it->second)->getSockfd();
            pollinfo[idx].events    = POLLIN | POLLERR | POLLHUP | POLLNVAL;
            pollinfo[idx++].revents = 0;
        }

        rc = poll(pollinfo, conncount, -1);
        if(rc)
        {
            for(idx=0; idx<conncount; idx++)
            {
                if(pollinfo[idx].revents & POLLIN)
                {
                    FL_Write(FLConn, FL_POLLIN, "Data available on %ld (fd=%ld)  revents=%lx",
                             idx, pollinfo[idx].fd, pollinfo[idx].revents,0);

                    if( __glibc_unlikely(pollinfo[idx].fd == connection_doorbell[0]) )
                    {
                        FL_Write(FLConn, FL_NewConnect, "Adding/removing connection to poll loop",0,0,0,0);
                        int tmp;
                        int bytesRead = read(connection_doorbell[0], &tmp, sizeof(tmp));
                        if (bytesRead<0) {
                            LOG(txp,debug) << __PRETTY_FUNCTION__<< "doorbell read had errno="<<errno;
                        }
                        sem_post(&connection_sem);
                        break; //rescan file descriptors
                    }
                    else if ( __glibc_unlikely(pollinfo[idx].fd == listen_socket) )
                    {
                        FL_Write(FLConn, FL_AcceptStart, "Remote connection is being accepted",0,0,0,0);
                        txp::Connex* newsock;

                        lockConnectionMaps("responseThread - accept remote connection");
                        {
                            (connections[pollinfo[idx].fd])->accept(newsock);
                            newsock->keepAlive();
                            connections[newsock->getSockfd()] = newsock;
                        }
                        unlockConnectionMaps("responseThread - accept remote connection");

                        bberror.clear();

                        FL_Write(FLConn, FL_AcceptDone, "Remote connection was connected.  fd=%ld",newsock->getSockfd(),0,0,0);
                    }
                    else if ( __glibc_unlikely(pollinfo[idx].fd == ssl_listen_socket) )
                    {
                        FL_Write(FLConn, FL_AcceptStartSSL, "Remote SSL connection is being accepted",0,0,0,0);
                        txp::Connex* newsock;

                        lockConnectionMaps("responseThread - accept remote SSL connection");
                        {
                            (connections[pollinfo[idx].fd])->accept(newsock);
                            newsock->keepAlive();
                            connections[newsock->getSockfd()] = newsock;
                        }
                        unlockConnectionMaps("responseThread - accept remote SSL connection");

                        bberror.clear();

                        FL_Write(FLConn, FL_AcceptDoneSSL, "Remote SSL connection was connected.  fd=%ld",newsock->getSockfd(),0,0,0);
                    }
                    else if( __glibc_unlikely(pollinfo[idx].fd == unix_listen_socket) )
                    {
                        FL_Write(FLConn, FL_AcceptStartUNIX, "Remote unix connection is being accepted",0,0,0,0);
                        txp::Connex* newsock;

                        lockConnectionMaps("responseThread - accept remote unix connection");
                        {
                            int res =(connections[pollinfo[idx].fd])->accept(newsock);
                            if (res < 0)
                            {
                                LOG(bb,error) << "accept failed on Unix socket";
                                continue;
                            }
                            // setting credentials for this connection
                            newsock->setCred();
                            connections[newsock->getSockfd()] = newsock;
                        }
                        unlockConnectionMaps("responseThread - accept remote unix connection");

                        bberror.clear();

                        FL_Write(FLConn, FL_AcceptDoneUNIX, "Remote connection was connected.  fd=%ld",newsock->getSockfd(),0,0,0);
                    }
                    else
                    {
                        FL_Write(FLConn, FL_IncomingMsg, "Reading incoming transport message on fd=%ld",pollinfo[idx].fd,0,0,0);

                        bberror.clear(getConnectionName(connections[pollinfo[idx].fd]));

                        // Read socket
                        txp::Msg* msg = 0;
                        rc = connections[pollinfo[idx].fd]->read(msg);

                        if(rc == 0)
                        {
#if MSG_STALE_CHECK
                            LOG(txp,always) << "responseThread incoming msg "<< (connections[pollinfo[idx].fd]->getFamily() == AF_UNIX ? "AF_UNIX":"SOCK")<<" msg msgId="<<msg->getMsgId()<<" msgNumber="<<msg->getMsgNumber()
                            <<std::hex<<" hex msgId="<<msg->getMsgId()<<" hex msgNumber="<<msg->getMsgNumber() <<std::dec<<" pointer msg="<<msg;
#endif
                            if((msg->getRequestMsgNumber() != 0) &&
                               (msg->retrieveAttrs()->find(txp::responseHandle) != msg->retrieveAttrs()->end()))
                            {
                                ResponseDescriptor* resp = (ResponseDescriptor*)((txp::Attr_uint64*)(msg->retrieveAttrs()->at(txp::responseHandle)))->getData();
                                resp->reply = msg;
                                resp->sempost();
                            }
                            else if (msg->getMsgId() == txp::CORAL_AUTHENTICATE)
                            {
                                connection_authenticate(txp::CORAL_AUTHENTICATE,  connections[pollinfo[idx].fd], msg);
                            }
                            else
                            {
                                pthread_t threadid = 0;

                                // Find available thread
                                pthread_mutex_lock(&threadFreePool_mutex);
                                {
                                    if(threadFreePool.size() > 0)
                                    {
                                        threadid = threadFreePool.back();
                                        threadFreePool.pop_back();
                                    }
                                    if(threadid != 0)
                                    {
                                        threadPool[threadid]->msg  = msg;
                                        Connex * conn = connections[pollinfo[idx].fd];
                                        threadPool[threadid]->connectionName=getConnectionName(conn);
#if BBPROXY
                                        if (connections[pollinfo[idx].fd]->getFamily() == AF_UNIX)
                                        {
                                          threadPool[threadid]->uid=conn->getUser_ID();
                                          threadPool[threadid]->gid=conn->getGrp_ID();
                                        }
                                        else
                                        {
                                          threadPool[threadid]->uid=-1;
                                          threadPool[threadid]->gid=-1;
                                        }
#endif
                                        pthread_mutex_unlock(&threadFreePool_mutex);
                                        sem_post(&threadPool[threadid]->workAvailable);
                                    }
                                    else
                                    {
                                        FL_Write(FLConn, FL_AddBacklog, "Adding message to backlog.  Backlog depth=%ld", threadBacklog.size(),0,0,0);

                                        // backlog
                                        /// \todo Add limit on the total size of backlog messages (and throw RAS)
                                        threadState* bl  = new(threadState);
                                        Connex * conn = connections[pollinfo[idx].fd];
#if BBPROXY
                                        if (connections[pollinfo[idx].fd]->getFamily() == AF_UNIX)
                                        {
                                          bl->uid=conn->getUser_ID();
                                          bl->gid=conn->getGrp_ID();
                                        }
                                        else
                                        {
                                          bl->uid=-1;
                                          bl->gid=-1;
                                        }
#endif
                                        bl->msg = msg;
                                        bl->connectionName=getConnectionName(conn);
                                        threadBacklog.push_back(bl);
                                        pthread_mutex_unlock(&threadFreePool_mutex);
                                    }
                                }
                            }
                        }
                        else //rc!=0
                        {
                            if (connections[pollinfo[idx].fd]->getFamily() != AF_UNIX)
                            {
                                FL_Write(FLConn, FL_ReadError, "Error reading the connection, closing connection.  rc=%ld fd=%ld", rc,pollinfo[idx].fd,0,0);
                                bberror << errloc(rc);
                                bberror << err("error.fileDescriptor",pollinfo[idx].fd);
                                bberror << err("error.text",strerror(-rc));
                                bberror << err("error.connection", connections[pollinfo[idx].fd]->getInfoString() );
                                bberror << RAS(bb.net.SockConnErr);
                            }
                            std::string nameWas = cleanupConnection(pollinfo[idx].fd);//calls releaseReplyWaiters
#if BBPROXY
                           //start_bbserverRecoveryThread(nameWas);
#endif
                        }
                    }
                }
                else if(pollinfo[idx].revents & (POLLERR | POLLHUP | POLLNVAL) )
                {
                    FL_Write(FLConn, FL_PollError, "Error polling the connection, closing connection.  fd=%ld POLLERR=%ld POLLHUP=%ld POLLNVAL=%ld",pollinfo[idx].fd,pollinfo[idx].revents & POLLERR ,pollinfo[idx].revents &  POLLHUP ,pollinfo[idx].revents & POLLNVAL);
                    rc=EBADFD;
                    bberror << errloc(rc) << err("error.fileDescriptor",pollinfo[idx].fd)<< err("error.connection", connections[pollinfo[idx].fd]->getInfoString() );
                    bberror << err("error.text", string( (pollinfo[idx].revents & POLLERR) ? "":"POLLERR ") + string( (pollinfo[idx].revents & POLLHUP) ? "":"POLLHUP ") + string( (pollinfo[idx].revents & POLLNVAL) ? "":"POLLNVAL ") );
                    bberror << RAS(bb.net.revent);

                    std::string nameWas = cleanupConnection(pollinfo[idx].fd);//calls releaseReplyWaiters
#if BBPROXY
                   //start_bbserverRecoveryThread(nameWas);
#endif
                }
            }
        }
        free(pollinfo);
    }
    return NULL;
}
