/*******************************************************************************
 |    bb_init.c
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
int    InvokeTwiceSameCredentials = 0;                          // Call BB_TerminateLibrary twice with same Contrib ID and Version
int    InvokeTwiceDifCredentials = 0;                          // Call BB_TerminateLibrary twice with Different/TerminateLibrary Contrib ID and Version
char        JSONDumpFile[1000]  =  "";


   //Command line options
static struct option long_options[] = {
        {"rank",                required_argument,       0,  'r' },
        {"string",              required_argument,       0,  's' },
        {"invokeTwice",         no_argument,             0,  't' },
		{"invokeTwiceDifCredtls", no_argument,           0,  'd' },
        {"dumpJSON",            required_argument,       0,  'j' },
        {0,                     0,                 0,  0 }
 };



int  parseArgs( int, char**  );
int check(int);
void dumpJSON(char*, size_t );
void dumpJSON2File(char*, size_t );
void getLastErrorDetals(BBERRORFORMAT, char** , size_t*);
void alterInitString();





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

void dumpJSON2File(char* data, size_t size)
{
       FILE*  fd = 0; 


       if ( size == 0 || data == 0 )
       {
           return;     //User called func with wrong parms.   Just return
       }

       if ( strlen(JSONDumpFile) == 0 )
       {
           return;     //User does not want JSON Dumped to file.   Just return
       }
      
       fd = fopen(JSONDumpFile,"w");

         //Dumping data to file
       fprintf(fd,"%s",data);

       fclose(fd);


} //..end of dumpJSON2File func


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
        dumpJSON2File(errstring, errSz);
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
    char *successString=0;
    size_t successStringSz=0;
  

  
    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }
  

//   ** FOR DEBUG ONLY
//    rc = BB_InitLibrary(Rank, BBAPI_CLIENTVERSIONSTR);
//	printf("BBAPI_CLIENT=[%s]\n", BBAPI_CLIENTVERSIONSTR);
//    printf("Rank = [%d]\n",Rank);

    if (InvokeTwiceSameCredentials) {
	    rc = BB_InitLibrary(Rank, BB_InitString);
            check(rc); // This should work
		
	    rc = BB_InitLibrary(Rank, BB_InitString);  // THIS SHOULD NOT WORK  
            if ( rc == 0 ) {    
	        // IF WE ARE HERE... It's an error
	       printf("Error rc:       %d\n", rc);
               printf("Error details:  InvokeTwiceSameCredentials Test Failure\n");
               printf("Aborting due to failures\n");
                 // Dump Successfull JSON 2 File 
               dumpJSON2File(successString, successStringSz);
               free(successString);
               exit(-1);	    
            }
		//  GOOD TEST!!
	    printf("Success InvokeTwiceSameCredentials TEST  rc:       %d\n", rc);
		
         // end of if block  		
    } else if ( InvokeTwiceDifCredentials ) {
           rc = BB_InitLibrary(Rank, BB_InitString);
           check(rc); // This should work
		
           alterInitString();
		
           rc = BB_InitLibrary(Rank+99, BB_InitString);  // THIS SHOULD NOT WORK  
           if ( rc == 0 ) {    
		   // IF WE ARE HERE... It's an error
	     printf("Error rc:       %d\n", rc);
             printf("Error details:  InvokeTwiceDifCredentials Test Failure\n");
             printf("Aborting due to failures\n");
                // Dump Successfull JSON 2 File 
             dumpJSON2File(successString, successStringSz);
             free(successString);
             exit(-1);	    
           }
		//  GOOD TEST!!
           printf("Success InvokeTwiceDifCredentials TEST  rc:       %d\n", rc);
	
         // end of else if block  			
    } else {
	    rc = BB_InitLibrary(Rank, BB_InitString);
            check(rc);
 		
    } //..end of else block
	

 	
 
 //   printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

     // Dump Successfull JSON 2 File 
    getLastErrorDetals(BBERRORJSON, &successString, &successStringSz);
    dumpJSON2File(successString, successStringSz);
    free(successString);
    
    printf("Burstbuffer Service Initialized & Terminated Successfully\n");

    return 0;

}   /******End of Program & main func*******/

    
int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;

  
   
   while ((option = getopt_long(argc, argv,"dtr:s:", long_options, &long_index)) != -1) {
        switch (option) {
             case 'r' : Rank = atoi(optarg);
    		 break;
             case 's' : strcpy( BB_InitString, optarg);
                 break;
             case 't' : InvokeTwiceSameCredentials = 1;
                 break;
             case 'd' : InvokeTwiceDifCredentials = 1;
                 break;
             case 'j' : strcpy(JSONDumpFile,optarg);
                 break;
             default: 
                 printf("Usage: %s [--rank <RANK> [-invokeTwice || --invokeTwiceDifCredtls]] --string <BBAPI_CLIENTVERSIONSTR> \n", argv[0]);
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
        }
    }
//    if (strlen(BB_InitString) == 0 ) {
//      printf("Client String not found \n");
//        printf("Usage: %s --string <BBAPI_CLIENTVERSIONSTR> \n", argv[0]);
//        return -1;
//    }


    return 0;


} /****** end of parseArgs func **********/

// Injects an asterisk at the end gitcommit json value
void alterInitString() {
 
    if ( strlen(BB_InitString) > 4 )  {
	   BB_InitString[strlen(BB_InitString)-3] = '*';
	}

	
} //..end of alterInitString func


