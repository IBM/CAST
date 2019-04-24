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
class GrabStderr {
public:
    GrabStderr(size_t pBuffSize=2048);
    char * getStdErrBuffer();
    ~GrabStderr();
    void dumpBuff2stderr();
public:
    
private:
    void setNonBlockMode(int pFD);
    size_t buffSize;
    char* buffer;
    int  stderrPipeFD[2];
    int dupSTDERR_FILENO;
    int dupRC;
    int buffEnd;
};
