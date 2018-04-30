/*================================================================================

    RestApiReply.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RESTAPI_REPLY_H__
#define __RESTAPI_REPLY_H__

#include <string>
#include <vector>
#include <boost/asio.hpp>

struct  RestApiKvPair
{
    RestApiKvPair(std::string const &name, std::string const &value) :
        _name(name),
        _value(value) {};
  std::string _name;
  std::string _value;
};


/// A reply to be sent to a client.
struct RestApiReply
{
    void clear() {
        _headers.clear();
    };
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } _status;

  /// The headers to be included in the reply.
  std::vector<RestApiKvPair> _headers;

  /// The content to be sent in the reply.
  std::string _content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers();

  /// Get a stock reply.
  static RestApiReply stock_reply(status_type status);

};


#endif

