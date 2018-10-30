/*================================================================================

    RestApiServer.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <boost/bind.hpp>
#include <signal.h>
#include <iostream>
#include "RestApiServer.h"

#include "logging.h"

using namespace std;


RestApiServer::RestApiServer(const std::string& address, const std::string& port)
  : _enableSsl(true),      // default to use the ssl server...
    _io_service(),
    _acceptor(_io_service),
    _sslContext(boost::asio::ssl::context::sslv23) /*,
    connection_manager_(),
    new_connection_(),
    request_handler_(doc_roots) */
{
  LOG(csmd, debug ) << "RestApiServer: Initializing...";

  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(_io_service);
  boost::asio::ip::tcp::resolver::query query(address, port);
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  _acceptor.open(endpoint.protocol());
  _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  _acceptor.bind(endpoint);
  _acceptor.listen();

  _sslContext.set_options(boost::asio::ssl::context::default_workarounds);
  //setSslParms("server.crt", "server.key", "server.crt");

  // turn on requiring a cert from the client...
  _sslContext.set_verify_mode(boost::asio::ssl::verify_peer | 
                              boost::asio::ssl::verify_fail_if_no_peer_cert | 
                              boost::asio::ssl::verify_client_once);       // require client cert...

  LOG( csmd, debug ) << "RestApiServer: Initializiation complete";
}


void RestApiServer::setSslParms(const std::string &certFile, 
                                const std::string &keyFile,
                                const std::string &certAuthFile)
{
    LOG( csmd, debug ) << "RestApiServer: setting SSL parameters";
    _sslContext.use_certificate_chain_file(certFile);
    _sslContext.use_private_key_file(keyFile, boost::asio::ssl::context::pem);
    _sslContext.load_verify_file(certAuthFile);
}

void RestApiServer::run()
{
  startAccept();        // prime the acceptor...
  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the RestApiServer is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  _io_service.run();
}

void RestApiServer::startAccept()
{

  _newConnection.reset(new RestApiConnection(_io_service, 
                                             _enableSsl?&_sslContext:NULL, 
                                             _connMgr, 
                                             _methodMap));
  _acceptor.async_accept(_newConnection->socket(),
      boost::bind(&RestApiServer::handleAccept, this,
        boost::asio::placeholders::error));
}

void RestApiServer::handleAccept(const boost::system::error_code& e)
{
  // Check whether the RestApiServer was stopped by a signal before this completion
  // handler had a chance to run.
  if (!_acceptor.is_open())
    return;

  RestApiConnectionPtr nc = _newConnection;
  startAccept();      // immediatly start acceptance for the next connnection...

  if (!e)
      _connMgr.start(nc);

  LOG( csmd, info ) << "RestApiServer: Accepted new connection.";
}

void RestApiServer::handleStop()
{
  // The RestApiServer is stopped by cancelling all outstanding asynchronous
  // operations. Once all operations have finished the io_service::run() call
  // will exit.
  _acceptor.close();
  _connMgr.stopAll();
  LOG( csmd, info ) << "RestApiServer: Closed all connections for shutdown.";
}


/**
 * Check that the http method is a valid one...
 * 
 * @param method 
 * 
 * @return int 
 */
int RestApiServer::checkMethod(std::string const &method)
{
    boost::regex validMethodsRx("GET|PUT|HEAD|POST", boost::regex::extended);
    boost::smatch what; 
    bool isMatchFound = boost::regex_match(method, what, validMethodsRx); 
    if (!isMatchFound) {
        return(-1);
    } 
    return(0);
}


int RestApiServer::setUrlCallback(std::string const &method, 
                                  std::string const &url, 
                                  RestApiUrlCallback callback)
{
    int rc;
    rc = checkMethod(method);
    if (rc != 0) 
        return(rc);
    boost::regex urlRx(url);

    string key = method + " " + url;

    _methodMap[key] = shared_ptr<RestApiMethodRec>(new RestApiMethodRec(method, url, urlRx, callback));

    return(0);
}
int RestApiServer::clearUrlCallback(std::string const &method, 
                                    std::string const &url)
{
    int rc;
    rc = checkMethod(method);
    if (rc != 0) 
        return(rc);


    string key = method + " " + url;
    auto it = _methodMap.find(key);
    if (it != _methodMap.end())
        _methodMap.erase(it);

    return(0);
}

