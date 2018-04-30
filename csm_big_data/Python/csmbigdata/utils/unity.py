#!/usr/bin/env python
# encoding: utf-8

#===========================================================================

# Licensed Materials - Property of IBM

# 5725-K26

# (C) Copyright IBM Corp. 2013 All Rights Reserved.

# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
#===========================================================================

import cookielib
import gettext
import os
import re
import string
import urllib
import urllib2
import urlparse

# Try importing the built-in json package (for 2.6 and higher). If that doesn't
# work (Python 2.5 and lower), use simplejson
try:
    import json
except ImportError:
    import simplejson as json


__version__ = '1.1.0.2'

script_directory = os.path.dirname(os.path.abspath(__file__))
gettext.bindtextdomain('unity', script_directory + '/locales')
gettext.textdomain('unity')
_ = gettext.gettext


def formatted_string(_format, **kwargs):
    """Returns a string with the specified parameters safely substituted into the format string.

    See string.Template for more information about the format string and safe substitution.

    Keyword arguments:
    _format -- The format string to be used. See string.Template.
    kwargs -- A dictionary of keys and values to substitute.
    """
    return string.Template(_format).safe_substitute(kwargs)


class UnityException(Exception):
    """UnityException is the base class for all exceptions raised by the Unity module."""
    def __init__(self, msg, reason=None):
        self.msg = msg
        self.reason = reason

    def __repr__(self):
        return repr(self.msg)

    def __str__(self):
        return str(self.msg)


class UnityAuthenticationRequiredException(UnityException):
    """UnityAuthenticationRequiredException is the exception raised when authentication is required to
    complete an HTTP/HTTPS operation."""
    def __init__(self, msg, reason=None):
        super(UnityAuthenticationRequiredException, self).__init__(msg, reason)


class UnityConnection(object):
    """UnityConnection objects can be used to connect to a Unity server and get/post data.

    Example:
    from unity import UnityConnection, get_response_content
    from urllib import urlencode
    import simplejson as json

    unity_connection = UnityConnection('https://localhost:9987/Unity/', 'spam', 'eggs')
    unity_connection.login()

    response_text = get_response_content(unity_connection.get('/RetentionConfiguration/latest'))
    retention_period = json.loads(response_text)['retentionPeriod']
    print 'The retention period was %d.' % retention_period

    post_data = urlencode({'retentionPeriod': 15})
    response_text = get_response_content(unity_connection.post('/RetentionConfiguration', data=post_data))
    retention_period = json.loads(response_text)['retentionPeriod']
    print 'The retention period is now %d.' % retention_period

    unity_connection.logout()
    """
    def __init__(self, base_uri, username=None, password=None, access_token=None):
        """Initializes a new instance with the specified base URI, username, and password.

        Keyword arguments:
        base_uri -- The base URI for the Unity server, e.g., 'https://example.com:9987/Unity/'
        username -- The username to use when connecting
        password -- The password to use when connecting
        access_token -- The OAuth access token to use when connecting
        """ 
        self.base_uri = re.sub('/*$', '', urllib.quote(base_uri, ':/'))
        self.username = username
        self.password = password
        self.access_token = access_token

        if self.access_token is None and not self.__has_user_credentials():
          raise UnityException(_('Either access_token or username and password must be provided'))

        # Install the cookie processor for our HTTP opener to allow session-based login and save away the CSRF token
        self.cookie_jar = cookielib.CookieJar()
        self.url_opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(self.cookie_jar))
        self.csrf_token = None


    def login(self):
        """Logs into the instance's server, returning a boolean indicating whether login was successful."""
        self.cookie_jar.clear()
        if self.__has_user_credentials():
            self.__login_with_user_credentials()
        self.__update_csrf_token()
        return True


    def __login_with_user_credentials(self):
        """Logs into the instance's server using the instance's username and password."""
        form_values = {'j_username': self.username, 'j_password': self.password, 'action': 'Go'}
        request = urllib2.Request(self.__full_uri('/j_security_check'), urllib.urlencode(form_values))
        try:
            response = self.url_opener.open(request)
        except urllib2.HTTPError, e:
            error = formatted_string(_('An error occurred while logging into $uri. $error.'), uri=self.base_uri, error=e)
            raise UnityException(error, e)
        except urllib2.URLError, e:
            (errno, msg) = e.reason
            error = formatted_string(_('An error occurred while logging into $uri. $error.'), uri=self.base_uri, error=msg)
            raise UnityException(error, e)
        except Exception, e:
            error = formatted_string(_('An error occurred while logging into $uri. $error.'), uri=self.base_uri, error=e)
            raise UnityException(error, e)

        if self.__login_required(response):
            error = formatted_string(_('Failed to log into $uri. Bad username or password.'), uri=self.base_uri)
            raise UnityAuthenticationRequiredException(error)
            


    def __has_user_credentials(self):
        """Returns whether the instance has user credentials."""
        return self.username is not None and self.password is not None
        

    def __login_required(self, response):
        """Examines the specified HTTP response and returns whether login is required."""
        return urlparse.urlparse(response.url)[2].endswith('/Unity/login.jsp')


    def __update_csrf_token(self):
        """Internal method for getting a CSRF token from the server and adding it to our cookie jar.
        Returns the token value.
        """
        response = self.get('/CSRFToken', auto_login=False)
        response_text = response.read()
        response.close()

        json_response = json.loads(response_text)
        if json_response is None or 'CSRFToken' not in json_response:
            raise UnityException(_('An unknown error occurred while getting a CSRF token.'))

        self.csrf_token = json_response['CSRFToken']


    def logout(self):
        """Logs out of the instance's server."""
        try:
            self.get('/jsp/logoff.jsp', params={'CSRFToken': self.csrf_token}, auto_login=False)
        except:
            pass
        self.cookie_jar.clear()
        self.csrf_token = None


    def __enter__(self):
        """Logs in on entry."""
        self.login()
        return self


    def __exit__(self, type, value, tb):
        """Logs out on exit without handling any exceptions."""
        self.logout()
        return False


    def get(self, uri_path, params={}, headers={}, auto_login=True):
        
        """Performs an HTTP GET on the specified path and returns the result.

        Keyword arguments:
        uri_path -- The path (relative to the base URI) to GET.
        params -- A dictionary of parameter names and their values to use.
        headers -- A dictionary of headers to send with the HTTP GET.
        auto_login -- Whether to automatically login or not if the GET requires login.
        """
        params = params.copy()
        if not self.__has_user_credentials() and 'access_token' not in params:
            params['access_token'] = self.access_token
      
            
        request = urllib2.Request(self.__full_uri(uri_path, params), None, headers)
        try:
            response = self.url_opener.open(request)
        except urllib2.URLError, e:
            error = formatted_string(_('An error occurred while executing GET $path. $error.'), path=uri_path, error=e)
            raise UnityException(error, e)

        if self.__login_required(response):
            if auto_login:
                self.login()
                return self.get(uri_path, params, headers, False)

            error = formatted_string(_('An error occurred while executing GET $path: Login required.'), path=uri_path)
            raise UnityAuthenticationRequiredException(error)
        
        return response


    def post(self, uri_path, data, content_type='application/x-www-form-urlencoded', params={}, headers={}, auto_login=True):
        """Performs an HTTP POST on the specified path and returns the result. Adds the connection's CSRF token
        as a parameter ('CSRFToken') if it isn't already included, which it never should be.

        Keyword arguments:
        uri_path -- The path (relative to the base URI) to POST to.
        data -- The data to post.
        content_type -- The content type of the data being posted. Defaults to 'application/x-www-form-urlencoded'.
        params -- A dictionary of URI parameter names and their values. These should be distinct from the data
                  that is being posted. That is, if you are posting to a form, pass those parameters as a URL
                  encoded string via the data argument.
        headers -- A dictionary of headers to send with the HTTP POST. If this dictionary contains the 'Content-Type'
                   key, the content_type parameter is ignored.
        auto_login -- Whether to automatically login or not if the POST requires login.
        """
        params = params.copy()
        if not self.__has_user_credentials() and 'access_token' not in params:
            params['access_token'] = self.access_token

        if 'CSRFToken' not in params:
            params['CSRFToken'] = self.csrf_token

        if 'Content-Type' not in headers:
            headers['Content-Type'] = content_type

        request = urllib2.Request(self.__full_uri(uri_path, params), data, headers)
        try:
            response = self.url_opener.open(request)
        except urllib2.URLError, e:
            error = formatted_string(_('An error occurred while executing POST $path. $error.'), path=uri_path, error=e)
            raise UnityException(error, e)
        
        if self.__login_required(response):
            response.close()
            if auto_login:
                self.login()
                return self.post(uri_path, data, content_type, headers, params, False)

            error = formatted_string(_('An error occurred while executing POST $path: Login required.'), path=uri_path)
            raise UnityAuthenticationRequiredException(error)
        
        return response 

    def delete(self, uri_path, content_type='application/json; charset=UTF-8', params={}, headers={}, auto_login=True):
        """Performs an HTTP DELETE on the specified path and returns the result.

        Keyword arguments:
        uri_path -- The path (relative to the base URI) to DELETE.
        params -- A dictionary of parameter names and their values to use.
        headers -- A dictionary of headers to send with the HTTP DELETE.
        auto_login -- Whether to automatically login or not if the DELETE requires login.
        """
        
        params = params.copy()
        if not self.__has_user_credentials() and 'access_token' not in params:
            params['access_token'] = self.access_token

        if 'CSRFToken' not in params:
            params['CSRFToken'] = self.csrf_token
        
        if 'Content-Type' not in headers:
            headers['Content-Type'] = content_type
        
        request = urllib2.Request(self.__full_uri(uri_path, params), None, headers)
        
        """ !!! This is a major HACK ALERT because urllib2 only supports GET and POST requests.  We really should be 
        using httlib module, which supports GET, POST, PUT, and DELETE.  This hack overrides the get_method to return
        DELETE for the http request.
        """
        request.get_method = lambda: 'DELETE'
        
        try:
            response = self.url_opener.open(request)
        except urllib2.URLError, e:
            error = formatted_string(_('An error occurred while executing DELETE $path. $error.'), path=uri_path, error=e)
            raise UnityException(error, e)

        if self.__login_required(response):
            if auto_login:
                self.login()
                return self.delete(uri_path, params, headers, False)

            error = formatted_string(_('An error occurred while executing DELETE $path: Login required.'), path=uri_path)
            raise UnityAuthenticationRequiredException(error)
        
        return response

    def __full_uri(self, uri_path, params={}):
        """Internal convenience method for composing a full URI given a path and parameters.

        Keyword arguments:
        uri_path -- The path (relative to the base URI).
        params -- A dictionary of parameter names and their values.
        """
        if not uri_path.startswith('/'):
            uri_path = '/' + uri_path

        param_string = ''
        if len(params) != 0:
            param_string = '?' + urllib.urlencode(params)

        return self.base_uri + urllib.quote(uri_path) + param_string


def get_response_content(response):
    """Returns the content of the specified response or None if response is None. This is a convenience function
    for users who are simply interested in the content of a UnityConnection response.

    Keyword arguments:
    response -- The response from a UnityConnection HTTP request.

    Example:
    import unity
    import simplejson as json
    ...
    response = unity.get_response_content(connection.get('/RetentionConfiguration/latest'))
    json_object = json.loads(response)
    """
    if response is None:
        return None
    response_content = response.read()
    response.close()
    return response_content
