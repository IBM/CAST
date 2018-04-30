/*******************************************************************************
 |    xfer_cancel.cc
 |
 |    Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>


#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

//#include "mpi.h"
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>



class transferDef{
public:
    transferDef(BBTransferHandle_t pThandle){
        _thandle=pThandle;
        _rc=0;
    }
    int createTransferDef(){
        _rc =  BB_CreateTransferDef(&_BBTransferDefptr);
        return _rc;
    }
    ~transferDef(){
        _rc = BB_FreeTransferDef(_BBTransferDefptr);
    }
    int addFiles(const char* source, const char* target){
        _rc = BB_AddFiles(_BBTransferDefptr, source, target, (BBFILEFLAGS)0);
        return _rc;
    }
    int addDirRecur(const char* source, const char* target){
        _rc = BB_AddFiles(_BBTransferDefptr, source,target, BBRecursive);
        return _rc;
    }
    int addFiles(const char * pFileList);
    int startTransfer(){
        _rc = BB_StartTransfer(_BBTransferDefptr, _thandle);
        return _rc;
    }

    int cancelTransfer(){
        _rc = BB_CancelTransfer(_thandle, BBSCOPETRANSFER);
        return _rc;
    }
    BBTransferInfo_t getTransferStatus(){
        BB_GetTransferInfo(_thandle, &_transferInfo);
        return _transferInfo;
    }
private:
    int _rc;
    BBTransferDef_t* _BBTransferDefptr;
    BBTransferHandle_t _thandle;
    BBTransferInfo_t _transferInfo;
};
int transferDef::addFiles(const char * pFileList)
{
    
    
    if(pFileList)
    {
        FILE* fp=NULL;
        char buffer[1024];
        fp = fopen(pFileList, "rd");
        if (!fp)
        {
            return -1;
        }
        while (!feof(fp))
        {
            buffer[0] = 0;
            fgets(buffer, sizeof(buffer), fp);
            if(feof(fp))
                break;
            buffer[strcspn(buffer, "\r\n")] = 0;
            
            std::string text = buffer;
            std::vector<std::string> strs;
            boost::split(strs,text,boost::is_any_of(" \t\r"));
            std::string src, dst, flags;
            if(strs.size() < 2)
            {
                printf("Invalid number of parameters for filelist");
                _rc = -1;
                break;
            }
            src = strs[0];
            dst = strs[1];
            if ( (strs.size() < 3) || (strs[2]=="0") )
            {
                _rc = addFiles( src.c_str(), dst.c_str() );
                if (_rc) break;
            }
            else {
                _rc = addDirRecur( src.c_str(), dst.c_str() );
                if (_rc) break;
            }
        }
        fclose(fp);
    }
    return _rc;
}


class BBclient {
public:
    BBclient(int pRank){
        _rank=pRank;
    }
    int init(){
        _rc = BB_InitLibrary(_rank, BBAPI_CLIENTVERSIONSTR);
        _tag=getpid();
        if (!_rc) _rc = BB_GetTransferHandle(_tag, 1, &_rank, &_thandle);
        return _rc;
    }
    ~BBclient(){
        _rc = BB_TerminateLibrary();
    }
    BBTransferHandle_t getThandle(){return _thandle;}
private:
    int _rc;
    uint32_t _rank;
    BBTransferHandle_t _thandle;
    BBTAG _tag;
};

static inline void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer)
{
    int rc;
    size_t l_NumBytesAvailable;
    if(pBuffer)
    {
        rc = BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, 0, NULL);
        if(rc == 0)
        {
            *pBuffer = (char*)malloc(l_NumBytesAvailable+1);
            BB_GetLastErrorDetails(pFormat, NULL, l_NumBytesAvailable, *pBuffer);
            printf("%s", *pBuffer);
        }
        else
        {
            *pBuffer = NULL;
        }
    }
}

/**********************************************************************
 * general bb helpers
 **********************************************************************/
void printbbret(int rc)
{
    if(rc)
    {
        char* errstring = 0;
        getLastErrorDetails(BBERRORJSON, &errstring);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring);
        free(errstring);
    }
    return;
}

void checkRC(int rc){
    printbbret(rc);
    //if (rc) //MPI_Abort(MPI_COMM_WORLD,1);
}

static char* sourceDir=NULL;
static char* targetDir=NULL;
static char* fileList=NULL;
int  parseArgs( int argc, char * argv[] )
{
    
    int option = 0;
    
    while ((option = getopt(argc, argv,"s:t:f:")) != -1) {
        switch (option) {
            case 's' : sourceDir=optarg;
                break;
            case 't' : targetDir=optarg;
                break;
            case 'f' : fileList=optarg;
                break;
            default:
                printf("%s -f fileList \n",argv[0]);
                printf("%s -s sourceDir -t targetDir",argv[0]);
                return -1;
        }
    }
    return 0;
    
    
} /****** end of parseArgs func **********/


int main( int argc, char *argv[] ) {
    int rc = parseArgs(argc,argv);
    if (rc) return -1;
    int num_procs=0;
    int temp_rank = 1;
    
    optind = 1;
    
    //MPI_Init(&argc, &argv);
    
    //MPI_Comm_rank(MPI_COMM_WORLD, &temp_rank);
    //MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    unsigned int my_rank = (unsigned int) temp_rank;
    if(my_rank == 0)
    {
        printf("RANK 0 of processes: %d\n", num_procs);
    }
    BBclient bbclient(my_rank); //initializes library and terminates when out of scope
    rc=bbclient.init();
    printf("init rc=%d rank=%u\n",rc,my_rank);
    checkRC(rc);
    transferDef xferDef(bbclient.getThandle());
    rc = xferDef.createTransferDef();
    checkRC(rc);
    rc=xferDef.addFiles(fileList);
    checkRC(rc);
    
    //MPI_Barrier(MPI_COMM_WORLD);
    
    rc=xferDef.startTransfer();
    checkRC(rc);
    rc=xferDef.cancelTransfer();
    checkRC(rc);
    BBTransferInfo_t info = xferDef.getTransferStatus();
    int i = 3;
    while (BBNOTSTARTED!=info.status){
        printf("status=%lu i=%u/n",info.status,i--);
        if (!i) break;
        sleep(10);
        info = xferDef.getTransferStatus();
    }
    //MPI_Barrier(MPI_COMM_WORLD);

    //MPI_Finalize();
    return 0;

}   /******End of Program & main func*******/


/*
 typedef struct
 {
 BBTransferHandle_t  handle;                 ///< Transfer handle
 uint32_t            contribid;              ///< Contributor Id
 uint64_t            jobid;                  ///< Job Id
 uint64_t            jobstepid;              ///< Jobstep Id
 BBTAG               tag;                    ///< User specified tag
 uint64_t            numcontrib;             ///< Number of contributors
 BBSTATUS            status;                 ///< Current status of the transfer
 uint64_t            numreportingcontribs;   ///< Number of reporting contributors
 size_t              totalTransferKeyLength; ///< Total number of bytes required to retrieve all
 ///<   key:value pairs using BB_GetTransferKeys()
 size_t              totalTransferSize;      ///< Total number of bytes for the files associated with
 ///<   all transfer definitions
 BBSTATUS            localstatus;            ///< Current status of the transfer for the requesting contributor
 size_t              localTransferSize;      ///< Total number of bytes for the files associated with
 ///<   the transfer definition for the requesting contributor
 } BBTransferInfo_t;
*/
