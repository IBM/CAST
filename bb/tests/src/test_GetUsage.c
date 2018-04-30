/*******************************************************************************
 |    test_GetUsage.c
 |
 |  The purpose of the progam is to verify that the burst buffer API interfaces,
 |  as defined in bbapi.h, match the actual implementations for those APIs.
 |  The intent is that this program should successfully compile and link.
 |  It is not intended for this program to be run.
 |
 |  � Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

/*
typedef struct BBusage
{
    uint64_t totalBytesRead;     ///< Number of bytes written to the logical volume
    uint64_t totalBytesWritten;  ///< Number of bytes read from the logical volume
    uint64_t localBytesRead;     ///< Number of bytes written to the logical volume via compute node
    uint64_t localBytesWritten;  ///< Number of bytes read from the logical volume via compute node
    uint64_t burstBytesRead;     ///< Number of bytes written to the logical volume via burst buffer transfers
    uint64_t burstBytesWritten;  ///< Number of bytes read from the logical volume via burst buffer transfers  
} BBUsage_t;
*/ 

void printBBusage(BBUsage_t* pBBUsage)
{
  printf("totalBytesRead=%ld\n",pBBUsage->totalBytesRead);
  printf("totalBytesWritten=%ld\n",pBBUsage->totalBytesWritten);
  printf("localBytesRead=%ld\n",pBBUsage->localBytesRead);
  printf("localBytesWritten=%ld\n",pBBUsage->localBytesWritten);
  printf("burstBytesRead=%ld\n",pBBUsage->burstBytesRead);
  printf("burstBytesWritten=%ld\n",pBBUsage->burstBytesWritten);
}


int main(int argc, char** argv)
{
/* do declares */
    int rc=0;
    uint32_t l_contribId = 0;
    char* l_MountPoint = "/tmp/GetUsage";
    BBUsage_t l_Usage;
    BBCREATEFLAGS l_CreateFlags = (BBCREATEFLAGS)0;
    char bbsize[]="200M";
    
    size_t l_Size = (size_t)2048;
    char l_CharArrayPtr[l_Size];
    BBERRORFORMAT l_ErrorFormat = (BBERRORFORMAT)0;

    /* setup */
    memset(&l_Usage,0,sizeof(BBUsage_t) );
    rc=BB_InitLibrary(l_contribId, BBAPI_CLIENTVERSIONSTR);
    if (rc){
      printf("InitLibrary failed\n");
      exit(-1);
    }
    rc= BB_CreateLogicalVolume(l_MountPoint, bbsize, l_CreateFlags); 
    if (rc){
      printf("BB_CreateLogicalVolume failed\n");
      rc=BB_GetLastErrorDetails(l_ErrorFormat, &l_Size, l_Size, l_CharArrayPtr);
      printf("%s\n",l_CharArrayPtr);
      exit(-1);
    }
    /* the test */
    rc=BB_GetUsage(l_MountPoint, &l_Usage);
    if (rc){
      printf("BB_GetUsage failed\n");
    }

    /* cleanup  */
    rc=BB_RemoveLogicalVolume(l_MountPoint);
    if (rc){
      printf("BB_RemoveLogicalVolume failed\n");
    }
    rc=BB_TerminateLibrary();
    if (rc){
      printf("BB_TerminateLibrary failed\n");
    }
    return rc;
}
