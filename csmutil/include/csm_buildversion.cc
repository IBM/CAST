/*******************************************************************************
|    csm_buildversion.cc
|
|  © Copyright IBM Corporation 2015,2016. All Rights Reserved
|
|    This program is licensed under the terms of the Eclipse Public License
|    v1.0 as published by the Eclipse Foundation and available at
|    http://www.eclipse.org/legal/epl-v10.html
|
|    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
|    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/
#include <cstdio>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char** argv)
{
    FILE* fd;
    char *version;    
    char *commit;
    char *date;

    if(argc > 2)
    {
        version = argv[2];
    }
    else version = strdup("unknown");
    
    if( argc > 3 )
    {
      commit = argv[3];
    }
    else commit = strdup("unknown");

    if( argc > 4 )
    {
      date = argv[4];
    }
    else date = strdup("no date");

    std::string fn = argv[1];
    fn += ".tmp";
    
    fd = fopen(fn.c_str(), "w");
    if(fd == NULL)
    {
        printf("Unable to create file '%s'  errno=%s\n", argv[1], strerror(errno));
        exit(-1);
    }
    fprintf(fd, "/*******************************************************************************\n");
    fprintf(fd, "|    %s\n", argv[1]);
    fprintf(fd, "|  © Copyright IBM Corporation 2015,2016. All Rights Reserved\n");
    fprintf(fd, "|\n");
    fprintf(fd, "|    This program is licensed under the terms of the Eclipse Public License\n");
    fprintf(fd, "|    v1.0 as published by the Eclipse Foundation and available at\n");
    fprintf(fd, "|    http://www.eclipse.org/legal/epl-v10.html\n");
    fprintf(fd, "|\n");
    fprintf(fd, "|    U.S. Government Users Restricted Rights:  Use, duplication or disclosure\n");
    fprintf(fd, "|    restricted by GSA ADP Schedule Contract with IBM Corp.\n");
    fprintf(fd, "*******************************************************************************/\n\n");
    fprintf(fd, "#define CSM_VERSION \"%s\"\n\n", version);
    fprintf(fd, "#define CSM_COMMIT \"%s\"\n\n", commit);
    fprintf(fd, "#define CSM_DATE \"%s\"\n\n", date);
    fclose(fd);
    
    std::string rsynccmd = std::string("rsync -c ") + fn + " " + std::string(argv[1]);
    system(rsynccmd.c_str());
    unlink(fn.c_str());
    return 0;
}
