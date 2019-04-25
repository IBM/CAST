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
        pipe2(stderrPipeFD, O_CLOEXEC);
        dupSTDERR_FILENO = dup(STDERR_FILENO);
        dupRC = dup2(stderrPipeFD[1],STDERR_FILENO);
        setNonBlockMode(stderrPipeFD[0]);
        setNonBlockMode(stderrPipeFD[1]);
        setNonBlockMode(STDERR_FILENO);
    }

    void GrabStderr::getStdErrBuffer(char* pBuffer,const size_t pBuffSize) const{
        pBuffer[0]=0;
        if (stderrPipeFD[0] != -1){
            int buffEnd=read(stderrPipeFD[0],pBuffer,pBuffSize-1);
            if (buffEnd != -1) pBuffer[buffEnd]=0;
        }
        return;
    }

    void GrabStderr::setNonBlockMode(const int pFD) { 
            int flags = fcntl(pFD,F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(pFD,F_SETFL,flags);    
            return; 
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
