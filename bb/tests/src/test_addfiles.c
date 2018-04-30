/*******************************************************************************
 |    test_addfiles.c
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

//#include "bb/include/bbapi_types.h"
#include "bb/include/bbapi.h"



/****************
****GLOBALS
*****************/

uint32_t  Rank = 0;                                            // BB_InitLibrary rank var - DEFAULT
char   BB_InitString[1000] = BBAPI_CLIENTVERSIONSTR;           // BB_InitLibrary String variable - Default
int    TEST_BBFILEFLAG  = 0;                                  //  BBFILEFLAGS value
BBTAG  TEST_BBTAG  = 0;                                      //  BBTAG  value
int    NoTDef      =  0;                                    //  Boolean TRUE = Do Not create BBTransferDef_t
int    DupHdl      =  0;                                    //  Boolean TRUE = test for duplicate transfer handle 
int    ContribSz      =  0;                                 //  Contribution List size 
char   FileList[1000] = "";                                 // File with list of Tgt and Src files
char   SrcFile[1000] = "";                                  // Source File Name
char   SrcFile2[1000] = "";                                 // Source File Name  
char   TgtFile[1000] = "";                                  // Target File Name
char   JSONDumpFile[1000]  =  "";                           // File where JSON Error will be written




   //Command line options
static struct option long_options[] = {
        {"bbFFlag",         required_argument,     0,  'b' },
        {"contribSz",       required_argument,     0,  'c' },
        {"filelist",        required_argument,     0,  'f' },
        {"getTransferHdl",  required_argument,     0,  'g' },
        {"getDupTransferHdl",  required_argument,  0,  'd' },
        {"dumpJSON",        required_argument,     0,  'j' },
        {"noTransferDef",   no_argument,           0,  'n' },
        {"srcFile",         required_argument,     0,  's' },		
        {"targetFile",      required_argument,     0,  't' },		
        {"srcFile2",        required_argument,     0,  'u' },		
        {0,                     0,                 0,  0 }
 };



int  parseArgs( int, char**  );
int check(int);
void dumpJSON(char*, size_t );
void getLastErrorDetals(BBERRORFORMAT, char** , size_t*);
void addTransferFiles( BBTransferDef_t* tdef, char *filelist);
void dumpJSON2File(char* data, size_t size);



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
        printf("Error details:  %s\n", errstring );
		
        dumpJSON2File(errstring, errSz);


        free(errstring);

   //     printf("Aborting due to failures\n");
        exit(-1);
    }
    return 0;
} /**********End of check() func***************/



/***********************
*******PROGRAM ENTRY
***********************/
int main( int argc, char *argv[] ) {
    
    int rc;
    BBTransferDef_t* tdef = 0;
    BBTransferHandle_t thandle = 0;
    BBTransferHandle_t dup_thandle = 0;

    uint32_t *contrib_list = 0;
    uint32_t i;

    char *successString=0;
    size_t successStringSz=0;
  
    if (parseArgs( argc, argv ) < 0)
    {
        exit(-1);
    }
  

    rc = BB_InitLibrary(Rank, BB_InitString);
    //check(rc); // This should work
    if (rc) return 2;

  //    printf("Value of BBTransferOrderMask is [0x%04X]\n", BBTransferOrderMask );
  //    printf("Value of TEST_BBFILEFLAG is [0x%04X]\n", TEST_BBFILEFLAG );	
  

    if (!NoTDef) {
  //	   printf("Creating transfer definition\n");
       rc = BB_CreateTransferDef(&tdef);
       if (rc) return 3;
    }

     /***********************
        TESTING AREA 
       *********************/
    if (ContribSz)  {
             /*************************************
                 TEST AREA:  Process  BB_GetTransferHandle  API  
              ****************************/ 
       if( !(contrib_list = (uint32_t*)malloc(ContribSz*sizeof(uint32_t))) ) {
           fprintf( stderr, "Malloc Failed\n" );
           return 4;
        
       }
       for( i = 0; i < ContribSz; i++ )  contrib_list[i] = i;  
    
       printf("Obtaining transfer handle\n");
       rc = BB_GetTransferHandle( TEST_BBTAG, ContribSz, contrib_list, &thandle ); 
       free(contrib_list);
       check(rc);
        
       if (DupHdl) {
               /**** Obtaining transfer handle second time  *****/

           if( !(contrib_list = (uint32_t*)malloc(ContribSz*sizeof(uint32_t))) ) {
               fprintf( stderr, "Malloc Failed\n" );
        
           }
           for( i = 0; i < ContribSz; i++ )  contrib_list[i] = i;  
           rc = BB_GetTransferHandle( TEST_BBTAG, ContribSz, contrib_list, &dup_thandle ); 
           free(contrib_list);
           if (rc) return 6;
           if( thandle != dup_thandle ) {
               printf( "ERROR: expecting handles to be same\n\t%lu != %lu\n", 
                     thandle, dup_thandle );
               return 7;
           }
    
       }  //..end of if stme DupHdl

    } else {
             /*************************************
                 TEST AREA:  Process BB_AddFiles API 
              ****************************/ 
       if (strlen(FileList) > 0) {
          addTransferFiles( tdef, FileList);
       }  //..end of Filelist if stmt
    
       
       struct stat l_stat;
       stat(TgtFile, &l_stat);
       
          // Used for individual Source File transfers	
       rc = BB_AddFiles(tdef, SrcFile, TgtFile, TEST_BBFILEFLAG);
        if (rc) return 8;

       if (strlen(SrcFile2) > 0) {
          //   THIS SHOULD NOT WORK 
          rc = BB_AddFiles(tdef, SrcFile2, TgtFile, TEST_BBFILEFLAG);
           if (rc) {
               printf("rc = BB_AddFiles(tdef, SrcFile2, TgtFile, TEST_BBFILEFLAG) failed like it should");
               check(rc);
           }
           if(( rc == 0 ) && (!S_ISDIR(l_stat.st_mode)))
           {
             printf("Duplicate AddFiles Testcase FAILURE:  SHOULD NOT RETURN SUCCESS\n");
             printf( "Aborting Test\n");
             exit(-1);
        }
       } //..end of if (strlen  )


   } //..end of if stmt ContribSz

    printf("Test SUCCESS:  BB_AddFiles\n" );	
  //    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    //check(rc);
   
     // Dump Successfull JSON 2 File 
    dumpJSON2File(successString, successStringSz);
    free(successString);

    return 0;

}   /******End of Program & main func*******/


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
//        rc = BB_AddFiles(tdef, sfn, tfn, 0);
        rc = BB_AddFiles(tdef, sfn, tfn, TEST_BBFILEFLAG);
        check(rc);
    }

    fclose(filelist );

}  /* end of addTransferFiles func  **/

    
int  parseArgs( int argc, char * argv[] )
{

   int option = 0;
   int long_index = 0;

  
   
   while ((option = getopt_long(argc, argv,"b:c:d:f:g:j:ns:t:u:", long_options, &long_index)) != -1) {
        switch (option) {
//             case 'b' : TEST_BBFILEFLAG = atoi(optarg);
             case 'b' : TEST_BBFILEFLAG = strtol(optarg, NULL, 0);
                 break;
             case 'c' : ContribSz = atoi(optarg);
                 break;
             case 'f' : strcpy(FileList, optarg);
                 break;
             case 'g' : TEST_BBTAG = strtol(optarg, NULL, 0);
                 break;
             case 'd' : TEST_BBTAG = strtol(optarg, NULL, 0);
                 DupHdl = 1; 
                 break;
             case 'j' : strcpy(JSONDumpFile,optarg);
                 break;
             case 'n' : NoTDef = 1; 
                 break;
             case 's' : strcpy(SrcFile, optarg);
                 break;
             case 't' : strcpy( TgtFile, optarg);
                 break;
             case 'u' : strcpy( SrcFile2, optarg);
                 break;
             default: 
                 printf("Usage: %s --bbFFlag \n", argv[0]);
                 printf("          --fileList \n");				 
                 printf("          --contribSz \n");				 
                 printf("          --getTransferHdl=<USER BBTAG> \n");				 
                 printf("          --noTransferDef \n");
                 printf("          --srcFile \n");
                 printf("          --srcFile2 \n");
                 printf("          --targetFile \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;
        } //..end of switch stmt
    } //..end of while loop
   
    if (!ContribSz && (strlen(SrcFile) == 0 || strlen(TgtFile) == 0) ) {
                 printf("Usage: %s --bbFFlag \n", argv[0]);
                 printf("          --fileList \n");				 
                 printf("          --contribSz \n");				 
                 printf("          --getTransferHdl=<USER BBTAG> \n");				 
                 printf("          --noTransferDef \n");
                 printf("          --srcFile \n");
                 printf("          --srcFile2 \n");
                 printf("          --targetFile \n");
                 printf("          --dumpJSON <FILENAME> \n");
                 return -1;

    } //..end of if stmt

    return 0;


} /****** end of parseArgs func **********/
