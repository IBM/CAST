/*******************************************************************************
 |    logging.h
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


#ifndef LOGGING_H_
#define LOGGING_H_

#define BOOST_LOG_DYN_LINK 1

#include <iomanip>
#include <iostream>
#include <string>

#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/utility/manipulators.hpp>
#include <boost/property_tree/ptree.hpp>

#include <string>
#include <chrono>


/**
   \brief Helper macros for __FILENAME__ "filename.cc" and __FILELINE__ "filename.cc:122"
   
   \note __FILENAME__ is being set in the toplevel CMakeLists.txt 

   \example
     LOG(error) << __FILENAME__ << ": This is my error message";
     Output: ... | filename.cc: This is my error message 
     
     LOG(error) << __FILELINE__ << ": This is my error message";
     Output: ... | filename.cc:122: This is my error message 
*/
#ifdef __FILENAME__
#define LINE1(x) #x
#define LINE2(x) LINE1(x)
#define __FILELINE__ __FILENAME__ ":" LINE2(__LINE__)
#endif

namespace utility
{
    class bluecoral_filename;

    enum bluecoral_sevs
    {
#define SEVERITY(n) n,
#include "severity.h"
#undef SEVERITY
	NUM_SEVERITIES
    };
    enum bluecoral_subcomponents
    {
#define SUBCOMPONENT(n) n,
#include "subcomponent.h"
#undef SUBCOMPONENT
	NUM_SUBCOMPONENTS
    };
    extern utility::bluecoral_sevs minlevel[NUM_SUBCOMPONENTS];

    extern boost::log::sources::severity_channel_logger_mt< bluecoral_sevs > logger;
    extern boost::log::sources::channel_logger_mt< > bds_logger;
    extern boost::log::sources::channel_logger_mt< > allocation_logger;

    /**
     * @brief Retrieves the current time then converts it to a string.
     * @return The UTC timestamp.
     */
    std::string GetUTCTimestampNow();
};


/**
   \brief Log a message at the given severity level

   \note Severities are defined in "utility/include/severity.h".  Only include logging.h, however.
   \example
     LOG(error) << "This is my error message";

*/


#ifndef USE_SC_LOGGER
#define LOG(sevlevel) BOOST_LOG_SEV(utility::logger, utility::bluecoral_sevs::sevlevel)
#else
#define LOG(subcomponent, sevlevel) \
    if(__glibc_unlikely(utility::minlevel[utility::bluecoral_subcomponents::subcomponent] <= utility::bluecoral_sevs::sevlevel)) \
        BOOST_LOG_SEV(utility::logger, utility::bluecoral_sevs::sevlevel) \
                               << boost::log::add_value("SubComponent", utility::bluecoral_subcomponents::subcomponent) \
                               << boost::log::add_value("FileName"    , (utility::bluecoral_filename*)__FILE__) \
                               << boost::log::add_value("Line"        , __LINE__) \
                               << boost::log::add_value("Function"    , __func__)

#define TIMESTAMP_NOW() ",\"timestamp\":\"" << utility::GetUTCTimestampNow() << "\""

#define TRANSACTION(type, traceid, uid, data) BOOST_LOG(utility::bds_logger) << "{\"type\":\"" << type << \
    "\",\"traceid\":" << traceid << ",\"uid\":" << uid << TIMESTAMP_NOW() <<",\"data\":" << data << "}"

#define ALLOCATION(type, source, data) BOOST_LOG(utility::allocation_logger) << "{\"type\":\"" << type << \
"\""  TIMESTAMP_NOW() <<",\"data\":" << data << ",\"source\":\"" << source << "\"}"

#define setLoggingLevel(subcomponent, setlevel) \
    utility::minlevel[utility::bluecoral_subcomponents::subcomponent] = utility::bluecoral_sevs::setlevel
#endif

/**
   \brief Initializing the logging framework

   initializeLogging() configures the Boost logging facility.

   It creates two log sinks (a sink is an output from the logging facility):
      - console output
      - file output

   The LOG(sevlevel) macro uses a thread-safe log source to post messages to the logging.

   \todo The file sink is setup to rotate logs >10MB at midnight.  This should be configurable.
   \note Parameters to this function may be subject to change to be more configurable.

   \param[in] ptree_prefix ptree prefix
   \param[in] config configuration
   \return Error code
   \retval 0 Success
 */
int initializeLogging(std::string ptree_prefix, boost::property_tree::ptree& config);

#endif /* LOGGING_H_ */
