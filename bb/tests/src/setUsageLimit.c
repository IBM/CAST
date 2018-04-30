/*******************************************************************************
 |    setUsageLimit.c
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
#include <errno.h>


#include "bb/include/bbapi.h"


/***********  Thread Data    ****/
typedef struct
{
    BBTransferDef_t* tdef;
    BBTransferHandle_t thandle;
    char filename[256];
}  Transfer_Data;
/*****-----------------------------    ****/




/****************
****GLOBALS
*****************/


/*** Burst Buffer Usage Vars   **/
uint64_t    TotalBytesRead    =   0;
uint64_t    TotalBytesWritten =   0;
uint64_t    LocalBytesRead    =   0;
uint64_t    LocalBytesWritten =   0;
uint64_t    BurstBytesRead    =   0;
uint64_t    BurstBytesWritten =   0;
char        MountPoint[1000]  =  "";
char        JSONDumpFile[1000]  =  "";

 
   //Command line options
static struct option long_options[] = {
        {"mount",               required_argument,       0,  'm' },
        {"totalBytesRead",      required_argument,       0,  't' },
        {"totalBytesWritten",   required_argument,       0,  'u' },
        {"localBytesRead",      required_argument,       0,  'l' },
        {"localBytesWritten",   required_argument,       0,  'n' },
        {"burstBytesRead",      required_argument,       0,  'b' },
        {"burstBytesWritten",   required_argument,       0,  'c' },
        {"help",                no_argument,             0,  'h' },
        {"dumpJSON",            required_argument,       0,  'j' },
        {0,                     0,                 0,  0 }
 };


int  parseArgs( int, char**  );
int check(int);
void dumpJSON(char*, size_t );
void dumpJSON2File(char*, size_t );
void getLastErrorDetals(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize);

void dumpJSON2File(char* data, size_t size)
{
       FILE*  fd = 0; 





       if ( size == 0 || data == 0 )
       {
           return;     //User called func with wrong parms.   Just return
       }

       if ( strlen(JSONDumpFile) == 0 )
       {
           return;     //User called func with wrong parms.   Just return
       }
      
       fd = fopen(JSONDumpFile,"w");

         //Dumping data to file
       fprintf(fd,"%s",data);

       fclose(fd);


} //..end of dumpJSON2File func




void dumpJSON(char*  data, size_t size)
{
       char *buf;
       char *command;


       if ( size == 0 || data == 0 )
       {
           return;     //User called func with wrong parms.   Just return
       }

       buf = (char *)malloc(size*sizeof(char));
       command = (char *)malloc(size*sizeof(char)+100);
       memset(buf, '\0', size);
       memset(command, '\0', size+100);
       sprintf( command, "echo '%s' | python -mjson.tool", data );  /* Format JSON output */
       FILE *sz = popen(command, "r");
       while (fgets(buf, sizeof(buf), sz) != 0) {
           printf("%s", buf);
       } /*..end of pstream */
       pclose(sz);

       free(buf);
       free(command);
       printf("Size of JSON: %lu chars\n", size);

} /* end of dumpJSON */


void getLastErrorDetals(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize)
{
    int rc;
    size_t l_NumBytesAvailable;
    if(pBuffer)
    {
        rc = BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, 0, NULL);
        if(rc == 0)
        {
            *pBuffer = (char*)malloc(l_NumBytesAvailable+1);
            memset(*pBuffer, '\0', l_NumBytesAvailable+1);
          //  BB_GetLastErrorDetails(pFormat, NULL, l_NumBytesAvailable, *pBuffer);
            BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, l_NumBytesAvailable+1, *pBuffer);
            *bufSize = l_NumBytesAvailable;
          //  printf("Error details:  %s\n", *pBuffer);
        }
        else
        {
            *pBuffer = NULL;
            *bufSize = 0;
        }
    }
}  /*  ..end of getLastErrorDetals..  */


int check(int rc)
{
    if (rc)
    {
        char* errstring = 0;
        size_t  errSz = 0;
        getLastErrorDetals(BBERRORJSON, &errstring, &errSz);
        dumpJSON2File(errstring,errSz); 
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring);
        free(errstring);

        printf("Aborting due to failures\n");
        exit(-1);
    }
    return 0;
} /**********End of check() func***************/




/***********************
*******PROGRAM ENTRY
***********************/
int main( int argc, char *argv[] ) {
    int rc;

    BBUsage_t bUsg;
    uint32_t rank = 0;
    char *errString=0, *successString=0;
    size_t   errSz = 0, successStringSz=0;

    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }
  

      // Set all values to zero
    memset(&bUsg, 0, sizeof(BBUsage_t));

     /****DEBUG SECTION*******
    printf("ARGS: Size of each Transfer file=%d\n",SzTransFiles);
//    printf("ARGS: THANDLE=%lx\n",THandle);
    printf("ARGS: THANDLE=%lld\n",THandle);
    exit(0);

    ******************/


//    filelist = argv[1];
  //  LV = argv[1];




    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);

       //.. Getting Usage limit first to properly populate BBUsage_t data
    rc = BB_GetUsage(MountPoint, &bUsg);
    check(rc);

    if ( TotalBytesRead    ) bUsg.totalBytesRead=TotalBytesRead;
    if ( TotalBytesWritten ) bUsg.totalBytesWritten=TotalBytesWritten;
    if ( LocalBytesRead    ) bUsg.localBytesRead=LocalBytesRead;
    if ( LocalBytesWritten ) bUsg.localBytesWritten=LocalBytesWritten;
    if ( BurstBytesRead    ) bUsg.burstBytesRead=BurstBytesRead;
    if ( BurstBytesWritten ) bUsg.burstBytesWritten=BurstBytesWritten;



       //.. Setting the limits
    rc = BB_SetUsageLimit(MountPoint, &bUsg);
//    check(rc);

    if( rc == 0 ) {
 
        if ( TotalBytesRead    ) printf("TotalBytesRead Successfully Set..\n");
        if ( TotalBytesWritten ) printf("TotalBytesWritten Successfully Set..\n");
        if ( LocalBytesRead    ) printf("LocalBytesRead Successfully Set..\n");
        if ( LocalBytesWritten ) printf("LocalBytesWritten Successfully Set..\n");
        if ( BurstBytesRead    ) printf("BurstBytesRead Successfully Set..\n");
        if ( BurstBytesWritten ) printf("BurstBytesWritten Successfully Set..\n");
        getLastErrorDetals(BBERRORJSON, &successString, &successStringSz);
    }
    else { 
       int unix_err = errno; // saving in case any calls set errno;
       
       printf( "ERROR: BB_SetUsageLimit() returned %d\n", rc );
       getLastErrorDetals(BBERRORJSON, &errString, &errSz);
       printf("Error details:\n");

             /*  Output JSON DATA  */
       dumpJSON2File(errString,errSz); 
       dumpJSON(errString, errSz);
       free(errString);


       printf( "errno = %d: %s\n", unix_err, strerror(unix_err) );
    }

    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);


    // Dump Successfull JSON 2 File 
    dumpJSON2File(successString, successStringSz);
    free(successString);

    return 0;

}   /******End of Program & main func*******/


int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;


   while ((option = getopt_long(argc, argv,"t:u:l:m:b:c:h", long_options, &long_index)) != -1) {
        switch (option) {
             case 'm' : strcpy(MountPoint, optarg);                                 
                 break;
             case 't' : TotalBytesRead = a64l(optarg);                                 
                 break;
             case 'u' : TotalBytesWritten = a64l(optarg);
                 break;
             case 'l' : LocalBytesRead = a64l(optarg);                                 
                 break;
             case 'n' : LocalBytesWritten = a64l(optarg);
                 break;
             case 'b' : BurstBytesRead = a64l(optarg);                                 
                 break;
             case 'c' : BurstBytesWritten = a64l(optarg);
                 break;
             case 'j' : strcpy(JSONDumpFile,optarg);
                 break;
             case 'h' :
             default: 
                 printf("Usage: %s --totalBytesRead <VALUE> --totalBytesWritten <VALUE>\n", argv[0]);
                 printf("          --localBytesRead <VALUE> --localBytesWritten <VALUE>\n");
                 printf("          --burstBytesRead <VALUE> --burstBytesWritten <VALUE>\n");
                 printf("          --mount <PATH> \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
        }
    }



       //  Must pass in MountPoint as a minimum
    if (strlen(MountPoint) == 0) {
                 printf("Usage: %s --totalBytesRead <VALUE> --totalBytesWritten <VALUE>\n", argv[0]);
                 printf("          --localBytesRead <VALUE> --localBytesWritten <VALUE>\n");
                 printf("          --burstBytesRead <VALUE> --burstBytesWritten <VALUE>\n");
                 printf("          --mount <PATH> \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
    } /****..end of if stmt    */



    return 0;


} /****** end of parseArgs func **********/



