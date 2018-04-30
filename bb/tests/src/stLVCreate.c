/*******************************************************************************
 |    stLVCreate.c
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
#include <pthread.h>
#include <unistd.h>


#include "app_common.h"
#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"


#define  MAX_THREADS                1000

/***********  Thread Data    ****/
typedef struct
{
//    BBTransferDef_t* tdef;
//    BBTransferHandle_t thandle;
    char MtPoint[256];
}  Thread_Data;

     /* Thread Prototypes   */
void *runCreateLVs( void *);

// General Prototypes
int  parseArgs( int, char**  );
void createRandomFiles( char* seed, char* filelist, int noOfFls );
int  check(int);
void dumpJSON(char*, size_t );
void dumpJSON2File(char*, size_t );
void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer, size_t *bufSize);
long long  numFormat(const char* inval );


  //Command line options
static struct option long_options[] = {
        {"path",                required_argument,       0,  'p' },
        {"filelist",            required_argument,       0,  'f' },		
        {"noLV",                required_argument,       0,  'n' },
        {"help",                no_argument,             0,  'h' },
        {"dumpJSON",            required_argument,       0,  'j' },
        {0,                     0,                 0,  0 }
 };


/****************
****GLOBALS
*****************/


/*** Burst Buffer Usage Vars   **/
char        Path[1000]  =  "";
char        Filelist[50] = "";                                           // Size of LV for BB_CreateLogicalVOlume
char        JSONDumpFile[1000]  =  "";
int         NoLV   = 0;                                              // Bool:  Call BB_CreateLogicalVolume
int ThreadSz = MAX_THREADS;                                          // Number of threads to create for test

void createRandomFiles( char* path, char* filelist, int noOfFls )
{


   char   mstr[400]="";
   char   flNm[400]="";
   int    n = 0, c = 0;
   FILE  *fp;


       /***** Initialize memory **********/
  memset(mstr, '\0',400);
   memset(flNm, '\0',400);


   sprintf(mstr, "%s/%s", path,filelist);
   struct stat st;
   if(stat(path,&st) != 0) {
        printf("Aborting:  %s was not found!!\n", path);
		exit(-1);
   }
   
   strcpy(filelist, mstr);
   
   if( !(fp = fopen(filelist,"w")) ) {
     fprintf( stderr, "failed to open %s\n", filelist );
     exit(-1);
   }
   
       // Generates a random number usind current time as seed
   srand(time(NULL)); // randomize seed
   for (c = 1; c <= noOfFls; c++) {
   

    n = (rand() % 1000) + 1;;  // Generate number btwn 1,1000
 	sprintf(flNm,"%s/FVT_%d", path,n);  // Construct file name with Random number
	fprintf(fp,"%s\n", flNm);             // Write to filelist
   }  //.. end of for stmt
   
   fclose(fp);
 
}  /** end of createRandomFiles func   **/



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
       // printf("Aborting due to failures\n");
        //exit(-1);
    }
	
	return 0;
}

/*********************
Main Entry
************************/

int main( int argc, char *argv[] ) {
    int rc, i=0;
    int rank = 0;
  //  char *errString=0, *successString=0;
  //  size_t   errSz = 0, successStringSz=0;
    char *successString=0;
    size_t successStringSz=0;

	pthread_t* tid;                             // Number of initial threads
	FILE  *fp;
	char file_line[256];
	char *sfn;
	Thread_Data *pTD;
 
 
    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }

 
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
	
	
	createRandomFiles(Path, Filelist, NoLV );
	ThreadSz = NoLV;
	
	
	  /**********************
     ****Launching Threads - Processing area
    *************************/
	
	
   if( !(fp = fopen(Filelist,"r")) ) {
       fprintf( stderr, "failed to open %s\n", Filelist );
       return -1;
    }

    printf( "Opening for processing filesist  %s\n", Filelist);
	
	tid = malloc(sizeof(pthread_t)*ThreadSz);
    while( fgets( file_line, 255, fp) ) {
        sfn = strtok( file_line, " \n\t" );
        if( !sfn )  continue;
       pTD = malloc(sizeof(Thread_Data));
       memset(pTD, 0, sizeof(Thread_Data)); 
    
       strcpy(pTD->MtPoint, sfn);	
       if (pthread_create(&tid[i++], NULL, runCreateLVs, (void *) pTD))
         return 1;


 
    }  /**end of if while loop*/

 
 
         // wait until all threads end before exiting 
    for(i = 0; i < ThreadSz; i++)  {
       pthread_join(tid[i], NULL);
    } //..end of for loop ThreadSz 

    fclose(fp );
	
    free(tid);
	
	
	
	
	
	
	
    
   
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


   while ((option = getopt_long(argc, argv,"p:f:n:j:h", long_options, &long_index)) != -1) {
        switch (option) {
             case 'p' : strcpy(Path, optarg);                                 
                 break;
             case 'f' : strcpy(Filelist, optarg);                                 
                 break;
             case 'n' : NoLV = atoi(optarg);                                 
                 break;
             case 'j' : strcpy(JSONDumpFile,optarg);
                 break;
             case 'h' :
             default: 
                 printf("Usage: %s --noLV <NN> --path <LOCATION> \n", argv[0]);
	             printf("          --filelist <FILENAME> \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
        }
    }


       //  Verify Parameters
    if (strlen(Path) == 0 || strlen(Filelist) == 0 || NoLV <= 0 ) 	{
                 printf("Usage: %s --noLV <NN> --path <LOCATION> \n", argv[0]);
	             printf("          --filelist <FILENAME> \n");
                 printf("          --dumpJSON <FILENAME> \n");
     	         return -1;
	} /****..end of if stmt    */

    return 0;


} /****** end of parseArgs func **********/

/*******Thread***/ 
void *runCreateLVs( void *arg)
{
 

   Thread_Data*  pTD = (Thread_Data*)arg;
   char*  MP = pTD->MtPoint;

    int rc = 0;


//printf("DEBUG: INSIDE runCreateLVs Thread -> %d\n", (int)pthread_self());
printf("DEBUG: INSIDE runCreateLVs Thread -> %lu\n", pthread_self());

 //   rc = BB_GetTransferHandle(pthread_self(), 1, &rank, &pTD->thandle);  
	rc = BB_CreateDirectory(MP);
        check(rc);
	
	/********************
	   TEST IS FAILING HERE!!!
	**************************************/
	
	rc = BB_CreateLogicalVolume(MP, "500M", BBXFS);
	check(rc);
	if (!rc) printf("Successful BB_CreateLogicalVolume for [%s]\n", MP);
        if (rc) printf("Error  BB_CreateLogicalVolume for [%s]\n", MP);
        //sleep(1);
        //usleep(500*1000);           // Sleep for a half second

	rc = BB_RemoveLogicalVolume(MP);
	check(rc);
        if (!rc) printf("Successful BB_RemoveLogicalVolume for [%s]\n", MP);
        if (rc) printf("Error: BB_RemoveLogicalVolume for [%s]\n", MP); 
	
	rc = BB_RemoveDirectory(MP);
        check(rc);	
 	if (!rc) printf("Successful RemoveDirectory [%s]\n", MP);
        if (rc) printf("Error: BB_RemoveDirectory for [%s]\n", MP); 


    free(pTD);
//    printf("DEBUG: runCreateLVs Thread -> ended... %d\n", (int)pthread_self());
    printf("DEBUG: runCreateLVs Thread -> ended... %lu\n", pthread_self());


    return NULL;
} /***  end of runCreateLVs func  */
      
