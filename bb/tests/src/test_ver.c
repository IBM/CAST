/*******************************************************************************
 |    test_ver.c
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



/****************
****GLOBALS
*****************/

uint32_t  Rank = 0;                                            // BB_InitLibrary rank var - DEFAULT
char   BB_InitString[1000] = BBAPI_CLIENTVERSIONSTR;           // BB_InitLibrary String variable - Default
int    SzBuffer   = 0;                                  // Call BB_GetVersion with Buffer Size


   //Command line options
static struct option long_options[] = {
        {"szBuffer  ",         required_argument,       0,  's' },
        {0,                     0,                 0,  0 }
 };



int  parseArgs( int, char**  );
int check(int);
void dumpJSON(char*, size_t );
void getLastErrorDetals(BBERRORFORMAT, char** , size_t*);





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
	char version[256] = "";

  
    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }
  

    rc = BB_InitLibrary(Rank, BB_InitString);
    check(rc); // This should work


    if (SzBuffer) {
	
        char  *lVer = malloc(sizeof(char) * SzBuffer);;
	
        rc = BB_GetVersion(SzBuffer, lVer);  // THIS May or may not work
        free(lVer);
        check(rc);	
         // end of if block  		
    } else {
        rc = BB_GetVersion(sizeof(version), version);  // THIS SHOULD WORK
        check(rc);
 		
    } //..end of else block
	

 	
 
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);
    
    return 0;

}   /******End of Program & main func*******/

    
int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;

  
   
   while ((option = getopt_long(argc, argv,"s", long_options, &long_index)) != -1) {
        switch (option) {
             case 's' : SzBuffer = atoi(optarg);;
                 break;
             default: 
                 printf("Usage: %s --smallBuffer=NN \n", argv[0]);
                 return -1;
        }
    }

    return 0;


} /****** end of parseArgs func **********/



