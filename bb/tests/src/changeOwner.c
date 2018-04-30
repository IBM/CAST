/*******************************************************************************
 |    changeOwner.c
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
#include <grp.h>
#include <pwd.h>

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
        size_t  errSz = 0;

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
    char *owner, *group;
    struct group *grp;
    struct passwd *pwd;
    struct stat file_status;
    char* errstring = 0;
    size_t numBytesAvail;
    
    if( argc != 4 )
    {
        printf("testcase <owner> <group> <path>\n"); 
        exit(-1);
    }
    
    owner = argv[1];
    group = argv[2];
    path = argv[3];
    
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    errno = 0;
    rc = BB_ChangeOwner( path, owner, group );
    
   if( rc == 0 ) {
 

       printf("BB details:\n");
       printf( "BB_ChangeOwner() reported success!\n" );
       printf( "BB_ChangeOwner() returned %d\n", rc );
       getLastErrorDetails(BBERRORJSON, &errstring, &numBytesAvail);

             /*  Output JSON DATA  */
       dumpJSON(errstring, numBytesAvail);
       free(errstring);
   }
   else { 
       int unix_err = errno; // saving in case any calls set errno;
       
       printf( "ERROR: BB_ChangeOwner() returned %d\n", rc );
       getLastErrorDetails(BBERRORJSON, &errstring, &numBytesAvail);
       printf("Error details:\n");

             /*  Output JSON DATA  */
       dumpJSON(errstring, numBytesAvail);
       free(errstring);


       printf( "errno = %d: %s\n", unix_err, strerror(unix_err) );
   }
        
   /* Check Stat */
   printf( "\nChecking ownership of path\n" );
   errno = 0;
   if( stat(path, &file_status) ) {
       printf( "attempt to open path was not successful\n" );
       printf( "errno = %d: %s\n", errno, strerror(errno) );
       
       exit( errno );
   }
   
   errno = 0;    
   if( !(grp = getgrnam(group)) || !(pwd = getpwnam(owner)) ) {
       printf( "Could not retrieve either group or owner\n" );
       printf( "errno = %d: %s\n", errno, strerror(errno) );
       
       exit( errno );
   }
   
   if( grp->gr_gid != file_status.st_gid ) {
       printf( "Groups don't match\n" );
       exit( EXIT_FAILURE );
   }
   
   if( pwd->pw_uid != file_status.st_uid ) {
       printf( "Owners don't match\n" );
       exit( EXIT_FAILURE );
   }
       
   printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    printf( "Success\n" );
    return 0;
}
    
    
    
        
