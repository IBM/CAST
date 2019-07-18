/*================================================================================

    csmrestd/src/csmrestd.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


/*! \file
  \brief
  csmrestd

  Restful api for recording daemon for inserting RAS events into the CSM system
  for CORAL based systems.

  POST /csmi/V1.0/ras/event/create

  Create a Ras Event.

    POST data:
    {
      "msg_id": "test.testcat01.test01",
      "time_stamp": "timestamp",
      "location_name" => "fsgb001",
      "raw_data":"raw data"
      "kvcsv": "key_value_csv data"
    }

    msg_id -- message id string.
    time_stamp -- timestamp string.
    location_name -- location name string.
    raw_data -- raw data string.
    kvcsv -- comma separated keyvalue data..

  POST /csmi/V1.0/ras/event/query

  POST /csmi/V1.0/loopback/test


*/


#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "RestApiServer.h"
#include "csm_api_common.h"
#include "csm_api_ras.h"
#include "logging.h"
#include "csm_daemon_exception.h"

//
// todo, add signal handler that then kills off the server...
//

namespace pt = boost::property_tree;
namespace po = boost::program_options;

using std::string;
using std::endl;
using std::cerr;

using namespace std;
#define NAME       "csmrestd"

#ifndef CSM_OPT_HELP_LONG
#define CSM_OPT_HELP_LONG "help"
#define CSM_OPT_HELP_SHORT 'h'
#endif

#ifndef CSM_OPT_FILE_LONG
#define CSM_OPT_FILE_LONG "file"
#define CSM_OPT_FILE_SHORT "f"
#endif

// Supported configuration file keys
#define CONFIG_FILE_KEY_LISTENIP "csmrestd.listenip"
#define CONFIG_FILE_KEY_PORT "csmrestd.port"

class CsmRestApiServer : public RestApiServer
{
public:
  explicit CsmRestApiServer(const string& address, const string& port) :
      RestApiServer(address, port),
      _mytext("myText") {};
  int setCallbacks();
protected:
    RestApiReply::status_type csmRasEventCreate(string const &method,
                   string const &url,
                   string const &jsondata,
                   string &jsonOut);
    RestApiReply::status_type csmRasEventQuery(string const &method,
                   string const &url,
                   string const &jsondata,
                   string &jsonOut);
    RestApiReply::status_type csmLoopbackTest(string const &method,
                   string const &url,
                   string const &jsondata,
                   string &jsonOut);
private:
    string _mytext;
};

#define LOG_RAS_EVENT_PREFIX_WIDTH (21)

RestApiReply::status_type CsmRestApiServer::csmRasEventCreate(
   string const &method, string const &url, string const &jsonIn, string &jsonOut)
{
    RestApiReply::status_type rc = RestApiReply::ok;
    try
    {

        LOG( csmrestd, debug ) << "csmRasEventCreate: Start processing.";
        std::stringstream ss; 
        ss << jsonIn;
        //extract all fields...
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);

        // parse all the json fields...
        string msg_id;
        string time_stamp;
        string location_name;
        string raw_data;
        string kvcsv;
        string ctx;

        for(boost::property_tree::ptree::iterator iter = pt.begin(); iter != pt.end(); iter++)
        {
            if (iter->first == CSM_RAS_FKEY_MSG_ID) msg_id = iter->second.data();
            else if (iter->first == CSM_RAS_FKEY_TIME_STAMP) time_stamp = iter->second.data();
            else if (iter->first == CSM_RAS_FKEY_LOCATION_NAME) location_name = iter->second.data();
            else if (iter->first == CSM_RAS_FKEY_RAW_DATA) raw_data = iter->second.data();
            else if (iter->first == CSM_RAS_FKEY_KVCSV) kvcsv = iter->second.data();
            else if (iter->first == CSM_RAS_FKEY_CTXID) ctx = iter->second.data();
            else {
                // ignore other fields...  or maybe do some sor tof error..
            }
        }

        csm_api_object *csmobj = NULL;
        LOG( csmrestd, info ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << std::left << "NEW RAS EVENT" << std::setw(0)
            << " ctx:" << ctx
            << " ts:" << time_stamp
            << " loc:" << location_name
            << " msg:" << msg_id;
        int csmrc = csm_ras_event_create(&csmobj,
                                          msg_id.c_str(),
                                          time_stamp.c_str(),
                                          location_name.c_str(),
                                          raw_data.c_str(),
                                          kvcsv.c_str());
        if  (csmrc != 0) {
            char *errmsg = csm_api_object_errmsg_get(csmobj);
            jsonOut = string("{\"error\":\"") + "CSMRESTD csm_ras_event_create = " + errmsg + "\"}";
            LOG( csmrestd, error ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << std::left << "RAS EVENT ERROR" << std::setw(0)
                << " ctx:" << ctx
                << " ts:" << time_stamp
                << " loc:" << location_name
                << " msg:" << msg_id
                << " errstr:" << errmsg;
            rc = RestApiReply::internal_server_error;
            // put this into an error return too...
        }
        else
        {
          LOG( csmrestd, info ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << std::left << "RAS EVENT COMPLETE" << std::setw(0)
              << " ctx:" << ctx
              << " ts:" << time_stamp
              << " loc:" << location_name
              << " msg:" << msg_id;
        }
        csm_api_object_destroy(csmobj);

    }
    catch (std::exception & e)
    {
      LOG( csmrestd, warning ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << std::left << "RAS EVENT ERROR" << std::setw(0)
        <<  " error parsing json data: " << e.what();

        // need to return some sort of error here... internal_server_error and some extra text...?
        rc = RestApiReply::internal_server_error;
        jsonOut = string("{\"error\":\"") + "exception = " + e.what() + "\"}";
    }

    // no output...
    //    unless we had an error...

    return(rc);
}

RestApiReply::status_type CsmRestApiServer::csmRasEventQuery(
   string const &method, string const &url, string const &jsonIn, string &jsonOut)
{
    RestApiReply::status_type rc = RestApiReply::ok;

    try
    {

        LOG( csmrestd, debug ) << "csmRasEventQuery: Start processing.";
        std::stringstream ss;
        ss << jsonIn;
        //extract all fields...
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);

        string msg_id;
        string suppress_ids;
        string severity;
        string start_time_stamp;
        string end_time_stamp;
        string location_name;
        string control_action;
        string message;


        int limit = 0;
        int offset = 0;
        //return(_event.get<string>(key));
        //_event.count(key)

#if 0
        const char *msg_id,
        const char *suppress_ids,
        const char *severity,
        const char *start_time_stamp,
        const char *end_time_stamp,
        const char *location_name,
        const char *control_action,
        const char *message,
#endif
        if (pt.count(CSM_RAS_FKEY_MSG_ID)) msg_id=pt.get<string>(CSM_RAS_FKEY_MSG_ID);
        if (pt.count(CSM_RAS_FKEY_SUPPRESS_IDS)) suppress_ids=pt.get<string>(CSM_RAS_FKEY_SUPPRESS_IDS);
        if (pt.count(CSM_RAS_FKEY_SEVERITY)) severity=pt.get<string>(CSM_RAS_FKEY_SEVERITY);
        if (pt.count(CSM_RAS_FKEY_START_TIME_STAMP)) start_time_stamp=pt.get<string>(CSM_RAS_FKEY_START_TIME_STAMP);
        if (pt.count(CSM_RAS_FKEY_END_TIME_STAMP)) end_time_stamp=pt.get<string>(CSM_RAS_FKEY_END_TIME_STAMP);
        if (pt.count(CSM_RAS_FKEY_LOCATION_NAME)) location_name=pt.get<string>(CSM_RAS_FKEY_LOCATION_NAME);
        if (pt.count(CSM_RAS_FKEY_CONTROL_ACTION)) control_action=pt.get<string>(CSM_RAS_FKEY_CONTROL_ACTION);
        if (pt.count(CSM_RAS_FKEY_MESSAGE)) message=pt.get<string>(CSM_RAS_FKEY_MESSAGE);
        if (pt.count(CSM_RAS_FKEY_LIMIT)) limit=pt.get<int>(CSM_RAS_FKEY_LIMIT);
        if (pt.count(CSM_RAS_FKEY_OFFSET)) offset=pt.get<int>(CSM_RAS_FKEY_OFFSET);

        /*Set up data to call API*/
        csm_ras_event_query_input_t* input = NULL;
        /* CSM API initialize and malloc function*/
        csm_init_struct_ptr(csm_ras_event_query_input_t, input);
        csm_ras_event_query_output_t* output = NULL;

        input->msg_id = strdup(msg_id.c_str());
        //input->severity = strdup(severity.c_str());
        //get enum value of the severity.
        int temp_severity = csm_get_enum_from_string(csmi_ras_severity_t, (char*)severity.c_str());
        input->severity = temp_severity != -1 ? (csmi_ras_severity_t) temp_severity : csm_enum_max(csmi_ras_severity_t);

        input->start_time_stamp = strdup(start_time_stamp.c_str());
        input->end_time_stamp = strdup(end_time_stamp.c_str());
        input->location_name = strdup(location_name.c_str());
        input->control_action = strdup(control_action.c_str());
        input->message = strdup(message.c_str());
        input->limit = limit;
        input->offset = offset;
        input->order_by = 'd';

        csm_api_object *csmobj = NULL;
        LOG( csmrestd, info ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << "NEW RAS QUERY" << std::setw(0)
            << jsonIn;
        //csmi_ras_event_vector_t *event_vect = NULL;
        int csmrc = csm_ras_event_query(&csmobj, input, &output);
        //Use CSM API free to release arguments. We no longer need them.
        csm_free_struct_ptr(csm_ras_event_query_input_t, input);
        if  (csmrc != 0) {
            char *errmsg = csm_api_object_errmsg_get(csmobj);
            jsonOut = string("{\"error\":\"") + "CSMRESTD csm_ras_event_create = " + errmsg + "\"}";
            LOG( csmrestd, error ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << "RAS QUERY ERROR" << std::setw(0)
                << " " << jsonIn
                << " errstr: " << errmsg;
            rc = RestApiReply::internal_server_error;
        }
        else {

            boost::property_tree::ptree ev_vect;

            // todo: print out the event rec before formatting the reply...
            //csmi_ras_event_t **p = event_vect->events;
            for (uint32_t n = 0; n < output->results_count; n++) {
                boost::property_tree::ptree ev;
                ev.put(CSM_RAS_FKEY_MSG_ID, output->results[n]->msg_id);
                ev.put(CSM_RAS_FKEY_SEVERITY, output->results[n]->severity);
                ev.put(CSM_RAS_FKEY_TIME_STAMP, output->results[n]->time_stamp);
                ev.put(CSM_RAS_FKEY_LOCATION_NAME, output->results[n]->location_name);
                ev.put(CSM_RAS_FKEY_COUNT, output->results[n]->count);
                ev.put(CSM_RAS_FKEY_CONTROL_ACTION, output->results[n]->control_action);
                ev.put(CSM_RAS_FKEY_MESSAGE, output->results[n]->message);
                ev.put(CSM_RAS_FKEY_RAW_DATA, output->results[n]->raw_data);
                ev_vect.push_back(std::make_pair("", ev));
            }
            boost::property_tree::ptree evw;
            evw.put("num_ras_events",output->results_count); 
            evw.add_child("events", ev_vect);

            ostringstream ss;
            boost::property_tree::json_parser::write_json(ss, evw);
            jsonOut = ss.str();
            LOG( csmrestd, info ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << "RAS QUERY COMPLETE" << std::setw(0)
                << " " << jsonIn;
            rc = RestApiReply::ok;
        }
        csm_api_object_destroy(csmobj);
    }
    catch (std::exception & e) {
      LOG( csmrestd, warning ) << std::setw( LOG_RAS_EVENT_PREFIX_WIDTH ) << std::left << "RAS QUERY ERROR" << std::setw(0)
        <<  " error parsing json data: " << e.what();

        // need to return some sort of error here... internal_server_error and some extra text...?
        rc = RestApiReply::internal_server_error;
        jsonOut = string("{\"error\":\"") + "exception = " + e.what() + "\"}";

    }

    return(rc);

}

RestApiReply::status_type CsmRestApiServer::csmLoopbackTest(
   string const &method, string const &url, string const &jsonIn, string &jsonOut)
{
   jsonOut = jsonIn;       // loop back the json data...

   LOG( csmrestd, info ) << "csmLoopbackTest: Start processing.";

   return(RestApiReply::ok);
}


int CsmRestApiServer::setCallbacks() 
{
    int rc = 0;
    rc = setUrlCallback("POST", "^/csmi/V1.0/ras/event/create$",
                        std::bind(&CsmRestApiServer::csmRasEventCreate, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  std::placeholders::_3,
                                  std::placeholders::_4));
    if (rc != 0) {
        LOG(csmrestd, info) << "setUrlCallback Failed" << endl << flush;
    }

    rc = setUrlCallback("POST", "^/csmi/V1.0/ras/event/query$",
                        std::bind(&CsmRestApiServer::csmRasEventQuery, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  std::placeholders::_3,
                                  std::placeholders::_4));
    if (rc != 0) {
        LOG(csmrestd, info) << "setUrlCallback Failed" << endl << flush;
    }
    rc = setUrlCallback("POST", "^/csmi/V1.0/loopback/test$",
                        std::bind(&CsmRestApiServer::csmLoopbackTest, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  std::placeholders::_3,
                                  std::placeholders::_4));
    if (rc != 0) {
        LOG(csmrestd, info) << "setUrlCallback Failed" << endl << flush;
    }

    return(rc);
}



class CsmRestdMain
{
public:
    CsmRestdMain();

    virtual ~CsmRestdMain();
    /**
     * main test framework entrypoint.
     *
     *
     * @param argc -- c arg count
     * @param argv -- c arg values
     *
     * @return int
     */
    int main (int argc, char *argv[]);

    /**
     * display usage.
     * @param none.
     * @return none.
     * 
     */
    void displayUsage();

    /**
     * Loads configuration information from the specific json configuration file.
     */
    bool LoadConfigFromFile( const string & config_file_name );

    /**
     * Read the value of the specified key from the configuration settings
     */
    string GetValueInConfig(const string key) const;

protected:
    bool _csminit;
    bool _config_init;

    boost::property_tree::ptree _config;

private:

};

CsmRestdMain::CsmRestdMain() :
   _csminit(false),
   _config_init(false),
   _config()
{
}


CsmRestdMain::~CsmRestdMain()
{
  csm_term_lib();
}

void CsmRestdMain::displayUsage() 
{
    std::cerr << "Usage: [options] <address> <port>\n";
    std::cerr << "  [options]\n";
    std::cerr << "     -usessl    -- use ssl for this connection\n";
    std::cerr << "     -nossl     -- don't use ssl for this connection\n";
    std::cerr << "     -cert=<certfile> -- certificiate file\n";
    std::cerr << "     -key=<keyfile>  -- keyfile\n";
    std::cerr << "     -config=<configfile>  -- configuration file\n";

    std::cerr << "    csmrestd 0.0.0.0 5555\n";

}

int CsmRestdMain::main (int argc, char *argv[])
{
   bool useSsl = false;
   string certFile;
   string keyFile;

   string config_file;

   po::options_description usage("Supported Command Line Options");
   string opt_help = std::string(CSM_OPT_HELP_LONG)+','+CSM_OPT_HELP_SHORT;
   string opt_file = std::string(CSM_OPT_FILE_LONG)+','+CSM_OPT_FILE_SHORT;
   usage.add_options()
      (opt_help.c_str(), "Show this help")
      (opt_file.c_str(),  po::value<std::string>(&config_file)->required(), "Configuration file name")
   ;

   po::variables_map vm;

   // Try to parse the commandline options
   try
   {
      po::store( po::parse_command_line(argc, argv, usage), vm);

      if (vm.count(CSM_OPT_HELP_LONG))
      {
         std::cerr << usage << std::endl;
         return 1;
      }

      po::notify(vm);
   }
   catch (const po::error & e)
   {
      cerr << "Error while parsing command line options: " << e.what() << endl << endl;
      cerr << usage << endl;

      return 1;
   }

   if (vm.count(CSM_OPT_FILE_LONG))
   {
      config_file = vm[CSM_OPT_FILE_LONG].as<string>();
      LOG(csmrestd, info) << "Using command line provided config: " << config_file;
   }


   // Get configuration options from the config file
   bool success(false);
   success = LoadConfigFromFile(config_file);
   if (success == true)
   {
      //cout << "Successfully parsed configuration file." << endl;
      LOG(csmrestd, info) << "Successfully parsed configuration file.";
   }
   else
   {
      cerr << "Failed to parse configuration file." << endl;
      return 9;
   }

   string listen_ip = GetValueInConfig(CONFIG_FILE_KEY_LISTENIP);
   if (listen_ip.empty())
   {
      LOG(csmrestd, critical) << "Error: " << CONFIG_FILE_KEY_LISTENIP << " not set in " << config_file;
      return 9;
   }

   string port = GetValueInConfig(CONFIG_FILE_KEY_PORT);
   if (port.empty())
   {
      LOG(csmrestd, critical) << "Error: " << CONFIG_FILE_KEY_PORT << " not set in " << config_file;
      return 9;
   }

   int rc = csm_init_lib();     // singleton csmi initialization..
   if (rc != 0)
   {
      LOG(csmrestd, warning) << "csm_init_lib: rc=" << rc << " (" << strerror( rc ) << ") Please check CSM Daemon status! Will try to contact daemon at next request.";
   }
   else
     _csminit = true;

   //
   // todo, make the connection here much more robust, or move this into some sort
   // of supervisory daemon that will attempt to keep things up all the time...
   //

   try
   {
      // Initialise the server.
      CsmRestApiServer server(listen_ip, port);

      /// The signal_set is used to register for process termination notifications.
      boost::asio::signal_set signals(server.getIoService());
      signals.add(SIGINT);
      signals.add(SIGTERM);
      #if defined(SIGQUIT)
         signals.add(SIGQUIT);
      #endif // defined(SIGQUIT)
      signals.async_wait(boost::bind(&CsmRestApiServer::handleStop, &server));

      server.enableSsl(useSsl);
      if (useSsl)
      {
         server.setSslParms(certFile, keyFile, certFile);
      }

      server.setCallbacks();

      // Run the server until stopped.
      server.run();
   }
   catch (std::exception& e)
   {
     LOG( csmrestd, error ) << "exception: " << e.what();
   }

   return(0);
}

bool CsmRestdMain::LoadConfigFromFile( const std::string & config_file_name )
{
   try
   {
      LOG(csmrestd,info) << "Reading configuration: " << config_file_name;
      pt::read_json(config_file_name, _config);
      _config_init = true;
    }
    catch (pt::json_parser_error& f)
    { 
       LOG(csmrestd, error) << "in config file: " << config_file_name << " ERROR: " << f.what();
       return false;
    }

    try
    {
       // set up the logging component in csm
       std::string component("csmrestd.log");
       initializeLogging(component, _config );
    }
    catch (csm::daemon::Exception& e)
    {
       LOG(csmrestd,error) << "Error loading configuration: " << e.what()
          << " (continuing with default settings...)";
    }

#ifdef REMOVED
  try
  {
    // checking if csm.role is specified
    std::string role = GetValueInConfig(std::string("csm.role"));
    if ( !roleOptionInCommand && !role.empty() )
    {
      boost::algorithm::to_upper(role);
      std::stringstream ss_m, ss_a, ss_c, ss_u;
      if (role == (ss_m << CSM_DAEMON_ROLE_MASTER).str() ) _Role = CSM_DAEMON_ROLE_MASTER;
      else if (role == (ss_a << CSM_DAEMON_ROLE_AGGREGATOR).str() ) _Role = CSM_DAEMON_ROLE_AGGREGATOR;
      else if (role == (ss_u << CSM_DAEMON_ROLE_UTILITY).str() ) _Role = CSM_DAEMON_ROLE_UTILITY;
      else if (role == (ss_c << CSM_DAEMON_ROLE_AGENT).str() ) _Role = CSM_DAEMON_ROLE_AGENT;
      else
      {
        LOG(csmrestd,error) << "csm.role (=" << role << ") is not valid";
        throw csm::daemon::Exception("Invalid daemon role in config file.", EINVAL);
      }
    }
  }
  catch (csm::daemon::Exception& e)
  {
    LOG(csmrestd,error) << "The value in csm.role is not recognized: " << e.what();
    throw;
  }
#endif

   return true;
}

string CsmRestdMain::GetValueInConfig(const string key) const
{
   // in the case when the config is not initialized, return the empty string for this key.
   if (!_config_init)
   {
      return string();
   }

   boost::optional<string> value = _config.get_optional<string>(key);
   if (value.is_initialized()) 
   {
      return (*value);
   }
   else 
   {
      return string();
   }
}



// todo, daemonize this...
int main(int argc, char* argv[])
{
   CsmRestdMain csmRestMain;

   try
   {
     int rc = csmRestMain.main(argc, argv);

     return(rc);
   }
   catch (...)
   {
     LOG( csmrestd, error ) << "Caught Unspecified exception. Exiting...";
   }
   return -1;
}
