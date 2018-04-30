/*******************************************************************************
 |    bbcondata.cc
 |
 |  © Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bbconndata.h"
#include <map>
#include <string>

std::map<std::string, Conndata> name2conndata;

pthread_mutex_t conndatalock = PTHREAD_MUTEX_INITIALIZER;



Conndata badConndata;
Conndata* badConndataPtr=&badConndata;

Conndata& conndataFind(const std::string& name)
{
    auto it = name2conndata.find(name);
    if (it != name2conndata.end() ){
       return it->second;
    }
    return badConndata;
}

Conndata* conndataFindPtr(const std::string& name)
{
    auto it = name2conndata.find(name);
    if (it != name2conndata.end() ){
       return &it->second;
    }
    return NULL;
}


uint32_t getContribId(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
  Conndata l_conndata=conndataFind(pConnectionName);  
  pthread_mutex_unlock(&conndatalock);
  return l_conndata._contribID;
}
uint64_t getJobId(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
  Conndata l_conndata=conndataFind(pConnectionName);
  pthread_mutex_unlock(&conndatalock);
  return l_conndata._jobID;
}

uint64_t getJobStepId(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
  Conndata l_conndata=conndataFind(pConnectionName);
  pthread_mutex_unlock(&conndatalock);
  return l_conndata._jobStepID;
}

int setContribId(const std::string& pConnectionName, uint32_t pValue)
{
  int rc=-1;
  pthread_mutex_lock(&conndatalock);
  Conndata* l_conndataPtr = conndataFindPtr(pConnectionName);
  if (l_conndataPtr) {
    l_conndataPtr->_contribID = pValue;
    rc=0;
  }
  pthread_mutex_unlock(&conndatalock);
  return rc;
}
int setJobId(const std::string& pConnectionName, uint64_t pValue)
{
  int rc=-1;
  pthread_mutex_lock(&conndatalock);
  Conndata* l_conndataPtr = conndataFindPtr(pConnectionName);
  if (l_conndataPtr) {
    l_conndataPtr->_jobID = pValue;
    rc=0;
  }
  pthread_mutex_unlock(&conndatalock);
  return rc;
}
int setJobStepId(const std::string& pConnectionName, uint64_t pValue)
{
  int rc=-1;
  pthread_mutex_lock(&conndatalock);
  Conndata* l_conndataPtr = conndataFindPtr(pConnectionName);
  if (l_conndataPtr) {
    l_conndataPtr->_jobStepID = pValue;
    rc=0;
  }
  pthread_mutex_unlock(&conndatalock);
  return rc;
}

void rmvContribId(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
  pthread_mutex_unlock(&conndatalock);
}
void rmvJobId(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
  pthread_mutex_unlock(&conndatalock);
}
void rmvJobStepId(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
  pthread_mutex_unlock(&conndatalock);
}

void rmvConnData(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
    name2conndata.erase(pConnectionName);
  pthread_mutex_unlock(&conndatalock);
}

void addConnData(const std::string& pConnectionName)
{
  pthread_mutex_lock(&conndatalock);
    name2conndata[pConnectionName];
  pthread_mutex_unlock(&conndatalock);
}
