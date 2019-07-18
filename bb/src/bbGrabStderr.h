/*******************************************************************************
 |    bbGrabStderr.h
 |
 |  © Copyright IBM Corporation 2019. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
#include <errno.h>
class GrabStderr {
public:
    GrabStderr();
    int getStdErrBuffer(char* pBuffer,const size_t pBuffSize) const;
    ~GrabStderr();
    int error() const;
    

private:
    int setNonBlockMode(int pFD);
    int  stderrPipeFD[2];
    int dupSTDERR_FILENO;
    int dupRC;
    int pipe0RC;
    int pipe1RC;
    int stdERRrc;
};
