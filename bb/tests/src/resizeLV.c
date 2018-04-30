/*******************************************************************************
 |    resizeLV.c
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


#include "bb/include/bbapiAdmin.h"
#include "bb/include/bbapi.h"




/****************
****GLOBALS
*****************/


/*** Burst Buffer Usage Vars   **/
char        MountPoint[1000]  =  "";
char        NewSize[64]  =  "";
int         PreserveFS   =   1;

 
   //Command line options
static struct option long_options[] = {
        {"mount",               required_argument,       0,  'm' },
        {"size",                required_argument,       0,  's' },
        {"preserve-FS-NOT",     no_argument,             0,  'p' },
        {"help",                no_argument,             0,  'h' },
        {0,                     0,                 0,  0 }
 };


int  parseArgs( int, char**  );
int check(int);
void dumpJSON(char*, size_t );
void getLastErrorDetals(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize);





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
    char *errString=0;
    size_t   errSz = 0;

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




       //..Resizing Logical Volume

    if ( PreserveFS ) 
       rc = BB_ResizeMountPoint(MountPoint, NewSize, BB_NONE);
    else
       rc = BB_ResizeMountPoint(MountPoint, NewSize, BB_DO_NOT_PRESERVE_FS );


    if( rc == 0 ) {
 
        printf("Mount Point %s Successfully Resized..\n", MountPoint);
    }
    else { 
       int unix_err = errno; // saving in case any calls set errno;
       
       printf( "ERROR: BB_ResizeMountPoint() returned %d\n", rc );
       getLastErrorDetals(BBERRORJSON, &errString, &errSz);
       printf("Error details:\n");

             /*  Output JSON DATA  */
       dumpJSON(errString, errSz);
       free(errString);


       printf( "errno = %d: %s\n", unix_err, strerror(unix_err) );
    }

    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    return 0;

}   /******End of Program & main func*******/


int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;


   while ((option = getopt_long(argc, argv,"m:s:ph", long_options, &long_index)) != -1) {
        switch (option) {
             case 'm' : strcpy(MountPoint, optarg);                                 
                 break;
             case 's' : strcpy(NewSize, optarg);                                 
                 break;
             case 'p' : PreserveFS = 0;
                 break;
             case 'h' :
             default: 
                 printf("Usage: %s [-p]  --mount <PATH> --size <VALUE>\n", argv[0]);
                 printf("          -p=Do not preserve FS\n");
                 return -1;
        }
    }



       //  Must pass in MountPoint as a minimum
    if (strlen(MountPoint) == 0 || 
        strlen(NewSize) == 0 ) {
                 printf("Usage: %s [-p]  --mount <PATH> --size <VALUE>\n", argv[0]);
                 printf("          --preserve-FS-NOT\n");
                 return -1;
    } /****..end of if stmt    */



    return 0;


} /****** end of parseArgs func **********/



