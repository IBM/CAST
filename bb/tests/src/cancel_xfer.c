/*******************************************************************************
 |    cancel_transfer.c
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
#include "bb/include/bbapiAdmin.h"


/***********  Thread Data    ****/
typedef struct
{
    BBTransferDef_t* tdef;
    BBTransferHandle_t thandle;
    char filename[256];
}  Transfer_Data;
/*****-----------------------------    ****/



#define  MSTR_FILE                  "FVT_MASTERFILE"
#define  FVT_SEED                   "FVT_TEST_"
#define  MSTR_FILE_SIZE             4000000
#define  FILE_SIZE                  2000
#define  MAX_THREADS                2 


/****************
****GLOBALS
*****************/

int ThreadSz = MAX_THREADS;                                  // Number of threads to create for test
char   BB_LV[1000] = "";                                    // BB Logical Volume created
char   TARGET_LOC[1000] = "/tmp";
int SzTransFiles  = FILE_SIZE;                              // Number of Transfer files to generate per thread
BBTransferHandle_t  THandle = 0;                               // Transfer handle to cancel
char   TLOC[1000] = "";                                     // Target location of copied files 
int CancelTransfer = 0;                                     // Indicator to cancel transfer only
 





void createRandomFiles( char* seed, char* filelist );
int  cleanUpRandomFiles( char* filelist);
void addTransferFiles( BBTransferDef_t* tdef, char *filelist);
int  parseArgs( int, char**  );
int check(int);
void dumpJSON(char*, size_t );
void getLastErrorDetals(BBERRORFORMAT, char** , size_t*);




     /* Thread Prototypes   */
void *runTransfer( void *);
void *cancelTransfer( void *);





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
}  /*  ..end of getLastErrorDetails..  */


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
}

int checkNoExit(int rc)
{
    if (rc)
    {
        char* errstring = 0;
        size_t  errSz = 0;
        getLastErrorDetals(BBERRORJSON, &errstring, &errSz);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring);
        free(errstring);
    }
    return rc;
} /**********End of check() func***************/




/***********************
*******PROGRAM ENTRY
***********************/
int main( int argc, char *argv[] ) {
    int rc;

   // threads = atoi(argv[2]);
    pthread_t* tid;                             // Number of initial threads
    int i;

    uint32_t rank = 0;
   // FILE *file_list;
   // char *file2open, *filelist;
   // char file_line[256];
    char *errString=0;
    size_t   errSz = 0;
    Transfer_Data* pTD;
    char *LV;

    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }
  

     /****DEBUG SECTION*******
    printf("ARGS: BB_LV=%s\n",BB_LV);
    printf("ARGS: ThreadSz=%d\n",ThreadSz);
    printf("ARGS: Size of each Transfer file=%d\n",SzTransFiles);
//    printf("ARGS: THANDLE=%lx\n",THandle);
    printf("ARGS: THANDLE=%lld\n",THandle);
    exit(0);

    ******************/


//    filelist = argv[1];
  //  LV = argv[1];
      LV = BB_LV;                                         // Grab Logical Volume location
      strcpy(TLOC, TARGET_LOC);                           // Grab Target Location of filelist





    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);

    /* create a logical volume */
    rc= BB_CreateLogicalVolume(BB_LV, "2G",BBXFS);
    if (rc){
      printf("BB_CreateLogicalVolume failed\n");
      checkNoExit(rc);
    }

    /**********************
     ****  Cancel Transfer ONLY - Processing area
    *************************/

    if (CancelTransfer) {

       printf("Attempting to Cancel Transfer for handle->%llu \n", (long long int)THandle );
       rc = BB_CancelTransfer(THandle, BBSCOPETRANSFER);
       check(rc);;
       printf("Successful Cancel Transfer for handle->%llu \n", (long long int)THandle );
       
    } else {  /****** end of CancelTransfer if stmt ******* */

    /**********************
     ****Launching Threads - Processing area
    *************************/

       tid = malloc(sizeof(pthread_t)*ThreadSz);
       for(i = 0; i < ThreadSz; i++)  {

          pTD = malloc(sizeof(Transfer_Data));
          memset(pTD, 0, sizeof(Transfer_Data)); 
          sprintf(pTD->filename,"%s/FVT_filelist_%d", LV,rand()%1000+1);   // Creates random set of filename list in range of 0-1000
          //strcpy(pTD->filename, filelist );
          if (pthread_create(&tid[i], NULL, runTransfer, (void *) pTD))
            return 1;
       } //..end of for loop ThreadSz 

          // wait until all threads end before exiting 
          for(i = 0; i < ThreadSz; i++)  {
             pthread_join(tid[i], NULL);
       } //..end of for loop ThreadSz 

       free(tid);

    }  /*******  End of else if (CancelTransfer) stmt    *******/


//    BBTransferDef_t* tdef;
//    BBTransferHandle_t thandle;
//    printf("Obtaining transfer handle\n");
//    rc = BB_GetTransferHandle(getpid(), 1, &rank, &thandle); /* \todo tag generation uses getpid() - need something better */
//    check(rc);

//    printf("Creating transfer definition\n");
//    rc = BB_CreateTransferDef(&tdef);
//    check(rc);

//       /***   adding files to Transfer Definition  */
//    addTransferFiles( tdef, file2read);


//       /***   Startin Transfer  */
//    printf("Starting transfer\n");
//    rc = BB_StartTransfer(tdef, thandle);
//    check(rc);

    getLastErrorDetals(BBERRORJSON, &errString, &errSz);
    printf("Cancel Transfer  rc:       %d\n", rc);
    printf("Cancel Transfer JSON details:\n");
    dumpJSON(errString, errSz);

    free(errString);
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    return 0;

}   /******End of Program & main func*******/


void createRandomFiles( char* seed, char* filelist )
{

   char   command[2000]="";
   char   dirNm[600]=""; 
   char   mstr[400]="", buf[2000]="";
   char   srcFl[400]="";
   int systemrc=0;


       /***** Initialize memory **********/
   memset(command, '\0',2000);
   memset(buf, '\0',2000);
   memset(dirNm, '\0',600);
   memset(mstr, '\0',400);
   memset(srcFl, '\0',400);

       /** Extract Directory Name from Filelist   **/
   sprintf( command, "dirname %s", filelist );  /* Format command */
   FILE *sz = popen(command, "r");
   while (fgets(buf, sizeof(buf), sz) != 0) {
        sprintf(buf,"%s", strtok(buf,"\n"));
   } /*..end of pstream */
   pclose(sz);
   strcpy(dirNm,buf);


   sprintf(mstr, "%s/%s", dirNm,MSTR_FILE);
   if( access( mstr, F_OK ) == -1 ) {
         /* Master file does not exist   CREATING IT */
     sprintf(command, "dd if=/dev/urandom of=%s bs=1 count=%d >/dev/null 2>&1", mstr, MSTR_FILE_SIZE);
     system(command); 
   }  /*   end of if stmt */


      /* Creating X number of files of size 'SzTransFiles', with random data in it */
   sprintf(srcFl,"%s/%s",dirNm,seed);
   sprintf(command, "split -b %d -a 20 %s %s", SzTransFiles, mstr,srcFl);
   system(command); 

      /* Creating the file list  */
   sprintf(command, "ls -A1 %s*| awk '{print $0 \"  %s\"}' >> %s", srcFl, TLOC, filelist);
   systemrc = system(command); 
   if (systemrc>0) {
      printf("systemrc=%d for command=%s \n",systemrc,command);
   }
   
}  /** end of createRandomFiles func   **/

void addTransferFiles( BBTransferDef_t* tdef, char *file2open)
{
    FILE *filelist;
    char file_line[256];
    char *sfn, *tfn;
    int rc = 0;



    printf( "opening %s\n", file2open );
    if( !(filelist = fopen(file2open,"r")) ) {
        fprintf( stderr, "failed to open %s\n", file2open );
        exit( EXIT_FAILURE );
    }

        printf("Adding files to transfer definition\n");
    while( fgets( file_line, 255, filelist) ) {
        sfn = strtok( file_line, " \n\t" );
        if( !sfn )  continue;
        tfn = strtok( NULL, " \n\t" );

//        printf("source file: %s\n", sfn);
//        printf("target file: %s\n", tfn);

//        printf("Adding files to transfer definition\n");
        rc = BB_AddFiles(tdef, sfn, tfn, 0);
        check(rc);
    }

    fclose(filelist );

}  /* end of addTransferFiles func  **/


/*******Thread***/ 
void *runTransfer( void *arg)
{
 

    uint32_t rank = 0;
    Transfer_Data*  pTD = (Transfer_Data*)arg;
    BBTransferDef_t* tdef = pTD->tdef;
    BBTransferHandle_t thandle = 0;
    char*  filelist = pTD->filename;
    char   fvt_seed[256]="";
    pthread_t    child_tid;
    int rc = 0;


printf("DEBUG: INSIDE runTransfer Thread -> %d\n", (int)pthread_self());
    printf("Obtaining transfer handle\n");
//    rc = BB_GetTransferHandle(getpid(), 1, &rank, &pTD->thandle); /* \todo tag generation uses getpid() - need something better */
    rc = BB_GetTransferHandle(pthread_self(), 1, &rank, &pTD->thandle);  
    check(rc);
    thandle=pTD->thandle;

    sprintf(fvt_seed,"%s_%u_",FVT_SEED,(int)pthread_self());  // Unique prefix name for files

    createRandomFiles( fvt_seed, filelist );

    printf("Creating transfer definition\n");
    rc = BB_CreateTransferDef(&tdef);
    check(rc);

       /***   adding files to Transfer Definition  */
    addTransferFiles( tdef, filelist);

   //   BEFORE starting transfer start up the cancel transfer thread 
    if (pthread_create(&child_tid, NULL, cancelTransfer, (void *) pTD))
    {
         printf("ERROR: pthread_create: runTransfer Thread\n");
         return NULL;
    }

       /***   Starting  Transfer  */
    printf("Starting transfer\n");
    rc = BB_StartTransfer(tdef, thandle);
    check(rc);
    printf("BB_StartTransfer SUCCESS  ...  Waiting on child thread to end.... \n");

    /**********  
       May want to do clean up on random files here

    **************/


    pthread_join(child_tid, NULL);                        // wait until child thread returns

       /****Clean up random files **/
    if (cleanUpRandomFiles(filelist)) {
        printf("ERROR: cleanUpRandomFiles function\n");
    } /* end of cleanUpRandomFiles(..) func**/


    free(pTD);
    printf("RunTransfer Thread ended... %llu\n", (long long int)pthread_self());

    return NULL;
} /***  end of runTransfer func  */
    
/*******Thread***/ 
void *cancelTransfer( void *arg )
{
    Transfer_Data*  pTD = (Transfer_Data*)arg;
//    BBTransferDef_t* tdef = pTD->tdef;
    BBTransferHandle_t thandle = pTD->thandle;
    char*  filelist = pTD->filename;
    int rc = 0;


        /***  Wait and give time for calling thread to Start Transfer   **/ 
 printf("DEBUG: INSIDE CANCEL Transfer Thread...%u.  Sleeping for 15 seconds\n", (int)pthread_self());
   // sleep(15);                          
    sleep(5);                          

       /***   Attempting to Cancel  Transfer on calling thread  */
    printf("Attempting to Cancel Transfer for handle->%lu for filelist: %s\n", thandle, filelist );
    rc = BB_CancelTransfer(thandle, BBSCOPETRANSFER);
    check(rc);
    printf("SUCCESSFULLY Canceled Transfer for handle->%lu for filelist: %s\n", thandle, filelist );
    printf("CancelTransfer Thread ended...%d\n", (int)pthread_self());
 
    return NULL;

} /**  end of cancelTransfer func   */
    
int  parseArgs( int argc, char * argv[] )
{

   int option = 0;

   while ((option = getopt(argc, argv,"ch:l:p:t:f:")) != -1) {
        switch (option) {
             case 'c' : CancelTransfer = 1;                                 // JUST DO CANCEL of Transfer
                 break;
             case 'l' : strcpy(BB_LV, optarg);
                 break;
             case 'p' : strcpy(TARGET_LOC, optarg);
                 break;
             case 't' : ThreadSz = atoi(optarg); 
                 break;
             case 'f' : SzTransFiles = atoi(optarg);
                 break;
 //            case 'h' : THandle = strtoul(optarg, NULL, 16);
             case 'h' : THandle = a64l(optarg);
                 break;
             default: 
                 printf("Usage: %s -l <LOGICAL VOLUME> -p <TARGET LOCATION> -t <NUMBER OF THREADS> -f <Size of each Transaction File>\n", argv[0]);
                 printf("Smaller the -f value the more files generated.  2000 generate exactly 2000 files\n");
                 printf("Usage: %s -c  -h <HANDLE>  'To only cancel transfer on handle' \n", argv[0]);
                 return -1;
        }
    }
    if (strlen(BB_LV) == 0 || 
             ThreadSz <= 0 ||
             SzTransFiles <= 0 ) {

        printf("Usage: %s -l <LOGICAL VOLUME> -p <TARGET LOCATION> -t <NUMBER OF THREADSS> -f <Size of each Transaction File>\n", argv[0]);
        printf("Smaller the -f value the more files generated.  2000 generate exactly 2000 files\n");
        printf("Usage: %s -c  -h <HANDLE>  'To only cancel transfer on handle' \n", argv[0]);
        return -1;
    }

    return 0;


} /****** end of parseArgs func **********/


int  cleanUpRandomFiles( char* file2open)
{
    FILE *filelist;
    char file_line[256];
    char *sfn, *tfn;
    int rc = 0;



//    printf( "opening %s\n", file2open );
    if( !(filelist = fopen(file2open,"r")) ) {
        fprintf( stderr, "failed to open %s\n", file2open );
        return -1;
    }

    printf( "Cleaning up random files from filelist %s\n", file2open);

    while( fgets( file_line, 255, filelist) ) {

        char junk[2000]="";   // strictly used to get passed compiler warning -Werror=unused-variable
        sfn = strtok( file_line, " \n\t" );
        if ( !sfn ) continue;
        tfn = strtok( NULL, " \n\t" );

        sprintf(junk, "%s", tfn); 
       // printf("source file: %s\n", sfn);
       // printf("target file: %s\n", tfn);
        if( access( sfn, F_OK ) == 0 ) {   
           rc = remove(sfn);      //  delete file
        }  /*   end of if stmt */
    }  /**end of if while loop*/

    fclose(filelist );
 
      /** As a last step, remove filelist***/
    rc = remove(file2open);      //  delete file

    return rc;

     
} /***** End of cleanUpRandomFiles func ****/



