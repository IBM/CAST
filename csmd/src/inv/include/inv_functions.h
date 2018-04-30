/*================================================================================
   
    csmd/src/inv/include/inv_functions.h

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

// This file contains common helper functions used by the main inventory collection functions

#ifndef __INV_FUNCTIONS_H
#define __INV_FUNCTIONS_H

#include "logging.h"

#include <string>

using namespace std;

// Remove any non-printable characters and white space from the beginning or end of the string (but not the middle)
// Example: "  PCIe3 1.6TB   " should produce "PCIe3 1.6TB"
static string trimString(const string& value)
{
  string outstring(value);

  // Start at the beginning and erase any non-printable characters 
  for (std::size_t i = 0; !outstring.empty() && !isgraph(outstring[i]); i=0)
  {
    outstring.erase(i,1);
  }

  // Start at the end and erase any non-printable characters 
  for (std::size_t i = outstring.size()-1; !outstring.empty() && !isgraph(outstring[i]); i=outstring.size()-1)
  {
    outstring.erase(i,1);
  }

  if (outstring.size() != value.size())
  {
    LOG(csmd, debug) << "trimmed " << (value.size() - outstring.size()) << " from [" << outstring << "]";
  }

  return outstring;
}

#endif
