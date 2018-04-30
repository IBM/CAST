/*================================================================================

    csmd/src/ras/src/RasEventChangeSpecList.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>


#include "../include/RasEventChangeSpecList.h"

using namespace std;


/**
 * Read in the event fiilter from a json file and populate the 
 * rasEventFilterMap struct 
 * 
 * @param env -- environment to rad event filter for. 
 * @param fname 
 * 
 * @return int 
 */
int RasEventChangeSpecList::readEventFilter(const std::string &env, const std::string &fname) 
{
    using namespace boost::property_tree;
    int  rc = 0;
    try {
        ptree pt;
        // read an array into pt then parse that and move it to the others...
        json_parser::read_json(fname, pt);        

        for(ptree::iterator iter = pt.begin(); iter != pt.end(); iter++)
        {   
            //json_parser::write_json(cout, iter->second);
            string idstr;
            ptree &spt = iter->second;
            //idstr = spt.get<string>("msgid");
            //cout << "idstr = " << idstr << endl;
            string envstr =  spt.get<string>("RasEnvironment");

            if (env != envstr)
                continue;   // skip if not the matching environment...

            // get the event change spec reccord as a child...
            ptree &cpt = spt.get_child("RasEventChangeSpec");

            //json_parser::write_json(cout, cpt);
            for(ptree::iterator iter = cpt.begin(); iter != cpt.end(); iter++)
            {   
                //json_parser::write_json(cout, iter->second);
                string msgId = iter->second.get<string>("msgId");
                string key = iter->second.get<string>("key");
                string value = iter->second.get<string>("value");

                addEventSpec(msgId, key, value);
            }

        }
    }
    catch(const ptree_error &e)
    {
        cout << "json file format error: " << fname << endl;
        cout << e.what() << endl;
        rc = -1;
    }
    return(rc); 
};

void RasEventChangeSpecList::addEventSpec(const std::string &msgId,
                                 const std::string &key,
                                 const std::string &value)
{
    _specs.push_back(RasEventChangeSpec(msgId, key,value));


    return;
}

