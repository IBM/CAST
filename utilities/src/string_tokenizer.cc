/*******************************************************************************
 |    string_tokenizer.cc
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


//////////////////////////////////////////////////////////////////////
//
// Basic string tokenizer class
//

#include "string_tokenizer.h"

///////////////////////////////////////////////////////////////////////////
int StringTokenizer::tokenize(const std::string &rStr, 
                              const char *pDelimiters,
                              const char *pComment,
                              unsigned int nLimit)
//
// inputs:
//    rStr -- source string to tokenize
//    rDelimiters -- delimiter string
//    rComment -- comment characters
//    nLimit -- upper limit of number of tokens to parse out.
//              0 == no limit.
// outputs:
//    returns -- number of tokens parsed.
//    this -- contains a vector of the parsed strings.
//
//
{
    clear();        // clear out the last string.
    if (pDelimiters == NULL)            // no delimiters, then nothing to do...
        return(0);
	std::string::size_type lastPos(rStr.find_first_not_of(pDelimiters, 0));
	std::string::size_type pos(rStr.find_first_of(pDelimiters, lastPos));
	while (std::string::npos != pos || std::string::npos != lastPos)
	{
        // do we have comment delimiters
        if (pComment)       
        {
		    if (rStr.find_first_of(pComment, lastPos) == 0)     // if so, is this a comment.
                break;                                          // then we are done...
        }                
        
        if (nLimit)
        {
            if (nLimit <= size()+1)
            {
                push_back(rStr.substr(lastPos));     // take the remainder as one token...
                break;
            }
        
        }
		push_back(rStr.substr(lastPos, pos - lastPos));
		lastPos = rStr.find_first_not_of(pDelimiters, pos);
		pos = rStr.find_first_of(pDelimiters, lastPos);
        
	}
    return(size());
}


