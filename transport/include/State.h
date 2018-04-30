/*******************************************************************************
 |    State.h
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


/*
 * \file  State.h
 * \brief Header file for the message State() class.
 *
 * \ingroup transport
 */

#ifndef STATE_H_
#define STATE_H_

#include "Common.h"

#include "HeapBuffer.h"
#include "Log.h"

namespace txp {

/**
 * \class State
 * Describes the state of a message
 */
class State {
public:
    friend class Msg;

    // Static data
    static txp::Log d_log;
    static char d_log_buffer[255];

    // Non-static methods

    /**
     * \brief Does Attribute Exist with a Pointer to Non-Array Data.
     *
     * \return      int  0 -> does not exist;  1 -> exists
     */
    inline int attrExistsWithPtrToNonArrayData() { return (stateFlag.msgAttrExistsWithPtrToNonArrayData ? 1 : 0); };

    /**
     * \brief Dumps State to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Inlined methods

    /**
     * \brief Is Storage Allocated for Msg() by Facility.
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline int isAllocated() { return (stateFlag.allocated ? 1 : 0); };

    /**
     * \brief Is Message CRC Value Calculated.
     *
     * \return      int  0 -> not calculated;  1 -> calculated
     */
    inline int isMsgCRC_Calculated() { return (stateFlag.msgCRC_IsCalculated ? 1 : 0); };

    /**
     * \brief Is Heap Buffer Allocated.
     *
     * \return      int  0 -> not current;  1 -> current
     */
    inline int isHeapBufferAllocated() { return (stateFlag.heapBufferAllocated ? 1 : 0); };

    /**
     * \brief Is Heap Buffer Current with Attributes.
     *
     * \return      int  0 -> not current;  1 -> current
     */
    inline int isHeapBufferCurrent() { return (stateFlag.heapBufferIsCurrent ? 1 : 0); };

    /**
     * \brief Is I/O Vector Current with Attributes.
     *
     * \return      int  0 -> not current;  1 -> current
     */
    inline int isIO_VectorCurrent() { return (stateFlag.IO_VectorIsCurrent ? 1 : 0); };

    /**
     * \brief Is Send Allowed.
     *
     * \return      int  0 -> not allowed;  1 -> allowed
     */
    inline int isSendAllowed() { return (stateFlag.sendAllowed ? 1 : 0); };

    /**
     * \brief Is Msg() Response Sent.
     *
     * \return      int  0 -> response not sent;  1 -> response sent
     */
    inline int isResponseSent() { return (stateFlag.response ? 1 : 0); };

    /**
     * \brief Is Msg() Sent.
     *
     * \return      int  0 -> not sent;  1 -> sent
     */
    inline int isSent() { return (stateFlag.sent ? 1 : 0); };

private:
    // Non-static methods

    /**
     * \brief Adds the State data to a heap buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -201 Failure for the add operation.
     */
    int add(HeapBuffer* pBuffer);

    // Inlined methods

    /**
     * \brief Clear flags.
     */
    inline void clearFlags() { stateFlag.flags = 0; return; };

    /**
     * \brief Set allocated flag.
     *
     * \param[in]   pValue 0 -> not allocated;  1 -> allocated
     */
    inline void setAllocatedFlag(const int pValue) { stateFlag.allocated = pValue; return; };

    /**
     * \brief Set attribute exists to non-array data flag.
     *
     * \param[in]   pValue 0 -> does not exist;  1 -> exists
     */
    inline void setAttrExistsWithPtrToNonArrayData(const int pValue) { stateFlag.msgAttrExistsWithPtrToNonArrayData = pValue; return; };

    /**
     * \brief Set heap buffer allocated flag.
     *
     * \param[in]   pValue 0 -> not current;  1 -> current
     */
    inline void setHeapBufferAllocatedFlag(const int pValue) { stateFlag.heapBufferAllocated = pValue; return; };

    /**
     * \brief Set heap buffer is current flag.
     *
     * \param[in]   pValue 0 -> not current;  1 -> current
     */
    inline void setHeapBufferIsCurrentFlag(const int pValue) { stateFlag.heapBufferIsCurrent = pValue; return; };

    /**
     * \brief Set I/O Vector is current flag.
     *
     * \param[in]   pValue 0 -> not current;  1 -> current
     */
    inline void setIO_VectorIsCurrentFlag(const int pValue) { stateFlag.IO_VectorIsCurrent = pValue; return; };

    /**
     * \brief Set Message CRC Calculated Value.
     *
     * \param[in]   pValue 0 -> not calculated;  1 -> calculated
     */
    inline void setMsgCRC_IsCalculated(const int pValue) { stateFlag.msgCRC_IsCalculated = pValue; return; };

    /**
     * \brief Set response sent flag.
     *
     * \param[in]   pValue 0 -> response not sent;  1 -> response sent
     */
    inline void setResponseSentFlag(const int pValue) { stateFlag.response = pValue; return; };

    /**
     * \brief Set sent flag.
     *
     * \param[in]   pValue 0 -> not sent;  1 -> sent
     */
    inline void setSentFlag(const int pValue) { stateFlag.sent = pValue; return; };

    /**
     * \brief Set sent allowed flag.
     *
     * \param[in]   pValue 0 -> not allowed;  1 -> allowed
     */
    inline void setSendAllowedFlag(const int pValue) { stateFlag.sendAllowed = pValue; return; };

    // Constructors

    /**
     * \brief Default constructor
     *
     * Sets all flags off.
     */
    State();

    /**
     * \brief Construct with pre-set flags
     *
     * \param[in]   pFlags Flag values
     */
    State(const uint16_t pFlags);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pState
     * \note Not implemented
     */
    State(const State &pState) {};

    /**
     * \brief Destructor
     */
    virtual ~State();

    // Data members
    union {
        uint16_t    flags;
        struct {
            uint8_t reserved_1:4;

	        uint8_t msgAttrExistsWithPtrToNonArrayData:1;
	                                        //! Massage has attribute added that has
	                                        //! a pointer to non-array data
            uint8_t msgCRC_IsCalculated:1;  //! Message has a calculated CRC value
            uint8_t heapBufferIsCurrent:1;  //! Heap buffer is current with
	                                        //! msgAttributes
            uint8_t IO_VectorIsCurrent:1;   //! I/O vector is current with
	                                        //! msgAttributes

            uint8_t reserved_2:1;
            uint8_t sendAllowed:1;          //! Send is allowed
            uint8_t heapBufferAllocated:1;  //! Associated heap buffer is allocated
                                            //! by message facility and therefore,
                                            //! will be deleted by the message facility.
            uint8_t allocated:1;            //! Msg() is allocated by message facility
	                                        //! and therefore, will be deleted by the
	                                        //! message facility.

            uint8_t reserved_3:2;
            uint8_t response:1;             //! A response has been sent for the messsage
            uint8_t sent:1;                 //! Msg() has been sent
        };
    }  stateFlag;
};

} // namespace

#endif /* STATE_H_ */
