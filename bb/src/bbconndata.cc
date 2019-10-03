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

#include <map>
#include <string>

#include "bbconndata.h"

std::map<std::string, Conndata> name2conndata;

pthread_mutex_t conndatalock = PTHREAD_MUTEX_INITIALIZER;

Conndata badConndata;
Conndata* badConndataPtr=&badConndata;


void Conndata::validateContribId()
{
    int rc = 0;

    if (_contribID == NO_CONTRIBID || _contribID == UNDEFINED_CONTRIBID)
    {
        rc = -1;
        stringstream errorText;
        errorText << "Invalid contribid value.  Value is " << _contribID;
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    return;
}

void Conndata::validateJobId()
{
    int rc = 0;

    if (_jobID == UNDEFINED_JOBID)
    {
        rc = -1;
        stringstream errorText;
        errorText << "Invalid jobid value.  Value is " << _jobID;
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    return;
}

void Conndata::validateJobStepId()
{
    int rc = 0;

    if (_jobStepID == UNDEFINED_JOBSTEPID)
    {
        rc = -1;
        stringstream errorText;
        errorText << "Invalid jobstepid value.  Value is " << _jobStepID;
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    return;
}

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


uint32_t getContribId(const std::string& pConnectionName, const VALIDATION_OPTION pValidationOption)
{
    pthread_mutex_lock(&conndatalock);
    Conndata l_conndata=conndataFind(pConnectionName);
    pthread_mutex_unlock(&conndatalock);
    if (pValidationOption == PERFORM_VALIDATION)
    {
        l_conndata.validateContribId();
    }

    return l_conndata._contribID;
}

uint64_t getJobId(const std::string& pConnectionName, const VALIDATION_OPTION pValidationOption)
{
    pthread_mutex_lock(&conndatalock);
    Conndata l_conndata=conndataFind(pConnectionName);
    pthread_mutex_unlock(&conndatalock);
    if (pValidationOption == PERFORM_VALIDATION)
    {
        l_conndata.validateJobId();
    }

    return l_conndata._jobID;
}

uint64_t getJobStepId(const std::string& pConnectionName, const VALIDATION_OPTION pValidationOption)
{
    pthread_mutex_lock(&conndatalock);
    Conndata l_conndata=conndataFind(pConnectionName);
    pthread_mutex_unlock(&conndatalock);
    if (pValidationOption == PERFORM_VALIDATION)
    {
        l_conndata.validateJobStepId();
    }

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
