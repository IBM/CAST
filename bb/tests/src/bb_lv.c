/*******************************************************************************
 |    bb_lv.c
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
#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "app_common.h"
#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

int  parseArgs( int, char**  );
int  check(int);
void dumpJSON(char*, size_t );
void dumpJSON2File(char*, size_t );
void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize);
long long  numFormat(const char* inval );


  //Command line options
static struct option long_options[] = {
        {"createLV",            no_argument,             0,  'c' },
        {"removeLV",            no_argument,             0,  'r' },
        {"mount",               required_argument,       0,  'm' },
        {"size",                required_argument,       0,  's' },
        {"bbCreateFlag",        required_argument,       0,  'b' },
        {"help",                no_argument,             0,  'h' },
        {"dumpJSON",            required_argument,       0,  'j' },
        {0,                     0,                 0,  0 }
 };


/****************
****GLOBALS
*****************/


/*** Burst Buffer Usage Vars   **/
char        MountPoint[1000]  =  "";
char        LvSz[50] = "";                                           // Size of LV for BB_CreateLogicalVOlume
char        JSONDumpFile[1000]  =  "";
int         CreateFlag = BBXFS;                                      // Create Flag for BB_CreateLogicalVOlume
int         CreateLV   = 0;                                          // Bool:  Call BB_CreateLogicalVolume
int         RemoveLV   = 0;                                          // Bool:  Call BB_RemoveLogicalVolume



void dumpJSON(char*  data, size_t size)
{
       char *buf;
       char *command;


       if ( size == 0 )
           return;     //USer called wrong.   Just return

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
            BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, l_NumBytesAvailable+1, *pBuffer);
           *bufSize = l_NumBytesAvailable;
        }
        else
        {
            *pBuffer = NULL;
            *bufSize = 0;
        }
    }
}


long long  numFormat(const char* inval )
{
   char buf[256]="";
   char command[256]="";
   long long  value = 0;

   if (strlen(inval) == 0 )
       return 0;

   sprintf( command, "numfmt --from=auto %s", inval );  /* numfmt command to convert value */
   FILE *sz = popen(command, "r");
   while (fgets(buf, sizeof(buf), sz) != 0) {
   /****  printf("lsblk reports this [%s] \n", strtok(buf, "\n")); ***/
      sprintf(buf, "%s", strtok(buf, "\n")); 
   } /*..end of pstream */
   pclose(sz);

   /*** value = atoi(buf); */
   value = atoll(buf); 
   return value;
} /*...end of numFormat  */

int check(int rc)
{
    if(rc)
    {
        char* errstring = 0;
        size_t  errSz = 0;
        getLastErrorDetails(BBERRORJSON, &errstring, &errSz);
        
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );
		
        dumpJSON2File(errstring, errSz);
        free(errstring); 
        printf("Aborting due to failures\n");
        exit(-1);
    }
	
	return 0;
}

/*********************
Main Entry
************************/

int main( int argc, char *argv[] ) {
    int rc;
    int rank = 0;
    char *errString=0, *successString=0;
    size_t   errSz = 0, successStringSz=0;
 
 
    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }

 
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
   
   // errno = 0;

    if (CreateLV) {
       printf( "Creating Logical Volume %s size = %s creation-flag=[0x%04X]\n",
                     MountPoint, LvSz, CreateFlag );
   
       rc = BB_CreateLogicalVolume( MountPoint, LvSz, CreateFlag );
 

   
       if( rc == 0 ) {

          printf("BB details:\n");
          printf( "BB_CreateLogicalVolume() reported success!\n" );
          printf( "BB_CreateLogicalVOlume() returned %d\n", rc );
          getLastErrorDetails(BBERRORJSON, &successString, &successStringSz);

                /*  Output JSON DATA  */
          dumpJSON(successString, successStringSz);
 
       }
       else { 
       
        //    int unix_err = errno; // saving in case any calls set errno;
       
          printf( "ERROR: BB_CreateLogicalVolume() returned %d\n", rc );
          getLastErrorDetails(BBERRORJSON, &errString, &errSz);
          printf("Error details:\n");

                /*  Output JSON DATA  */
          dumpJSON(errString, errSz);
	      dumpJSON2File(errString, errSz);
          free(errString);
          //printf("BB details:  %s\n Size of JSON: %zu\n", errstring, numBytesAvail);


 //         printf( "errno = %d: %s\n", unix_err, strerror(unix_err) );
	      return rc;
       } //..end of else blk
   
       printf( "\nChecking Logical Volume\n" );
       errno = 0;



          /*  VERIFYING RESULTS    */

          /*  EXAMPLE:  lsblk | grep /tmp/artis/mnt | awk '{ print $4 }' */

           /*  Build lsblk command returns the size only  */ 

       char command[256];
       sprintf( command, "lsblk | grep %s | awk '{ print $4 }'", MountPoint );

          /*  Execute command and store results in  buffer */ 
       FILE *sz = popen(command, "r");
       char buf[256]="";
       while (fgets(buf, sizeof(buf), sz) != 0) {
          sprintf(buf, "%s", strtok(buf, "\n")); 
       } /*..end of pstream */
       pclose(sz);

       // printf("lsblk returned --> [%lli]\n", numFormat(buf) );
       // printf("Desired Size --> [%lli]\n", numFormat(lv_sz) );
       if ( numFormat(buf) < numFormat(LvSz) ) {
           printf( "ERROR:  BB_CreateLogicalVolume() Volume Size verification Failed: Size=[%s]\n", buf );
           printf( "Exiting:  BB_CreateLogicalVolume() \n");
           exit(-1);

       } /***  ...end of if stmt   */



    }  //..end of if stmt CreateLV	
	else if (RemoveLV) {
	     printf( "Removing  Logical Volume = %s\n",
                     MountPoint);
   
       rc = BB_RemoveLogicalVolume( MountPoint );
   
       if( rc == 0 ) {

          printf("BB details:\n");
          printf( "BB_RemoveLogicalVolume() reported success!\n" );
          printf( "BB_RemoveLogicalVOlume() returned %d\n", rc );
          getLastErrorDetails(BBERRORJSON, &successString, &successStringSz);

                /*  Output JSON DATA  */
          dumpJSON(successString, successStringSz);
 
       }
       else { 
       
        //    int unix_err = errno; // saving in case any calls set errno;
       
          printf( "ERROR: BB_RemoveLogicalVolume() returned %d\n", rc );
          getLastErrorDetails(BBERRORJSON, &errString, &errSz);
          printf("Error details:\n");

                /*  Output JSON DATA  */
          dumpJSON(errString, errSz);
	      dumpJSON2File(errString, errSz);
          free(errString);
          //printf("BB details:  %s\n Size of JSON: %zu\n", errstring, numBytesAvail);


 //         printf( "errno = %d: %s\n", unix_err, strerror(unix_err) );
	      return rc;
	   }
	} // ..end of if stmt RemoveLV
	else {
	   printf("Should not be here! Check Parameters\n");
		
	   rc = BB_TerminateLibrary();
       check(rc);

	   exit(-1);
	} //end of else stmt
   
            /*  Termininating Processing    */ 
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

     // Dump Successfull JSON 2 File 
    dumpJSON2File(successString, successStringSz);
    free(successString);

   
    return 0;
}  //..end of program
 
 
int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;


   while ((option = getopt_long(argc, argv,"crm:b:s:j:h", long_options, &long_index)) != -1) {
        switch (option) {
             case 'c' : CreateLV = 1;                                 
                 break;
             case 'r' : RemoveLV = 1;                                 
                 break;
             case 'm' : strcpy(MountPoint, optarg);                                 
                 break;
 //             case 'c' : CreateFlag = atoi(optarg);
             case 'b' : CreateFlag = strtol(optarg, NULL, 0);
			     break;
             case 's' : strcpy(LvSz,optarg);
                 break;
             case 'j' : strcpy(JSONDumpFile,optarg);
                 break;
             case 'h' :
             default: 
                 printf("Usage: %s [--createLV --size <NN> --bbCreateFlag <BBCREATEFLAG>] \n", argv[0]);
				 printf("          [--removeLV] \n");
                 printf("          --mount <PATH> \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
        }
    }



       //  Verify Parameters
    if (((strlen(LvSz) == 0 || strlen(MountPoint) == 0) && CreateLV ) ||
	      (strlen(MountPoint) == 0 && RemoveLV) ||
          (!CreateLV && !RemoveLV) || (CreateLV && RemoveLV))	{
                 printf("Usage: %s [--createLV --size <NN> --bbCreateFlag <BBCREATEFLAG>] \n", argv[0]);
				 printf("          [--removeLV] \n");
                 printf("          --mount <PATH> \n");
                 printf("          --dumpJSON <FILENAME> \n");
    				return -1;
	} /****..end of if stmt    */

    return 0;


} /****** end of parseArgs func **********/
      
