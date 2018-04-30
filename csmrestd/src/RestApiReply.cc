/*================================================================================

    RestApiReply.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "RestApiReply.h"
#include <string>
#include <boost/lexical_cast.hpp>


namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(RestApiReply::status_type status)
{
  switch (status)
  {
  case RestApiReply::ok:
    return boost::asio::buffer(ok);
  case RestApiReply::created:
    return boost::asio::buffer(created);
  case RestApiReply::accepted:
    return boost::asio::buffer(accepted);
  case RestApiReply::no_content:
    return boost::asio::buffer(no_content);
  case RestApiReply::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case RestApiReply::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case RestApiReply::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case RestApiReply::not_modified:
    return boost::asio::buffer(not_modified);
  case RestApiReply::bad_request:
    return boost::asio::buffer(bad_request);
  case RestApiReply::unauthorized:
    return boost::asio::buffer(unauthorized);
  case RestApiReply::forbidden:
    return boost::asio::buffer(forbidden);
  case RestApiReply::not_found:
    return boost::asio::buffer(not_found);
  case RestApiReply::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case RestApiReply::not_implemented:
    return boost::asio::buffer(not_implemented);
  case RestApiReply::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case RestApiReply::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> RestApiReply::to_buffers()
{
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(status_strings::to_buffer(_status));
  for (std::size_t i = 0; i < _headers.size(); ++i)
  {
    RestApiKvPair& h = _headers[i];
    buffers.push_back(boost::asio::buffer(h._name));
    buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
    buffers.push_back(boost::asio::buffer(h._value));
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  }
  buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  buffers.push_back(boost::asio::buffer(_content));
  return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "{\"status\": \"Created\"}";
const char accepted[] =
  "{\"status\": \"Accepted \"}";
const char no_content[] =
  "{\"status\": \"No Content\"}";
const char multiple_choices[] =
  "{\"error\": \"Multiple Choices \"}";
const char moved_permanently[] =
  "{\"error\": \"Moved Permanently \"}";
const char moved_temporarily[] =
  "{\"error\": \"Moved Temporarily \"}";
const char not_modified[] =
  "{\"error\": \"Not Modified \"}";
const char bad_request[] =
  "{\"error\": \"Bad Request\"}";
const char unauthorized[] =
  "{\"error\": \"Unauthorized\"}";
const char forbidden[] =
  "{\"error\": \"Forbidden\"}";
const char not_found[] =
  "{\"error\": \"Not Found\"}";
const char internal_server_error[] =
  "{\"error\": \"Internal Server Error\"}";
const char not_implemented[] =
  "{\"error\": \"Not Implemented\"}";
const char bad_gateway[] =
  "{\"error\": \"Bad Gateway\"}";
const char service_unavailable[] =
  "{\"error\": \"Service Unavailable\"}";

std::string to_string(RestApiReply::status_type status)
{
  switch (status)
  {
  case RestApiReply::ok:
    return ok;
  case RestApiReply::created:
    return created;
  case RestApiReply::accepted:
    return accepted;
  case RestApiReply::no_content:
    return no_content;
  case RestApiReply::multiple_choices:
    return multiple_choices;
  case RestApiReply::moved_permanently:
    return moved_permanently;
  case RestApiReply::moved_temporarily:
    return moved_temporarily;
  case RestApiReply::not_modified:
    return not_modified;
  case RestApiReply::bad_request:
    return bad_request;
  case RestApiReply::unauthorized:
    return unauthorized;
  case RestApiReply::forbidden:
    return forbidden;
  case RestApiReply::not_found:
    return not_found;
  case RestApiReply::internal_server_error:
    return internal_server_error;
  case RestApiReply::not_implemented:
    return not_implemented;
  case RestApiReply::bad_gateway:
    return bad_gateway;
  case RestApiReply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

RestApiReply RestApiReply::stock_reply(RestApiReply::status_type status)
{
  RestApiReply rep;
  rep._status = status;
  rep._content = stock_replies::to_string(status);
  rep._headers.emplace_back(RestApiKvPair("Content-Length",boost::lexical_cast<std::string>(rep._content.size())));
  rep._headers.emplace_back(RestApiKvPair("Content-Type","text/html"));
  return rep;
}


