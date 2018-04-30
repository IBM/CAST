/*================================================================================

    RestApiRequest.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RESTAPI_REQUEST_H__
#define __RESTAPI_REQUEST_H__


#include <unordered_map>

class RestApiRequest {
public:
#if 0
    Content content;
#endif
    

#if 0
    boost::smatch path_match;
    
    std::string remote_endpoint_address;
    unsigned short remote_endpoint_port;
#endif
    RestApiRequest() {};

    /**
     * setHeaderKeyValue pair.
     * 
     * @param key   -- header key to set.
     * @param value -- header value to set...
     */
    void setHeaderKeyValue(const std::string &key, const std::string &value) {
        _httpHeader[key] = value; };
    
    /**
     * has Key. 
     * check to see if header has a given key 
     * 
     * 
     * @param key 
     * 
     * @return bool 
     */
    bool headerHasKey(const std::string &key) {
        return(_httpHeader.find(key) != _httpHeader.end()); 
    };
    /**
     * getHeaderKeyValue 
     * return the header key value...
     * 
     * @param key 
     * 
     * @return std::string& 
     */
    std::string &getHeaderKeyValue(const std::string &key) {
        // todo, handle not found...
        return(_httpHeader[key]);
    };

    /**
     * Set the http content string, if any...
     * 
     * @param content 
     */
    void setContent(const std::string &content) {
        _content = content; 
    };
    void appendContent(const std::string &content) {
        _content += content; 
    };
    std::string getContent() { return(_content); };

    void setMethod(const std::string &s) {
        _method = s; 
    };
    std::string getMethod() { return(_method); };
    void setUrl(const std::string &s) {
        _url = s; 
    };
    std::string getUrl() { return(_url); };
    void setHttpVersion(const std::string &s) {
        _httpVersion= s; 
    };
    std::string getHttpVersion() { return(_httpVersion); };

private:
    std::unordered_map<std::string, std::string> _httpHeader;      /// container for http header
    
    std::string _method;     // get/PUT/ post etc...
    std::string _url;
    std::string _httpVersion;

    std::string _content;

};

#endif

