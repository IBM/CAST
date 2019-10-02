/*******************************************************************************
 |    bbcondata.h
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


#ifndef BB_CONNDATA_H
#define BB_CONNDATA_H

#include <string>

#include "BBJob.h"
#include "bbinternal.h"

class Conndata {
  public:
  uint32_t _contribID;
  uint64_t _jobID;
  uint64_t _jobStepID;

  void validateContribId();
  void validateJobId();
  void validateJobStepId();

  Conndata(){_contribID=UNDEFINED_CONTRIBID; _jobID=UNDEFINED_JOBID; _jobStepID=UNDEFINED_JOBSTEPID;}
};


uint32_t getContribId(const std::string& pConnectionName, const VALIDATION_OPTION pValidationOption=DO_NOT_PERFORM_VALIDATION);
uint64_t getJobId(const std::string& pConnectionName, const VALIDATION_OPTION pValidationOption=DO_NOT_PERFORM_VALIDATION);
uint64_t getJobStepId(const std::string& pConnectionName, const VALIDATION_OPTION pValidationOption=DO_NOT_PERFORM_VALIDATION);

int setContribId(const std::string& pConnectionName, uint32_t pValue);
int setJobId(const std::string& pConnectionName, uint64_t pValue);
int setJobStepId(const std::string& pConnectionName, uint64_t pValue);

void rmvConnData(const std::string& pConnectionName);
void addConnData(const std::string& pConnectionName);

#endif //include BB_CONNDATA_H
