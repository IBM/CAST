/*================================================================================
   
    csmd/src/inv/tests/test_inv_ib_guid.cc

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>
#include <iomanip>
#include <functional>
#include <map>

#include "inv_ib_guid.h"

using std::cout;
using std::endl;
using std::setw;
using std::left;
using std::right;
using std::string;
using std::function;
using std::map;

int main(void)
{
  const string min_guid("0123456789abcdef");
  const string prefix_guid("0x0123456789abcdef");
  const string colon_guid("0123:4567:89ab:cdef");

  // map from the string name of the functions to the function object  
  map<const string, const function<string(string)>> funcmap =
  {
    { "stripGuid", stripGuid },
    { "standardizeGuid", standardizeGuid },
    { "getMinimumGuid", getMinimumGuid },
    { "getColonGuid", getColonGuid },
    { "getPrefixGuid", getPrefixGuid }
  };

  // Helper lambda to run each of the different test case permutations
  const auto runTest = [&funcmap](const string &name, const string &input, const string &expected) 
  { 
    string output = funcmap[name](input);
    if (output == expected)
    {
      cout << "Pass: " << setw(16) << name << "(" << setw(20) << input << ") -> " << setw(20) << output 
           << " == " << setw(20) << left << expected << right << endl;
    }
    else
    {
      cout << "Fail: " << setw(16) << name << "(" << setw(20) << input << ") -> " << setw(20) << output 
           << " != " << setw(20) << left << expected << right << endl;
    }
  };

  // Run all of the good case tests
  cout << "Valid input tests:" << endl;  
  const string guids[] = { min_guid, prefix_guid, colon_guid };
  for (string guid : guids)
  { 
    runTest("stripGuid", guid, min_guid);
    runTest("standardizeGuid", guid, colon_guid);
    runTest("getMinimumGuid", guid, min_guid);
    runTest("getColonGuid", guid, colon_guid);
    runTest("getPrefixGuid", guid, prefix_guid);
  }
  
  // Run all of the bad case tests, input guid should be returned unmodified
  cout << endl << "Invalid input tests:" << endl;  
  const string invalid_guids[] = { 
    "0123456789abcdeff", "0x0123456789abcdeff", "0123:4567:89ab:cdeff",  // Too long
    "0123456789abcde", "0x0123456789abcde", "0123:4567:89ab:cde"         // Too short
  };
  for (string guid : invalid_guids)
  { 
    runTest("standardizeGuid", guid, guid);
    runTest("getMinimumGuid", guid, guid);
    runTest("getColonGuid", guid, guid);
    runTest("getPrefixGuid", guid, guid);
  }

  return 0;
}
