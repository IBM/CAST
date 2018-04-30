/*================================================================================

    csmd/src/ras/src/RasEventLog.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "../include/RasEventLog.h"
#include <boost/filesystem/operations.hpp>

using std::flush;
using std::ifstream;

RasEventLog::RasEventLog() :
   _initialized(false),
   _file_is_open(false),
   _file_out(),
   _line_count(0)
{
   init();
}

int RasEventLog::init()
{
   if (_initialized == true)
   {
      return 0;
   }
   _initialized = true;

   // Count the current number of lines in the file
   ifstream file_in(RAS_EVENT_LOG_FILE);
   string line("");

   while (std::getline(file_in,line))
   {
      _line_count++;
   }
   file_in.close();   
  
   if (_line_count >= RAS_EVENT_LOG_MAX_LINES)
   {
      rotateFiles();
   } 
   
   if (_file_is_open == false)
   {
      _file_out.open(RAS_EVENT_LOG_FILE, std::ios_base::app | std::ios_base::out);
      _file_is_open = _file_out.is_open();
   }

   return 0;
}

void RasEventLog::log(string message)
{
   if (_file_is_open)
   {
      _file_out << message;
      if (message.back() != '\n')
      {
         _file_out << endl;
      }
      else
      {
         _file_out << flush;
      }
   
      _line_count++;
   }

   if (_line_count >= RAS_EVENT_LOG_MAX_LINES)
   {
      rotateFiles();
   } 
}

void RasEventLog::rotateFiles()
{
   // Close the current file if it is open
   if (_file_out.is_open())
   {
      _file_out.close();
   }
   _file_is_open = false;

   // Remove the old backup file
   if (boost::filesystem::exists(RAS_EVENT_LOG_FILE_PREVIOUS)) 
   {
      boost::filesystem::remove(RAS_EVENT_LOG_FILE_PREVIOUS);
   }

   // Move the current file to the backup
   if (boost::filesystem::exists(RAS_EVENT_LOG_FILE))
   {
      boost::filesystem::rename(RAS_EVENT_LOG_FILE, RAS_EVENT_LOG_FILE_PREVIOUS);
   } 

   _line_count = 0;

   // Attempt to open the new file
   _file_out.open(RAS_EVENT_LOG_FILE, std::ios_base::app | std::ios_base::out);
   _file_is_open = _file_out.is_open();
}
