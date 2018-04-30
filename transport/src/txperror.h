/*******************************************************************************
 |    txperror.h
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

#ifndef TXP_TXPERROR_H_
#define TXP_TXPERROR_H_

#include "tstate.h"

/*******************************************************************************
 | Macros
 *******************************************************************************/
#define LOG_ERROR_TEXT(TEXT) { \
    LOG(bb,error) << TEXT.str(); \
    txperror << err("error.text", TEXT.str()); \
}

#define LOG_ERROR_TEXT_RC(TEXT,RC) { \
    LOG(bb,error) << TEXT.str(); \
    txperror << err("error.text", TEXT.str()) << errloc(RC); \
}

#define LOG_ERROR_TEXT_AND_BAIL(TEXT) { \
    LOG(bb,error) << TEXT.str(); \
    txperror << err("error.text", TEXT.str()) << bailout; \
}

#define LOG_ERROR_TEXT_RC_AND_BAIL(TEXT,RC) { \
    LOG(bb,error) << TEXT.str(); \
    txperror << err("error.text", TEXT.str()) << errloc(RC) << bailout; \
}


namespace txp {

    /*******************************************************************************
     | Classes
     *******************************************************************************/
    class TXP_Handler : public TSHandler
    {
        public:
            TXP_Handler() :
                TSHandler() {};
            ~TXP_Handler() {};
    };


    /*******************************************************************************
     | External data
     *******************************************************************************/
    extern thread_local TXP_Handler txperror;

} // namespace

#endif /* TXP_TXPERROR_H_ */
