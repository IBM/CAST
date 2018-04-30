/*******************************************************************************
 |    get_version2.c
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


// ERROR: Attempt to get the verison with a size less than the minimum size for 
//  APIVersion
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

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
    
    // passing 4, which should be far too short
    errno = 0;
    rc = BB_GetVersion( 4, version );

    // check rc
    if( rc == 0 ) { // error did not happen
        fprintf( stderr, "%s\n",
"return from BB_GetVersion() should have indicated an error. 0 was returned." );

        fflush( stderr );
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    MPI_Barrier( MPI_COMM_WORLD );
    
    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
    
    
        
        
        
                 
        
        
    
