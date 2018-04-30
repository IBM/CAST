/*================================================================================

    ResapiConnection.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RESTAPI_RestApiConnection_H__
#define __RESTAPI_RestApiConnection_H__

#include <set>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ssl.hpp>

#include "RestApiRequest.h"
#include "RestApiReply.h"
#include "RestApiMethodMap.h"
//#include "RestApiSockStream.h"


typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> HTTPS;
typedef boost::asio::ip::tcp::socket HTTP;

class RestApiConnMgr;
class RestApiRequest;

/// Represents a single RestApiConnection from a client.
class RestApiConnection
  : public boost::enable_shared_from_this<RestApiConnection>,
    private boost::noncopyable
{
public:
  /// Construct a RestApiConnection with the given io_service.
  explicit RestApiConnection(boost::asio::io_service& io_service, 
                             boost::asio::ssl::context *context,
                             RestApiConnMgr &connMgr, 
                             RestApiMethodMap &methodMap);

  /// Get the socket associated with the connection.
  boost::asio::ip::tcp::socket& socket()
  {
      if (_httpsSocket)
          return _httpsSocket->next_layer();
      else
          return *_httpSocket;
  };

  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

  /**
   * Set the request timeout, how long to wait between requests, 
   * before closing the connection. 
   * 
   * @param timeout -- timeout value in seconds
   */
  void setReqTimeout(unsigned timeout) {_reqTimeout=timeout;};

  /**
   * Set the timeout to wait between the request header and the 
   * content section before closing the connection. 
   * 
   * @param timeout -- timeout value in seconds
   */
  void setContentTimeout(unsigned timeout) {_contentTimeout=timeout;};

protected:
   void startSslHandshake();
   void handleSslHandshake(const boost::system::error_code& error);

   /**
   * read the request and content of an http request.
   * 
   */
  void readHttpRequest();


  void readHttpHeaders(const boost::system::error_code& err,std::size_t bytes_transferred);
  void readHttpContent(const boost::system::error_code& err,std::size_t bytes_transferred);

  void handleHttpWrite(const boost::system::error_code& err);
  void handleHttpUrl(RestApiReply &reply);
  void replyHttp();
  bool chkErr(const boost::system::error_code& err);

#if 0
  //
  // wrapper class for boost async read and write calls...
  // todo, transfer to templated class to abstract HTTP and HTTP...
  //
  template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
      void (boost::system::error_code, std::size_t))
  async_read_until(AsyncReadStream& s,
      boost::asio::basic_streambuf<Allocator>& b, const std::string& delim,
      BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
  {
      return(boost::asio::async_read_until(s, b, delim, handler));
  }
    
  template <typename AsyncReadStream, typename Allocator,
    typename CompletionCondition, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
  async_read(AsyncReadStream& s, boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
  {
      return(boost::asio::async_read(s, b, completion_condition, handler));
  }
  template <typename AsyncWriteStream, typename ConstBufferSequence,
    typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
  async_write(AsyncWriteStream& s, const ConstBufferSequence& buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
  {
      return(boost::asio::async_write(s, buffers,  handler));
  }
#endif

  /// Socket for the connection.
  std::shared_ptr<HTTPS> _httpsSocket;
  std::shared_ptr<HTTP>  _httpSocket;


  boost::asio::io_service& _io_service;
  RestApiConnMgr &_connMgr;
  RestApiMethodMap &_methodMap;

  boost::asio::streambuf _sbuff;       /// stream buffer to buffer current reasd

  RestApiReply _reply;

  unsigned _reqTimeout;         // timeout to wait for requests in seconds.
  unsigned _contentTimeout;     // timeout to wait for content after request in seconds.

  std::shared_ptr<RestApiRequest> _httpRequest;

  bool _open;

private:

};


typedef boost::shared_ptr<RestApiConnection> RestApiConnectionPtr;

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class RestApiConnMgr
  : private boost::noncopyable
{
public:
  /// Add the specified connection to the manager and start it.
  void start(RestApiConnectionPtr c);

  /// Stop the specified connection.
  void stop(RestApiConnectionPtr c);

  /// Stop all connections.
  void stopAll();

private:
  /// The managed connections.
  std::set<RestApiConnectionPtr> _connections;

};



#endif

