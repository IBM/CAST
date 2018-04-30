/*******************************************************************************
 |    dynplugin.cc
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


#include <stdio.h>
#include <map>
#include <vector>
#include <dlfcn.h>
#include "flightlog.h"

using namespace std;

map<void*,string> pluginmap;
map<FlightRecorderRegistry_t*, map<uint32_t, FlightRecorderFunc_t> > decodermap;
vector<string> searchPath = { "." };

extern "C"
{
    int processPlugin(size_t bufferSize, char* buffer, FlightRecorderRegistry_t* reg, FlightRecorderLog_t* log);
    int loadPlugin(FlightRecorderRegistry_t* reg);
    int unloadAllPlugins();
};

int FL_AddDecoderLibPath(const char* searchpath)
{
    searchPath.push_back(searchpath);
    return 0;
}

int loadPlugin(FlightRecorderRegistry_t* reg)
{
    int rc;
    int (*openDecoder)(int*, FlightDecoderDesc_t**);
    
    if(reg->decoderName[0] == 0)
	return 0;
    
    printf("Loading flightlog plugin '%s' for registry '%s'\n", reg->decoderName, reg->registryName);
    
    void* handle = NULL;
    for(auto path : searchPath)
    {
	string decoderpath = path + "/" + reg->decoderName;
	handle = dlopen(decoderpath.c_str(), RTLD_LAZY);
	if(handle)
	{
	    printf("\t\t Found at %s\n", decoderpath.c_str());
	    break;
	}
	dlerror();
    }
    if(handle == NULL)
    {
	printf("Unable to open shared library '%s'.  Skipping\n", reg->decoderName);
	return 0;
    }
    
    dlerror();
    pluginmap[handle] = reg->decoderName;
    
    *(void **) (&openDecoder) = dlsym(handle, "FL_openDecoder");
    
    if(openDecoder == NULL)
    {
	printf("Unable to resolve FL_openDecoder symbol.  Not decoder plugin\n");
	return 0;
    }
    int count;
    FlightDecoderDesc_t* desc;
    
    rc = (*openDecoder)(&count, &desc);
    if(rc)
    {
	printf("Open decoder failed?!?\n");
	return -1;
    }
    int x;
    for(x=0; x<count; x++)
    {
	FlightRecorderFunc_t func;
	func = (FlightRecorderFunc_t) dlsym(handle, desc[x].functionName);
	if(func == NULL)
	{
	    string cppfuncname = string("_Z15") + desc[x].functionName + string("mPcPK17FlightRecorderLog");
	    func = (FlightRecorderFunc_t) dlsym(handle, cppfuncname.c_str());
	    if(func == NULL)
	    {
		printf("Unable to find symbol '%s' or C++ variant '%s'.  Skipping.\n", desc[x].functionName, cppfuncname.c_str());
	    }
	}
	
	if(func)
	{
	    decodermap[reg][desc[x].id] = func;
	}
    }
    return 0;
}

int unloadAllPlugins()
{
    for(auto plugin : pluginmap)
    {
	dlclose(plugin.first);
    }
    return 0;
}

int processPlugin(size_t bufferSize, char* buffer, FlightRecorderRegistry_t* reg, FlightRecorderLog_t* log)
{
    FlightRecorderFunc_t func;
    if(decodermap.find(reg) != decodermap.end())
    {
	if(decodermap[reg].find(log->id) != decodermap[reg].end())
	{
	    func = decodermap[reg][log->id];
	    (*func)(bufferSize, buffer, log);
	    return 0;
	}
    }
    return -1;
}
