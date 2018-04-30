/*******************************************************************************
 |    Common.h
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


/**
 * \file  Common.h
 * \brief This file contains the common definitions for the transport facility.
 *
 * \defgroup transport Transport Facility
 */


/**
 \page transport_hl Transport Facility

The transport facility provides a mechnism to pass data from process to process.  The client builds a message object and associates data attributes with the message.  The client can then send the message to another process using the facility.  The transport facility serializes the data and then performs the send using the connection method specified by the client.  Two connection methods are currently supported, sockets and RDMA.  The data is received by the corresponding transport facility at the target and is automatically deserialized into a message object.  The message can then be processed by code running at the target.  As part of the processing at the target, a response message can be built from the original message and additional data attributes associatged with the response.  Similar to the original message, the response message is then sent back to the source, first being serialized and then deserialized into a message object at the source.  The client can then process the response.

Work in progress...

 \par Examples

TBD...

 \verbatim

 \endverbatim

 */


#ifndef COMMON_H_
#define COMMON_H_


/**
 * \brief TXP_DEVELOPMENT Compiler directive tag to compile in/out debug txp::Log logging code.
 * \note Undefine TXP_DEVELOPMENT to compile out all txp::Log logging code
 * \ingroup transport
 */
// #define TXP_DEVELOPMENT

#define TXP_ATTR_TYPE_TYPE uint16_t
#define TXP_ATTR_NAME_TYPE uint16_t
#define TXP_CHAR_ARRAY_LENGTH_TYPE size_t
#define TXP_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS_TYPE size_t
#define TXP_ARRAY_OF_CHAR_ARRAYS_LENGTH_TYPE size_t
#define TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE size_t
#define TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA_TYPE char*

#include <ctype.h>
#include "stdint.h"
#include "string.h"

#include "logging.h"
#include "txp_flightlog.h"
#include "util.h"

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

/*
 *  Return code ranges by Msg() function:
 *        Msg() -1   -> -199
 *      State() -201 -> -250
 *    Version() -251 -> -299
 * HeapBuffer() -301 -> -399
 *  Attribute() -401 -> -499
 */

namespace txp {

// External data
extern int16_t initPerformed;

// External methods
extern void doInit();

// External inlined methods
extern inline int init() {
    int l_RC = 0;

    if (!initPerformed) {
        txp::doInit();
        l_RC = 1;
    }

    return l_RC;
};

enum COPY_DATA_OPTION {
    DO_NOT_COPY          = 0,
    COPY_TO_HEAP         = 1
};

enum CRC {
    DO_NOT_CALCULATE_CRC = 0,
    CALCULATE_CRC        = 1
};

#include "gened_Common.h"

}   // namespace

#endif /* COMMON_H_ */
