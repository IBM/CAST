/*******************************************************************************
|    buildversion.cc
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


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

int main(int argc, char** argv)
{
    FILE* fd;
    pt::ptree version;
    pt::read_json(argv[1], version);
    
    if(argc > 3)
    {
        version.put("gitcommit", argv[3]);
        printf("added gitcommit to arg[3] in %s \n",argv[0]);
    }
    std::ostringstream result_stream;
    boost::property_tree::write_json(result_stream, version, false);
    std::string r = result_stream.str();
    while(r.back() == '\n')
        r.pop_back();
    replace( r.begin(), r.end(), '"', '^');
    
    std::string fn = argv[2];
    fn += ".tmp";
    
    fd = fopen(fn.c_str(), "w");
    if(fd == NULL)
    {
        printf("Unable to create file '%s'  errno=%s\n", argv[2], strerror(errno));
        exit(-1);
    }
    fprintf(fd, "/*******************************************************************************\n");
    fprintf(fd, "|    %s\n", argv[2]);
    fprintf(fd, "|  © Copyright IBM Corporation 2015,2016. All Rights Reserved\n");
    fprintf(fd, "|\n");
    fprintf(fd, "|    This program is licensed under the terms of the Eclipse Public License\n");
    fprintf(fd, "|    v1.0 as published by the Eclipse Foundation and available at\n");
    fprintf(fd, "|    http://www.eclipse.org/legal/epl-v10.html\n");
    fprintf(fd, "|\n");
    fprintf(fd, "|    U.S. Government Users Restricted Rights:  Use, duplication or disclosure\n");
    fprintf(fd, "|    restricted by GSA ADP Schedule Contract with IBM Corp.\n");
    fprintf(fd, "*******************************************************************************/\n\n");
    fprintf(fd, "#define BBAPI_CLIENTVERSIONSTR \"%s\"  ///< Client string for BB_InitLibrary\n", r.c_str());
    fclose(fd);
    
    std::string rsynccmd = std::string("rsync -c ") + fn + " " + std::string(argv[2]);
    system(rsynccmd.c_str());
    unlink(fn.c_str());
    return 0;
}
