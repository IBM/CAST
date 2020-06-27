/*******************************************************************************
 |    bbGrabStderr.cc
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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "bbGrabStderr.h"

    GrabStderr::GrabStderr(){
        stderrPipeFD[0]=-1;
        stderrPipeFD[1]=-1;
        dupRC = 0;
        pipe0RC=0;
        pipe1RC=0;
        stdERRrc=0;
        pipe2(stderrPipeFD, O_CLOEXEC);
        dupSTDERR_FILENO = dup(STDERR_FILENO);
        dupRC = dup2(stderrPipeFD[1],STDERR_FILENO);
        if (dupRC!=-1) {
          pipe0RC=  setNonBlockMode(stderrPipeFD[0]);
          pipe1RC=  setNonBlockMode(stderrPipeFD[1]);
          stdERRrc= setNonBlockMode(STDERR_FILENO);
        }
        else dupRC=-errno;
        
    }
    int GrabStderr::error() const{
        if ( dupRC<0 ) return dupRC;
        if ( pipe0RC<0 ) return pipe0RC;    
        if ( pipe1RC<0 ) return pipe1RC; 
        if ( stdERRrc<0) return stdERRrc; 
        return 0;
    }

    int GrabStderr::getStdErrBuffer(char* pBuffer,const size_t pBuffSize) const
    {
        pBuffer[0]=0;
        int rc=this->error();
        if (rc) return rc; //-errno
        if (stderrPipeFD[0] != -1)
        {
            int buffEnd=read(stderrPipeFD[0],pBuffer,pBuffSize-2);
            pBuffer[pBuffSize-1] = 0;
            if (buffEnd >= 0) 
            {
                pBuffer[buffEnd]=0;
                return buffEnd;
            }
            else
            {
                if(errno==0) 
                    return -999;
                return -errno;
            }
        }
        return 0;
    }

    int GrabStderr::setNonBlockMode(const int pFD) { 
            int flags = fcntl(pFD,F_GETFL);
            flags |= O_NONBLOCK;
            int rc=fcntl(pFD,F_SETFL,flags); 
            if (rc==-1) {
                return -errno;
            }
            return 0;
    }    

    GrabStderr::~GrabStderr(){
        if (stderrPipeFD[0] != -1) close(stderrPipeFD[0]);
        if (stderrPipeFD[1] != -1) close(stderrPipeFD[1]);
        stderrPipeFD[0]=-1;
        stderrPipeFD[1]=-1;
        if (dupSTDERR_FILENO != -1){
            dupRC=dup2(dupSTDERR_FILENO,STDERR_FILENO);//restart stderr fd 
            close(dupSTDERR_FILENO);
            dupSTDERR_FILENO=-1;
        }
    }
