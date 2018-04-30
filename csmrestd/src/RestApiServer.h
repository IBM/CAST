/*================================================================================

    RestApiServer.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __REST_API_SERVER_H__
#define __REST_API_SERVER_H__


#include <boost/asio.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "RestApiConnection.h"
#include "RestApiMethodMap.h"


// these actually have to be part of the referenced by the connection object...
// 
// callback function for given url and method....
// does this handle the this functions??
// should we use the more flexabile boost bind....??


/// The top-level class of the HTTP server.
class RestApiServer
  : private boost::noncopyable
{
public:
  /// Construct the RestApiServer to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit RestApiServer(const std::string& address, const std::string& port);

  /// Run the RestApiServer's io_service loop.
  void run();

  /// Handle a request to stop the RestApiServer.
  void handleStop();


  /**
   * setSslParams
   * 
   * 
   * @param certFile  --- certificate file (pem format).
   * @param keyFile  -- key file (pem format).
   * @param certAuthFile  -- certerficate authority file..
   */
  void setSslParms(const std::string &certFile, 
                   const std::string &keyFile,
                   const std::string &certAuthFile);

  /**
   * enableSsl -- enable using SSL to connect to the http clients
   *              clients 
   * 
   * @param enable 
   */
  void enableSsl(bool enable) {
      _enableSsl = enable;
  };
  boost::asio::io_service &getIoService() {return(_io_service);};

  
  /**
   * Set the Url callback function for the method and url 
   * indicated. 
   * @param method -- method to call for (GET|PUT|DELETE|POST)
   * @param url -- url regular expression to attach this to...
   * @param callback -- callback function to call for this url..
   * 
   * @return 0 if success, != 0 if failed... 
   *  
   * the call back return code should.. 
   *           return the http return/status code,
   *           0 return will equal HTTP status 200..
   *           non zero codes will be put into the http return
   *           status field. addition error content will be
   *           returned in jsonOUt...
   */
  int setUrlCallback(std::string const &method, 
                      std::string const &url, 
                      RestApiUrlCallback callback);
  /**
   * clear the Url callback function for the method and url 
   * indicated. 
   * @param method -- method to call for (GET|PUT|DELETE|POST)
   * @param url -- url regular expression to attach this to...
   * @param callback -- callback function to call for this url..
   * 
   * @return 0 if success, != 0 if failed...
   */
  int clearUrlCallback(std::string const &method, 
                        std::string const &url);

private:
  /// Initiate an asynchronous accept operation.
  void startAccept();

  /// Handle completion of an asynchronous accept operation.
  void handleAccept(const boost::system::error_code& e);

  int checkMethod(std::string const &method);

  bool _enableSsl; 
  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service _io_service;
  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor _acceptor;
  boost::asio::ssl::context _sslContext;

  /// The next connection to be accepted.
  RestApiConnectionPtr _newConnection;

  /// The connection manager which owns all live connections.
  RestApiConnMgr _connMgr;

  RestApiMethodMap _methodMap;
  
};


#endif // __REST_API_SERVER_H__



