/*******************************************************************************
 |    stage-admin-wrapper.c
 |
 |    Copyright IBM Corporation 2017,2017. All Rights Reserved
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
#include <sys/types.h>

int main(int argc, char** argv)
{
    printf("Executing '%s'  (uid=%d  euid=%d)\n", SCRIPTPATH, getuid(), geteuid());
    execvp(SCRIPTPATH, argv);
    return 0;
}
