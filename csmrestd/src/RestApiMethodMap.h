/*================================================================================

    RestApiMethodMap.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RESTAPI_CALLBACK_H__
#define __RESTAPI_CALLBACK_H__

#include <string>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include "RestApiReply.h"
/**
 * Rest api callback containers...
 *  
 *  
 */
typedef std::function<RestApiReply::status_type (std::string const &method, std::string const &url, std::string const &jsonIn, std::string &jsonOut)> RestApiUrlCallback;
// map of url's to callbacks...

class RestApiMethodRec {
public:
    RestApiMethodRec(std::string const &method,
                     std::string const &url,
                     boost::regex &urlRx,
                     RestApiUrlCallback &urlCallback) :
        _method(method),
        _url(url),
        _urlRx(urlRx),
        _urlCallback(urlCallback) {};


    std::string _method;          // original method string
    std::string _url;             // original url regex.
    boost::regex _urlRx;          // compiled regex...
    RestApiUrlCallback _urlCallback;
protected:

private:

};

// single combined string for method and url...
typedef std::map<std::string, std::shared_ptr<RestApiMethodRec>> RestApiMethodMap;


#endif



