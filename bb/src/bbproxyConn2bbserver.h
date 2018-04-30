/*******************************************************************************
|    bbproxyConn2bbserver.h
|
|  © Copyright IBM Corporation 2017. All Rights Reserved
|
|    This program is licensed under the terms of the Eclipse Public License
|    v1.0 as published by the Eclipse Foundation and available at
|    http://www.eclipse.org/legal/epl-v10.html
|
|    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
|    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/
#ifndef BBPROXYCONN2BBSERVER_H_
#define BBPROXYCONN2BBSERVER_H_
#include <iostream>
#include <string>
#include <string>
#include <semaphore.h>

#include "CnxSock.h"
#include "CnxSockUnix.h"
#include "CnxSockSSL.h"

#include "bbinternal.h"

txp::Connex* createConnection2bbserver(const std::string& pName);
#endif //BBPROXYCONN2BBSERVER_H_
