/*******************************************************************************
 |    get_version1.c
 |
 |  © Copyright IBM Corporation 2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/


// 1.	Call BB_GetVersion()
//  Confirm that the correct version is returned
//  Confirm that 0 is returned
#include <stdlib.h>
#include <stdio.h>

#include "bb/include/bbapi.h"
#include "mpi.h"

int check(int rc)
{
    if(rc)
    {
        char errstring[256];
        size_t numBytesAvail;
        size_t buf_len = 256;
        
        BB_GetLastErrorDetails(BBERRORJSON, &numBytesAvail, buf_len, errstring);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );
        
        printf("Aborting due to failures\n");
        exit(-1);
    }
    return 0;
}

int main( int argc, char *argv[] ) {
    int rank;
    int rc;
    char version[256];
    
    MPI_Init( &argc, &argv );
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    rc = BB_GetVersion( 256, version );
    
    printf( "Version is:\n%s\n", version );
    
    if( rc != 0 ) {
        fprintf( stderr, "RC from BB_GetVersion() is %d, but should be 0\n",rc);
        fflush( stderr );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
    
