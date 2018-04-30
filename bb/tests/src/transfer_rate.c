/*******************************************************************************
 |    transfer_rate.c
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



/****************
****GLOBALS
*****************/


/*** Burst Buffer Usage Vars   **/
uint64_t    TransferRate   =   0;
int         GetThrottle       =   0;
int         SetThrottle       =   0;
char        MountPoint[1000]  =  "";
char        JSONDumpFile[1000]  =  "";

 
   //Command line options
static struct option long_options[] = {
        {"mount",               required_argument,       0,  'm' },
        {"getThrottle",         no_argument,             0,  'g' },
        {"setThrottle",         required_argument,       0,  's' },
        {"newRate",             required_argument,       0,  'n' },
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

    uint32_t rank = 0;
    char  *successString=0;
    size_t successStringSz=0;

    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }
  

 
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);

       //.. Getting Usage limit first to properly populate BBUsage_t data
	   
	if (GetThrottle)  {
	    rc = BB_GetThrottleRate(MountPoint, &TransferRate);
		if (rc == 0 ) {
		   getLastErrorDetals(BBERRORJSON, &successString, &successStringSz);
		} else {
		   check(rc);
		}
  	
        rc = BB_TerminateLibrary();
        check(rc);
		
		printf("%lu\n", TransferRate);
		
          // Dump Successfull JSON 2 File 
        dumpJSON2File(successString, successStringSz);
        free(successString);
		
		//  Nothing else to do.  We can end the program here
		return 0;

		
	} //..end of if stmt GetThrottle
	else if (SetThrottle)  {
	    rc = BB_SetThrottleRate(MountPoint, TransferRate);
		if (rc == 0 ) {
		   getLastErrorDetals(BBERRORJSON, &successString, &successStringSz);
		} else {
		   check(rc);
		}
 		
        rc = BB_TerminateLibrary();
        check(rc);
		
		printf("ok!\n");
		
           // Dump Successfull JSON 2 File 
        dumpJSON2File(successString, successStringSz);
        free(successString);
		
		//  Nothing else to do.  We can end the program here
		return 0;

		
	} //..end of if stmt SetThrottle
	
	
    printf("Usage: %s --getThrottle --setThrottle\n", argv[0]);
    printf("          --newRate <VALUE> \n");
    printf("          --mount <PATH> \n");
    printf("          --dumpJSON <FILENAME> \n");

	
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);


    return 0;

}   /******End of Program & main func*******/


int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;

   

   while ((option = getopt_long(argc, argv,"m:gsn:hj:", long_options, &long_index)) != -1) {
        switch (option) {
             case 'm' : strcpy(MountPoint, optarg);                                 
                 break;
             case 'g' : GetThrottle = 1;                                 
                 break;
             case 'n' : TransferRate = a64l(optarg);
                 break;
             case 's' : SetThrottle = 1;                                 
                 break;
             case 'j' : strcpy(JSONDumpFile,optarg);
                 break;
             case 'h' :
             default: 
                 printf("Usage: %s --getThrottle --setThrottle\n", argv[0]);
                 printf("          --newRate <VALUE> \n");
                 printf("          --mount <PATH> \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
        }
    }



       //  Must pass in MountPoint as a minimum
    if (strlen(MountPoint) == 0) {
                 printf("Usage: %s --getThrottle --setThrottle\n", argv[0]);
                 printf("          --newRate <VALUE> \n");
                 printf("          --mount <PATH> \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
    } /****..end of if stmt    */



    return 0;


} /****** end of parseArgs func **********/



