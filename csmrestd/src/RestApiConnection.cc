/*================================================================================

    RestApiConnection.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <iterator>     // std::istream_iterator
#include "RestApiConnection.h"
#include "RestApiReply.h"
//#include "RestApiSockStream.h"

using namespace std;

RestApiConnection::RestApiConnection(boost::asio::io_service& io_service, 
                                     boost::asio::ssl::context *context,
                                     RestApiConnMgr &connMgr,
                                     RestApiMethodMap &methodMap)
  : 
    //_socketocket(io_service, context),
    //_socket(io_service),
    _io_service(io_service),
    _connMgr(connMgr),
    _methodMap(methodMap),
    _reqTimeout(300),
    _contentTimeout(2),
    _open(true)
{
    // if we have a context, then we want to do https TLS security...
    if (context) {
        _httpsSocket.reset(new HTTPS(_io_service, *context));
    } else {
        _httpSocket.reset(new HTTP(_io_service));
    }
}



void RestApiConnection::start()
{
    boost::asio::ip::tcp::no_delay option(true);
    if (_httpsSocket)
        socket().set_option(option);


    if (_httpsSocket)
        startSslHandshake();
    else
        readHttpRequest();        

}

void RestApiConnection::stop()
{
    _open = false;
    if (_httpsSocket)
        _httpsSocket->lowest_layer().close();
    if (_httpSocket)
        _httpSocket->close();

}



void RestApiConnection::startSslHandshake()
{
    if (_httpsSocket) {
        _httpsSocket->async_handshake(boost::asio::ssl::stream_base::server,
            boost::bind(&RestApiConnection::handleSslHandshake, this,
              boost::asio::placeholders::error));
    } 
    else {
        stop();
    }
}

void RestApiConnection::handleSslHandshake(const boost::system::error_code& err)
{
    if (!chkErr(err)) {
        return;
    }

    readHttpRequest();      // go on to do the next step, we should now be secure...
}

/**
 * readHttpRequest
 * 
 * Read the request from the socket and 
 * 
 */
void RestApiConnection::readHttpRequest() 
{
    if (!_open)
        return;

    // timeouts....??
    _httpRequest.reset(new RestApiRequest());
    if (_httpsSocket) {
        // Read the response headers, which are terminated by a blank line.
        boost::asio::async_read_until(*_httpsSocket, _sbuff, "\r\n\r\n",
              boost::bind(&RestApiConnection::readHttpHeaders, this, _1, _2));
    }
    else {
        boost::asio::async_read_until(*_httpSocket, _sbuff, "\r\n\r\n",
              boost::bind(&RestApiConnection::readHttpHeaders, this, _1, _2));
    }

}



void RestApiConnection::readHttpHeaders(const boost::system::error_code& err, std::size_t bytes_transferred)
{

    if (!chkErr(err)) {
        return;
    }

    // headers consist of the http command line followed by the header, terminated by two line separators..
    //request->streambuf.size() is not necessarily the same as bytes_transferred, from Boost-docs:
    //"After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
    //The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
    //streambuf (maybe some bytes of the content) is appended to in the async_read-function below (for retrieving content).
    size_t num_additional_bytes=_sbuff.size()-bytes_transferred;

    // just dump to stdout for now...
    std::istream respStream(&_sbuff);
    std::string header;
    unsigned l = 0;
    //boost::regex reqLineRx("(\\S+) (\\S+) (\\S+)", boost::regex::extended);
    boost::regex reqLineRx("(\\S+) (\\S+) (\\S+).*", boost::regex::extended);
    boost::regex headerLineRx("(\\S+): (.*)", boost::regex::extended);
    while (std::getline(respStream, header) && header != "\r") {    
        if (l++ == 0) {
            // first line.
            boost::smatch what; 
            bool isMatchFound = boost::regex_match(header, what, reqLineRx); 
            if (!isMatchFound) {
                // todo, write reply, error code.  
                readHttpRequest();      // queue up the next read...
                return;
            }
            _httpRequest->setMethod(what[1]);
            _httpRequest->setUrl(what[2]);
            _httpRequest->setHttpVersion(what[3]);
        }
        else {
            // the rest..., key value pairs...
            boost::smatch what; 
            bool isMatchFound = boost::regex_match(header, what, headerLineRx); 
            if (isMatchFound)  {
                _httpRequest->setHeaderKeyValue(what[1], what[2]);
            }
            else {
                // todo, write reply, error code.  
                readHttpRequest();      // queue up the next read...
            }
        }

    }
    // do we have a content length??
    if (_httpRequest->headerHasKey("Content-Length")) {

        unsigned contentLength = strtoull(_httpRequest->getHeaderKeyValue("Content-Length").c_str(), NULL, 10);
        if(contentLength > num_additional_bytes) {
            unsigned len = contentLength-num_additional_bytes;
            if (_httpsSocket) {
                boost::asio::async_read(*_httpsSocket, _sbuff,
                        boost::asio::transfer_exactly(len),
                        boost::bind(&RestApiConnection::readHttpContent, this, _1, _2));
            } 
            else {
                boost::asio::async_read(*_httpSocket, _sbuff,
                        boost::asio::transfer_exactly(len),
                        boost::bind(&RestApiConnection::readHttpContent, this, _1, _2));
            }
        } 
        else {
            // we already have it in our stream buffer, process it directly..
            readHttpContent(err,0);
        }
    }
    else {
        replyHttp();
    }
}


void RestApiConnection::readHttpContent(const boost::system::error_code& err,std::size_t bytes_transferred)
{

    if (!chkErr(err)) 
        return;
    //std::istream respStream(&_sbuff);
    std::string str( (std::istreambuf_iterator<char>(&_sbuff)), std::istreambuf_iterator<char>() );
    //std::string str( std::istreambuf_iterator<char>(&respStream), std::istreambuf_iterator<char> );
    _httpRequest->appendContent(str);

    replyHttp();

}

void RestApiConnection::replyHttp()
{
    //RestApiReply rep = RestApiReply::stock_reply(RestApiReply::bad_request);
    handleHttpUrl(_reply);      // _reply needs to stay around until the write completion routine happens...
    // chain to the next thing...
    if (_httpsSocket) {
        boost::asio::async_write(*_httpsSocket, _reply.to_buffers(),
              boost::bind(&RestApiConnection::handleHttpWrite, shared_from_this(),
                boost::asio::placeholders::error));
    }
    else {
        boost::asio::async_write(*_httpSocket, _reply.to_buffers(),
              boost::bind(&RestApiConnection::handleHttpWrite, shared_from_this(),
                boost::asio::placeholders::error));
    }

}

void RestApiConnection::handleHttpWrite(const boost::system::error_code& err)
{

  if (!chkErr(err)) 
      return;

  // we are done, schedule the next one...
  readHttpRequest();      // queue up the next read...
}
/**
 * Handle the current http request for the current url..
 * 
 * 
 * @return reply -- reply to use...
 */
void RestApiConnection::handleHttpUrl(RestApiReply &reply)
{

    for (auto const & item: _methodMap) {
        boost::smatch what; 
        shared_ptr<RestApiMethodRec>  methRec = item.second;
        if (methRec->_method != _httpRequest->getMethod())
            continue;
        bool isMatchFound = boost::regex_match(_httpRequest->getUrl(), what, methRec->_urlRx); 
        if (isMatchFound) {
            // todo, call the functor and return the result...

            string jsonOut;
            RestApiReply::status_type rc  = methRec->_urlCallback(_httpRequest->getMethod(),
                                                                  _httpRequest->getUrl(), 
                                                                  _httpRequest->getContent() , 
                                                                  jsonOut);

            reply.clear();
            if (!rc) {
                reply._status = RestApiReply::ok;
                reply._content = jsonOut;
            } 
            else {
                reply._status = rc;     // patch in the return code...
                reply._content = jsonOut;   // and any error content...
            }
            reply._headers.emplace_back(RestApiKvPair("Content-Length", boost::lexical_cast<std::string>(reply._content.size())));
            reply._headers.emplace_back(RestApiKvPair("Content-Type","application/json"));
            // ad dthis to all headers to keep the connettion alive
            //    client frameworks pay attention to this to keep the tcp connection open...
            reply._headers.emplace_back(RestApiKvPair("connection","keep-alive"));
            return;
        }
    }   

    reply = RestApiReply::stock_reply(RestApiReply::bad_request);
    // ad dthis to all headers to keep the connettion alive
    reply._headers.emplace_back(RestApiKvPair("connection","keep-alive"));
}


/**
 * Check for error conditions and take action on the connection 
 * if we need to... 
 * 
 * @param err 
 * @return bool -- true if no error, fasle if we have an error.
 */
bool RestApiConnection::chkErr(const boost::system::error_code& err) 
{
    if (err)
    {
      // Initiate graceful connection closure.
      boost::system::error_code ignored_ec;
      if (_httpsSocket)
          _httpsSocket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
      if (_httpSocket)
          _httpSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
      _connMgr.stop(shared_from_this());
    }
    return(!err);

}

////////////////////////////////////////////////////////////////
// Connection Manager stuff
////////////////////////////////////////////////////////////////
void RestApiConnMgr::start(RestApiConnectionPtr c)
{
  _connections.insert(c);
  c->start();
}



/**
 * stop -- stop a single connection.
 * 
 * 
 * @param c 
 */
void RestApiConnMgr::stop(RestApiConnectionPtr c)
{
  _connections.erase(c);
  c->stop();
}

void RestApiConnMgr::stopAll()
{
  std::for_each(_connections.begin(), _connections.end(),
      boost::bind(&RestApiConnection::stop, _1));
  _connections.clear();
}


