/*******************************************************************************
 |    string_tokenizer.h
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


#ifndef __STRING_TOKENIZER__H__
#define __STRING_TOKENIZER__H__

//////////////////////////////////////////////////////////////////////
//
// Basic string tokenizer class
//

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////
//
// this class takes a source string ad a string of delimiters
// and tokenizes it into a vector of strings.
//
// successive delimiters are treated as a single delmiter.
// 
// currently it does not handle quoted strings.
//
//
class StringTokenizer : public std::vector<std::string>
{
	public:
        ///////////////////////////////////////////////////////////////////
        int tokenize(const std::string &rStr, 
                     const char *pDelimiters =" ,\n",
                     const char *pComment = NULL,
                     unsigned int nLimit = 0);
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
        // If the comment field is not empty then it defines the character
        // which is the comment character.
        //
        ////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        /*!
        // Trim characters from the right part of the string.
        //
        // @param source -- string to trimp
        // @param t -- characters to trim.
        // @returns trimmed string
        */
        std::string trim_right ( const std::string & source ,
                                 const std::string & t = " " )
        {
            std::string str = source;
            return str.erase ( str.find_last_not_of ( t ) + 1 ) ;
        }
        ////////////////////////////////////////////////////////////////
        /*!
        // Trim characters from the leftpart of the string.
        //
        // @param source -- string to trimp
        // @param t -- characters to trim.
        // @returns trimmed string
        */
        std::string trim_left ( const std::string & source ,
                                const std::string & t = " " )
        {
            std::string str = source;
            return str.erase ( 0 , source.find_first_not_of ( t ) ) ;
        }

        ////////////////////////////////////////////////////////////////
        /*!
        // Trim characters from the leftpart of the string.
        //
        // @param source -- string to trimp
        // @param t -- characters to trim.
        // @returns trimmed string
        */
        std::string trim ( const std::string & source ,
                           const std::string & t = " " )
        {
            std::string str = source;
            size_type n;
            
            n = source.find_first_not_of ( t ) ;
            if (n)
                str.erase ( 0 , n) ;
            return str.erase (str.find_last_not_of ( t ) + 1 ) ;
        }

        
};



#endif

