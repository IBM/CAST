/*******************************************************************************
 |    threadstate.h
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

#ifndef UTIL_ThreadState_H_
#define UTIL_ThreadState_H_

#include <string>
#include <list>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>


/*******************************************************************************
 | Local helper methods
 *******************************************************************************/
inline bool tserror_flat_compare(const std::pair<std::string,std::string>& first, const std::pair<std::string,std::string>& second)
{
    return (first.first.compare(second.first) < 0);
}


/*******************************************************************************
 | Macro definitions
 *******************************************************************************/
#define bailout ExceptionBailout()
#define err(a,b) std::make_pair(a,b)
#define errloc(rc) err("rc", rc) << err("error.func", __func__) << err("error.line", __LINE__) << err("error.sourcefile", __FILE__)


/*******************************************************************************
 | Classes
 *******************************************************************************/
class ExceptionBailout
{
};


class RAS
{
  private:
    std::string  _msgid;
  public:
    RAS(std::string msgid) { _msgid = msgid; };
    std::string  getmsgid() const { return _msgid; };
};


class TSHandler
{
    public:
        // Constructor
        TSHandler()
        {
            init();
        };

        // Virtual destructor
        virtual ~TSHandler() {};

        // Instance methods (public)
        int add(const char* key, int value)
        {
            if((strncmp(key, "error.",6) != 0) || (errstate.find(key) == errstate.not_found()))
            {
                errstate.put(key, value);
            }
            return 0;
        };

        int add(const char* key, unsigned int value)
        {
            if((strncmp(key, "error.",6) != 0) || (errstate.find(key) == errstate.not_found()))
            {
                errstate.put(key, value);
            }
            return 0;
        };

        int add(const char* key, long value)
        {
            if((strncmp(key, "error.",6) != 0) || (errstate.find(key) == errstate.not_found()))
            {
                errstate.put(key, value);
            }
            return 0;
        };

        int add(const char* key, unsigned long value)
        {
            if((strncmp(key, "error.",6) != 0) || (errstate.find(key) == errstate.not_found()))
            {
                errstate.put(key, value);
            }
            return 0;
        };

        int add(const char* key, double value)
        {
            if((strncmp(key, "error.",6) != 0) || (errstate.find(key) == errstate.not_found()))
            {
                errstate.put(key, value);
            }
            return 0;
        };

        int add(const char* key, std::string value)
        {
            if((strncmp(key, "error.",6) != 0) || (errstate.find(key) == errstate.not_found()))
            {
                errstate.put(key, value);
            }
            return 0;
        };

        void clear()
        {
            errstate.clear();
            add("rc", 0);

            return;
        }

        void clearErrorData()
        {
            errstate.erase("error");
        }

        void get(boost::property_tree::ptree& pt) const
        {
            pt = errstate;
        };

        void get(std::list<std::pair<std::string,std::string> >& keyvalues) const
        {
            flatten(errstate, "", keyvalues);
        };

        int getRC() const
        {
            return rc();
        };

        std::string get(std::string format = "json") const
        {
            std::ostringstream result_stream;
            if(format == "xml")
            {
                boost::property_tree::write_xml(result_stream, errstate);
            }
            else if((format == "flat") || (format == "pretty"))
            {
                std::list<std::pair<std::string,std::string> > keyvalues;
                size_t maxlen = 0;
                flatten(errstate, "", keyvalues);

                for(const auto& it : keyvalues)
                {
                    if(it.first.size() > maxlen)
                    {
                        maxlen = it.first.size();
                    }
                }
                keyvalues.sort(tserror_flat_compare);
                for(auto& it: keyvalues)
                {
                    it.first.resize(maxlen + 1, ' ');
                    result_stream << it.first << it.second << std::endl;
                }
            }
            else
            {
                boost::property_tree::write_json(result_stream, errstate, false);
            }
            std::string r = result_stream.str();
            while(r.back() == '\n')
                r.pop_back();
            return r;
        };

        // NOTE:  If the path already exists in errstate, this merge
        //        routine will replace the value at that path with
        //        the value from jsonstring.
        int merge(const std::string jsonstring)
        {
            boost::property_tree::ptree l_Tree;
            std::istringstream result_stream(jsonstring);
            boost::property_tree::read_json(result_stream, l_Tree);
            BOOST_FOREACH(auto& e, l_Tree)
            {
                errstate.boost::property_tree::ptree::put_child(e.first, e.second);
            }

            return 0;
        };



    protected:
        // Instance methods (protected)
        int flatten(const boost::property_tree::ptree& pt, std::string key, std::list<std::pair<std::string,std::string> > &keyvalues) const
        {
            std::string nkey;
            if (pt.empty())
            {
                keyvalues.push_back(std::make_pair(key, pt.data()));
            }
            else
            {
                if(key.empty() == false)
                {
                    key += ".";
                }
                for(auto it : pt)
                {
                    flatten(it.second, key + it.first, keyvalues);
                }
            }
            return 0;
        };

        void init()
        {
            gethostname(hostname, sizeof(hostname));
            char* l_Char = &hostname[0];
            while(*l_Char !='\0')
            {
                if (*l_Char == '.')
                    *l_Char = '_';
                l_Char++;
            }

            clear();

            return;
        };

        int rc() const
        {
            return errstate.get("rc", -1);
        };

        int set(const std::string jsonstring)
        {
            std::istringstream result_stream(jsonstring);
            boost::property_tree::read_json(result_stream, errstate);
            return 0;
        };

        // Instance data
        char hostname[64];
        boost::property_tree::ptree errstate;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::pair<const char*,int>& lhs)
{
    tsthis.add(lhs.first, lhs.second);
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::pair<const char*,unsigned int>& lhs)
{
    tsthis.add(lhs.first, lhs.second);
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::pair<const char*,long>& lhs)
{
    tsthis.add(lhs.first, lhs.second);
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::pair<const char*,unsigned long>& lhs)
{
    tsthis.add(lhs.first, lhs.second);
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::pair<const char*,double>& lhs)
{
    tsthis.add(lhs.first, lhs.second);
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::pair<const char*,std::string>& lhs)
{
    tsthis.add(lhs.first, lhs.second);
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const std::exception& e)
{
    tsthis.add("error.text", e.what());
    return tsthis;
};

inline TSHandler& operator<<(TSHandler& tsthis, const ExceptionBailout& e)
{
    throw e;
    return tsthis;
};


/*******************************************************************************
 | External methods
 *******************************************************************************/
extern TSHandler& operator<<(TSHandler& tsthis, const RAS& ras);

#endif /* UTIL_ThreadState_H_ */
