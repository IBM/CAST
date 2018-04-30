/*******************************************************************************
 |    getLastErr.c
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
#include <errno.h>

#include "app_common.h"
#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

int Token = 0;


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


void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize, int token)
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

/***************************************

DEFECT:  Second parameter of BB_GetLastErrorDetails, when 
set to NULL returns an invalid JSON.   One character truncated on the end
compared to if second parameter is passed in for the size of buffer 

****************************************/

            if (token)
                BB_GetLastErrorDetails(pFormat, NULL, l_NumBytesAvailable, *pBuffer);
            else
                BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, l_NumBytesAvailable+1, *pBuffer);

           // BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, l_NumBytesAvailable+1, *pBuffer);
            *bufSize = l_NumBytesAvailable;
//            printf("Error details:  %s\n", *pBuffer);
        }
        else
        {
            *pBuffer = NULL;
            *bufSize = 0;
        }
    }
}

int check(int rc)
{
    if (rc)
    {
        char* errstring = 0;
        size_t  errSz = 0;
        getLastErrorDetails(BBERRORJSON, &errstring, &errSz, 0);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring);
        free(errstring);

        printf("Aborting due to failures\n");
        exit(-1);
    }
    return 0;
}



int main( int argc, char *argv[] ) {
    int rc;
    int rank = 0;
    char* errstring = 0;
    char* command = 0;
    size_t numBytesAvail;
     
    
   rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
   check(rc);

   if(argc == 2)
   {
          // Call getLastError and pass in NULL as size
       if ( strcmp(argv[1],"NULL") == 0 )
          Token = 1;
   }

    
  
   getLastErrorDetails(BBERRORJSON, &errstring, &numBytesAvail, Token);
   command = (char *)malloc(numBytesAvail*sizeof(char)+100);

    /***TESTING JSON FORMAT ***/
   sprintf( command, "echo '%s' | python -mjson.tool", errstring );  /* Format JSON output */
   if (system(command)) {
      printf("ERROR: BB_GetLastErrorDetails JSON Invalid Format: %s\n", errstring);
      free(errstring);
      free(command);
      rc = BB_TerminateLibrary();
      check(rc);
      return 1;
   }

   getLastErrorDetails(BBERRORJSON, &errstring, &numBytesAvail, 0);
   sprintf( command, "echo '%s' | python -mjson.tool", errstring );  /* Format JSON output */
   if (system(command)) {
      printf("ERROR: BB_GetLastErrorDetails JSON Invalid Format: %s\n", errstring);
      free(errstring);
      free(command);
      rc = BB_TerminateLibrary();
      check(rc);
      return 1;
   }


   dumpJSON(errstring, numBytesAvail);
   free(errstring);
   free(command);
   


   printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    return 0;
}
   
       
