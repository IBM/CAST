/*******************************************************************************
 |    changeMode.c
 |
 |   Copyright IBM Corporation 2017. All Rights Reserved.
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

//#include "mpi.h"
#include "app_common.h"
#include "bb/include/bbapiAdmin.h"
#include "bb/include/bbapi.h"


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


void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize)
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
}  /*  ..end of getLastErrorDetails..  */


void check(int rc)
{
    if(rc)
    {
        char* errstring = 0;
        size_t errSz = 0;

        getLastErrorDetails(BBERRORJSON, &errstring, &errSz);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );
        
        printf("Aborting due to failures\n");
        free(errstring);
        exit(-1);
    }
}

int main( int argc, char *argv[] ) {
    int rc;
    int rank = 0;
    char *path;
    struct stat file_status;
    mode_t mode;
    char* errstring = 0;
    size_t numBytesAvail = 0;
    
    if(argc < 3)
    {
        printf("testcase <mode> <path>\n"); // mode is octet
        exit(-1);
    }
    
    mode = (mode_t)strtol( argv[2], NULL, 8 );
    path = argv[1];
    
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    errno = 0;
    rc = BB_ChangeMode( path, mode );
    
   if( rc == 0 ) {


       printf("BB details:\n");
       printf( "BB_ChangeMode() reported success!\n" );
       printf( "BB_ChangeMode() returned %d\n", rc );
       getLastErrorDetails(BBERRORJSON, &errstring, &numBytesAvail);

             /*  Output JSON DATA  */
       dumpJSON(errstring, numBytesAvail);
       free(errstring);

   }
   else { 

       int unix_err = errno; // saving in case any calls set errno;
       
       printf( "ERROR: BB_ChangeMode() returned %d\n", rc );
       getLastErrorDetails(BBERRORJSON, &errstring, &numBytesAvail);
           printf("Error details:\n");

             /*  Output JSON DATA  */
       dumpJSON(errstring, numBytesAvail);
       free(errstring);

       printf( "errno = %d: %s\n", unix_err, strerror(unix_err) );

   }
    
   /* Check Stat */
   printf( "\nChecking  of path\n" );
   errno = 0;
   if( stat(path, &file_status) ) {
       printf( "attempt to open directory was not successful\n" );
       printf( "errno = %d: %s\n", errno, strerror(errno) );
       
       exit( errno );
   }
   
   if((file_status.st_mode & 0777) != mode ) { 
       printf( "ERROR: BB_ChangeMode check-->  set = %o mode retrieved %o\n", mode, 
               file_status.st_mode );
       printf("ABORT: BB_Changemode...\n");
       exit( EXIT_FAILURE );
   }

   printf("mode same as set, Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    return 0;
}
    
    
    
