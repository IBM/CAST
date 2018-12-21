/*******************************************************************************
 |    earlyInit.c
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

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <bbapi.h>
#include <errno.h>

int expectedRC = -1;

int fclose(FILE* ptr)
{
    static int callinginit = false;

    if(callinginit == false)
    {
        int rc;
        printf("Calling BB_InitLibrary.  We should survive this with bad return code.\n");
        callinginit = true;
        rc = BB_InitLibrary(0, BBAPI_CLIENTVERSIONSTR);
        printf("rc=%d\n", rc);
        assert(rc == expectedRC);
        callinginit = false;
        
        callinginit = true;
        printf("Calling BB_TerminateLibrary\n");
        rc = BB_TerminateLibrary();
        printf("rc=%d\n", rc);
        assert(rc == expectedRC);
        callinginit = false;
    }
    else
    {
        printf("I/O occurred inside bbapi call.  This is expected\n");
    }
    return 0;
}

int main(int argc, char **argv)
{
    expectedRC = 0;   // started main()
    
    printf("Performing fopen() operation\n");
    FILE *ptr = fopen("/dev/zero", "r");
    if(ptr == NULL)
    {
        printf("Open of /dev/zero failed.  Failing testcase.  errno=%d", errno);
        return -1;
    }
    printf("Performing fclose().  This is intecepted\n");
    fclose(ptr);
    printf("Test passed.  Exiting\n");
    return 0;
}
