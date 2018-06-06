/*******************************************************************************
 |    logging.cc
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


#define BOOST_LOG_DYN_LINK 1

#include <string>
#include <map>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/support/date_time.hpp>

#include "logging.h"

using namespace std;
using namespace utility;
namespace logging  = boost::log;
namespace src      = boost::log::sources;
namespace keywords = boost::log::keywords;
namespace sinks    = boost::log::sinks;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)


static const char* severity_level_str[] =
{
#define SEVERITY(n) #n,
#include "severity.h"
#undef SEVERITY
};

static const char* subcomponent_str[] =
{
#define SUBCOMPONENT(n) #n,
#include "subcomponent.h"
#undef SUBCOMPONENT
};

static map<string, bluecoral_sevs> str2severity = {
#define SEVERITY(n) { #n, bluecoral_sevs::n },
#include "severity.h"
#undef SEVERITY
};

static size_t maxsubcomponentwidth = 1;
static size_t maxseveritywidth = 1;

namespace utility
{
    src::severity_logger_mt< bluecoral_sevs > logger;
    bluecoral_sevs minlevel[NUM_SUBCOMPONENTS];

    void touch_unused_variable()
    {
	cout << timestamp.get_name() << endl;
    }

    /**
       Convert the bluecoral_sevs into strings for posting into the log
       This must be in the same namespace as bluecoral_sevs, otherwise Boost::log won't find it.

    */
    template< typename CharT, typename TraitsT >
    basic_ostream< CharT, TraitsT >& operator<< (basic_ostream< CharT, TraitsT >& strm, bluecoral_sevs lvl)
    {
	if ((lvl < NUM_SEVERITIES) && (lvl >= 0))
	    strm << setw(maxseveritywidth) << left << severity_level_str[lvl];
	else
	    strm << setw(maxseveritywidth) << left << static_cast< int >(lvl);
	return strm;
    }

    template< typename CharT, typename TraitsT >
    basic_ostream< CharT, TraitsT >& operator<< (basic_ostream< CharT, TraitsT >& strm, bluecoral_subcomponents subcomponent)
    {
        if ((subcomponent < NUM_SUBCOMPONENTS) && (subcomponent >= 0))
            strm << setw(maxsubcomponentwidth) << right << subcomponent_str[subcomponent];
        else
            strm << setw(maxsubcomponentwidth) << right << static_cast< int >(subcomponent);
        return strm;
    }
    
    template< typename CharT, typename TraitsT >
    basic_ostream< CharT, TraitsT >& operator<< (basic_ostream< CharT, TraitsT >& strm, bluecoral_filename* bfn)
    {
	const char* filename = (const char*)bfn;
	if(strncmp(filename, __SOURCEROOT__, strlen(__SOURCEROOT__)) == 0)
	{
	    filename += strlen(__SOURCEROOT__)+1;
	}
	strm << filename;
        return strm;
    }
};

int initializeLogging(string ptree_prefix, boost::property_tree::ptree& config)
{
    static bool alreadyInitialized = false;
    if(alreadyInitialized)
	return 0;
    alreadyInitialized = true;

#define SUBCOMPONENT(n) maxsubcomponentwidth = max(strlen(#n), maxsubcomponentwidth);
#include "subcomponent.h"
#undef SUBCOMPONENT

#define SEVERITY(n) maxseveritywidth = max(strlen(#n), maxseveritywidth);
#include "severity.h"
#undef SEVERITY

    int x;
    string defaultsev = config.get(ptree_prefix + ".default_sev", "info");
    for(x=0; x<NUM_SUBCOMPONENTS; x++)
    {
	string setlvl = config.get(ptree_prefix + "." + subcomponent_str[x], defaultsev);
	if(str2severity.find(setlvl) != str2severity.end())
	{
	    minlevel[x] = str2severity[setlvl];
	}
	else
	{
	    cout << "Invalid severity: " << subcomponent_str[x] << " = " << setlvl << endl;
	}
    }

    logging::add_common_attributes();
    logging::register_simple_formatter_factory< bluecoral_sevs, char >("Severity");
    logging::register_simple_formatter_factory< bluecoral_subcomponents, char >("SubComponent");
    logging::register_simple_formatter_factory< bluecoral_filename*, char >("FileName");

/*
  The following are parameters that initializeLogging can take.
  e.g., ptree_pretix could point to "bb.server.log" which specifies the parameter subtree of the config.

  "log" :
  {
    "format"       :   "%TimeStamp% %SubComponent%::%Severity% | %Message%"  // format string for log
    "consoleLog"   :   true                                                  // enable console logging
    "fileLog"      :   "none"                                                // filename
    "rotationSize" :   -1                                                    // rotationsize (bytes)
    "default_sev"  :   "info"                                                // default minimum severity
    <subcomponent> :   <default_sev>                                         // minimum severity for a subcomponent
  }
 */
    if(config.get(ptree_prefix + ".consoleLog", true))
    {
	logging::add_console_log
	    (
		((config.get(ptree_prefix + ".consoleStream", "stdout") == "stdout")?(std::cout):(std::cerr)),
		keywords::format = config.get(ptree_prefix + ".format", "%TimeStamp% %SubComponent%::%Severity% | %Message%")
		);
    }

    if(config.get(ptree_prefix + ".fileLog", "none") != "none")
    {
        auto mode = std::ios::out;
        if(config.get(ptree_prefix + ".fileAppend", false) == true)
        {
            mode |= std::ios::app;
        }
	auto sink = logging::add_file_log
	    (
		keywords::file_name = config.get(ptree_prefix + ".fileLog", "none"),
		keywords::rotation_size = config.get(ptree_prefix + ".rotationSize", (~0)),
		keywords::auto_flush = true,
		keywords::format = config.get(ptree_prefix + ".format", "%TimeStamp% %SubComponent%::%Severity% | %Message%"),
                keywords::open_mode = mode
		);
        
        if(config.get(ptree_prefix + ".archiveLogs", "none") != "none")
        {
            sink->locked_backend()->set_file_collector(sinks::file::make_collector(
                                                           keywords::target = config.get(ptree_prefix + ".archiveLogs", "none"),
                                                           keywords::max_size = config.get(ptree_prefix + ".archiveSize", 1024 * 1024 * 1024),
                                                           keywords::min_free_space = config.get(ptree_prefix + ".archiveMinDiskSize", 1024 * 1024 * 1024)
                                                           ));
            
            sink->locked_backend()->scan_for_files(sinks::file::scan_all);
        }
    }
    
    if(config.get(ptree_prefix + ".sysLog", false))
    {
        boost::shared_ptr< logging::core > core = logging::core::get();
        boost::shared_ptr< logging::sinks::syslog_backend > backend(
            new logging::sinks::syslog_backend(
                keywords::facility = logging::sinks::syslog::local0,
                keywords::use_impl = logging::sinks::syslog::udp_socket_based
        ));
        
        backend->set_target_address(config.get(ptree_prefix + ".server", "127.0.0.1"), 
                                    config.get(ptree_prefix + ".port",   514));
        
        // Map severities into syslog levels:
        logging::sinks::syslog::custom_severity_mapping< bluecoral_sevs > mapping("Severity");
        mapping[off]      = logging::sinks::syslog::debug;
        mapping[trace]    = logging::sinks::syslog::debug;
        mapping[debug]    = logging::sinks::syslog::debug;
        mapping[info]     = logging::sinks::syslog::info;
        mapping[warning]  = logging::sinks::syslog::warning;
        mapping[error]    = logging::sinks::syslog::error;
        mapping[critical] = logging::sinks::syslog::critical;
        mapping[always]   = logging::sinks::syslog::info;
        backend->set_severity_mapper(mapping);
        
        auto sink = boost::make_shared< logging::sinks::synchronous_sink< logging::sinks::syslog_backend > >(backend);
        sink->set_formatter(
            boost::log::parse_formatter("CAST[-]:%SubComponent% %Message%")
        );
        core->add_sink(sink);
    }
    
    return 0;
}
