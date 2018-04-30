/*******************************************************************************
 |    Version.h
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
 * \file  Version.h
 * \brief Header file for the message Version() class.
 *
 * \ingroup transport
 */

#ifndef VERSION_H_
#define VERSION_H_

#include "Common.h"

#include "HeapBuffer.h"
#include "Log.h"

namespace txp {

/**
 * \class Version
 * Describes the version for a message
 *
 * A Version is made up of a major and minor value.
 */
class Version {
public:
    friend class Msg;

    // Constants
    static const char USE_LATEST_MAJOR = 0x00;
    static const char USE_LATEST_MINOR = 0x00;

    // Static data
    static txp::Log d_log;
    static char d_log_buffer[255];

    // Static methods

    /**
     * \brief Returns the latest version value for a given message id.
     *
     * \param[in]   pId
     * \return      Version
     *
     * \internal    Not currently implemented @@DLH
     * \endinternal
     */
    static Version getLatestVersion(const Id pId);

    // Non-static methods

    /**
     * \brief Dumps Version to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    /**
     * \brief Returns the major value
     *
     * \return      uint8_t Major value
     */
    uint8_t getMajor() { return value.major; };

    /**
     * \brief Returns the minor value
     *
     * \return      uint8_t Minor value
     */
    uint8_t getMinor() { return value.minor; };

    // Constructors

    /**
     * \brief Default constructor
     *
     * Sets the major and minor values to zero.
     */
    Version();

    // Constructs a version with the latest major,minor for a given message id
    /**
     * \brief Construct with message id
     *
     * \param[in]   pId
     */
    Version(const Id pId);

    /**
     * \brief Construct with major and minor values
     *
     * \param[in]   pMajor Major value
     * \param[in]   pMinor Minor value
     */
    Version(const uint8_t pMajor, const uint8_t pMinor);

    /**
     * \brief Construct with major/minor value
     *
     * \param[in]   pMajorMinor Major/minor value
     */
    Version(const uint16_t pMajorMinor);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pVersion
     */
    Version(const Version &pVersion);

    /**
     * \brief Destructor
     */
    virtual ~Version();

private:
    // Non-static methods

    /**
     * \brief Adds the Version data to a heap buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -251 Failure when adding the major value.\n
     * -252 Failure when adding the minor value.
     */
    int add(HeapBuffer* pBuffer);

    /**
     * \brief Sets the minor value
     *
     * \param[in]   pMinor Minor value
     */
    void setMinor(const uint8_t pMinor);

    /**
     * \brief Sets the major and minor values
     *
     * \param[in]   pMajor Major value
     * \param[in]   pMinor Minor value
     */
    void set(const uint8_t pMajor, const uint8_t pMinor);

    /**
     * \brief Sets the major value
     *
     * \param[in]   pMajor Major value
     */
    void setMajor(const uint8_t pMajor);

    /**
     * \brief Sets the major and minor values
     *
     * \param[in]   pMajorMinor Major/minor value
     */
    void set(const uint16_t pMajorMinor);

    // Data members
    union {
        uint16_t major_minor;
        struct {
            uint8_t major;  //! Major value
            uint8_t minor;  //! Minor value
        };
    } value;
};

} // namespace

#endif /* VERSION_H_ */
