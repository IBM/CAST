/*******************************************************************************
 |    Log.h
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
 * \file  Log.h
 * \brief Header file for the message Log() class.
 *
 * \ingroup transport
 */

#ifndef LOG_H_
#define LOG_H_

#include <fstream>
#include <iostream>

#include "stdlib.h"
#include "stdio.h"

#include "Common.h"
#include "logging.h"

namespace txp {

//*****************************************************************************
//  Macros
//*****************************************************************************
/**
 * \brief Sends a line of prefix text to a Log indicating start of dump.
 *
 * \param[in]   VAR If true, prefix line will be generated
 * \param[in]   TEXT Prefix text
 */
#define START_PREFIX(VAR,TEXT) \
    char l_LogBuffer[DEFAULT_LINE_LENGTH] = {'\0'}; \
    if (VAR) { \
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), TEXT, VAR); \
        pLog.write(l_LogBuffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL); \
        pLog.incrIndent(6); \
    }

/**
 * \brief Sends a line of prefix text to a Log indicating end of dump.
 *
 * \param[in]   VAR If true, prefix line will be generated
 * \param[in]   TEXT Prefix text
 * \param[in]   PRINT_TRAILING_BLANK_LINE If true, ends output with blank line
 */
#define END_PREFIX(VAR,TEXT,PRINT_TRAILING_BLANK_LINE) \
    if (VAR) { \
        pLog.decrIndent(6); \
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), TEXT, VAR); \
        pLog.write(l_LogBuffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL); \
        if (PRINT_TRAILING_BLANK_LINE) { \
            snprintf(l_LogBuffer, sizeof(l_LogBuffer), " "); \
            pLog.write(l_LogBuffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL); \
        } \
    }

const int DEFAULT_LINE_LENGTH = 255;

/**
 * \class Log
 * Simple logging class with timestamps.
 */
class Log {
public:

    /**
     * \class Indent
     * Handles indentation for Log class.
     */
    class Indent {
    public:

        // Non-static methods

        /**
         * \brief Initializes an indent object.
         *
         * Sets the indent to zero.
         */
        void init() { value = 0; return; };

        /**
         * \brief Decrements the indent.
         *
         * \param[in]   pValue Amount to decrement the current indent value
         */
        void decr(int32_t pValue) { value -= pValue; return; };

        /**
         * \brief Returns the current indent value.
         *
         * \return   int32_t Current indent value
         */
        int32_t get() { return value; };

        /**
         * \brief Increments the indent.
         *
         * \param[in]   pValue Amount to increment the current indent value
         */
        void incr(int32_t pValue) { value += pValue; return; };

        /**
         * \brief Set the indent value.
         *
         * \param[in]   pValue New indent value
         */
        void set(int32_t pValue) { value = pValue; return; };

        // Constructors

        /**
         * \brief Default constructor
         *
         * Sets the indent value to zero.
         */
        Indent() : value(0) {};

        /**
         * \brief Destructor
         */
        virtual ~Indent();

        // Data members
        int32_t value;      //! Current indent value
    };

    // Constants

    /**
     * \brief Destination for output
     */
    enum Destination { NONE=0, BOOST=1, CONSOLE=2, LOGFILE=4, CONSOLE_AND_LOGFILE=6 };

    /**
     * \brief Logging levels
     */
    enum LogLevel { NO_LOGLEVEL=-3,
                    ALL_LOGLEVEL=-2,
                    DEFAULT_LOGLEVEL=-1,
                    ERROR_LOGLEVEL=10,
                    WARN_LOGLEVEL=30,
                    DIAG_LOGLEVEL=50,
                    INFO_LOGLEVEL=70,
                    DEBUG_LOGLEVEL=72,
                    DEBUG_LOGLEVEL_2=74,
                    DEBUG_LOGLEVEL_3=76,
                    DEBUG_LOGLEVEL_4=78,
                    DEBUG_LOGLEVEL_5=80,
                    C_DTOR_LOGLEVEL=90,
                    C_DTOR_LOGLEVEL_2=91,
                    ENTER_EXIT_LOGLEVEL=95,
                   };

    /**
     * \brief Open state for the log.
     */
    enum OpenState { CLOSED=0, OPEN=1 };

    /**
     * \brief Output types to log.
     */
    enum Type { INFO=0, DIAG=1, WARN=1<<1, ERROR=1<<2, DEBUG=1<<3, DBG2=1<<4, DBG3=1<<5, DBG4=1<<6, DBG5=1<<7};

    // Static data
    static Destination DEFAULT_LOG_DESTINATION;
    static OpenState DEFAULT_OPEN_LOGSTATE;
    static LogLevel DEFAULT_OPEN_LOGLEVEL;
    static LogLevel DEFAULT_OPEN_DUMP_LOGLEVEL;

    // Static methods

    /**
     * \brief Convert Destination value to a printable array of character data.
     *
     * \param[in]   pValue Destination value.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void destinationToChar(const Destination pValue, char* pBuffer, const int32_t pSize);

    /**
     * \brief Dump raw buffer
     *
     * Data is dumped in hex with a right-side eyecatcher
     *
     * \param[in]   pLog Output log
     * \param[in]   pBuffer Buffer to dump
     * \param[in]   pSize Size to dump
     * \param[in]   pPrefix Prefix text to output before dump
     */
    static void dump_buffer_raw(Log& pLog, const char* pBuffer, const size_t pSize, const char* pPrefix);

    /**
     * \brief Convert log level to a printable array of character data
     *
     * \param[in]   pLogLevel Log level.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void logLevelToChar(const LogLevel pLogLevel, char* pBuffer, const int32_t pSize);

    /**
     * \brief Set Default Log Destination
     *
     * \param[in]   pValue Value.
     */
    static void setDefaultLogDestination(const std::string pValue);

    /**
     * \brief Set Default Open Log State
     *
     * \param[in]   pValue Value.
     */
    static void setDefaultOpenLogState(const std::string pValue);

    /**
     * \brief Set Default Open LogLevel
     *
     * \param[in]   pValue Value.
     */
    static void setDefaultOpenLogLevel(const std::string pValue);

    /**
     * \brief Set Default Open Dump LogLevel
     *
     * \param[in]   pValue Value.
     */
    static void setDefaultOpenDumpLogLevel(const std::string pValue);

    /**
     * \brief Convert type to a printable array of character data
     *
     * \param[in]   pType Type.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void typeToChar(const Type pType, char* pBuffer, const int32_t pSize);

    // Inlined non-static methods

    /**
     * \brief Decrements the indent for the log
     *
     * \param[in]   pValue Indent value will be decremented by this value.
     */
    void decrIndent(int32_t pValue) { myIndent.decr(pValue); return; };

    /**
     * \brief Return indent vlaue
     *
     * \return      int32_t Current indent value
     */
    int32_t getIndent() { return myIndent.get(); };

    /**
     * \brief Return the logging level for the log
     *
     * \return      LogLevel Current logging level
     */
    LogLevel getLogLevel() { return msgLogLevel; };

    /**
     * \brief Increments the indent for the log
     *
     * \param[in]   pValue Indent value will be incremented by this value.
     */
    void incrIndent(int32_t pValue) { myIndent.incr(pValue); return; };

    /**
     * \brief Set the indent value for the log
     *
     * \param[in]   pValue Indent value is set to this value.
     */
    void setIndent(int32_t pValue) { myIndent.set(pValue); return; };

    /**
     * \brief Set the logging level for the log
     *
     * \param[in]   pLogLevel Logging level is set to this value.
     */
    void setLogLevel(LogLevel pLogLevel) { msgLogLevel = pLogLevel; return; };

    // Non-static methods

    /**
     * \brief Close the log.
     * When the log is closed, output is not allowed to the logging destinations.
     *
     * \return      int  0 -> success;  <0 -> failure
     */
    int close();

    /**
     * \brief Output entry to method entry
     *
     * \param[in]   pFile Source file name
     * \param[in]   pModule Module name
     * \param[in]   pLogLevel Log level for the entry, default is ENTER_EXIT_LOGLEVEL
     * \param[in]   pDestination Pointer to destination object to override the default one in the log.  Default is nil.
     * \return      int  0 -> success;  <0 -> failure
     */
    int enter(const char* pFile, const char* pModule, const LogLevel pLogLevel=ENTER_EXIT_LOGLEVEL, const Destination* pDestination=0);

    /**
     * \brief Output entry to method exit
     *
     * \param[in]   pFile Source file name
     * \param[in]   pModule Module name
     * \param[in]   pRC Exiting return code
     * \param[in]   pLogLevel Log level for the entry, default is ENTER_EXIT_LOGLEVEL
     * \param[in]   pDestination Pointer to destination object to override the default one in the log.  Default is nil.
     * \return      int  0 -> success;  <0 -> failure
     */
    int exit(const char* pFile, const char* pModule, const int pRC, const LogLevel pLogLevel=ENTER_EXIT_LOGLEVEL, const Destination* pDestination=0);

    /**
     * \brief Loglevel check
     *
     * \param[in]   pLogLevel Log level.
     * \return      0 = Does not statisfy loglevel clip value; 1 = Does satisfy;
     */
    int logLevelCheck(const LogLevel pLogLevel);

    /**
     * \brief Open the log.
     * When the log is open, output is allowed to the logging destinations.
     *
     * \return      int  0 -> success;  <0 -> failure
     */
    int open();

    /**
     * \brief Write to the log
     *
     * \param[in]   pData Pointer to character data to log.
     * \param[in]   pType Type for the entry, default is INFO.
     * \param[in]   pLogLevel Log level for this entry to log.  Default is DEFAULT_LOGLEVEL.
     * \param[in]   pDestination Pointer to destination object to override the default one in the log.  Default is nil.
     * \return      int  0 -> success;  <0 -> failure
     */
    int write(const char* pData, const Type pType=INFO, const LogLevel pLogLevel=DEFAULT_LOGLEVEL, const Destination* pDestination=0);

    /**
     * \brief Write to the log
     *
     * \param[in]   pData Pointer to character data to log.
     * \param[in]   pType Type for the entry.
     * \param[in]   pLogLevel Log level for this entry to log.
     * \param[in]   pDestination Destination object.
     * \return      int  0 -> success;  <0 -> failure
     */
    int write(const char* pData, const Type pType, const LogLevel pLogLevel, const Destination pDestination);

    // Constructors

    /**
     * \brief Default constructor
     * Default destination is to CONSOLE.
     *
     * \param[in]   pOpen Open state for the log.  Default is CLOSED.
     * \param[in]   pLogLevel Logging level for the log.  Default is DEFAULT_LOGLEVEL.
     */
    Log(OpenState pOpen=CLOSED, LogLevel pLogLevel=DEFAULT_LOGLEVEL);

    /**
     * \brief Constructor with destination
     *
     * \param[in]   pDestination Destination for output.
     * \param[in]   pOpen Open state for the log.  Default is CLOSED.
     * \param[in]   pLogLevel Logging level for the log.  Default is DEFAULT_LOGLEVEL.
     */
    Log(Destination pDestination, OpenState pOpen=CLOSED, LogLevel pLogLevel=DEFAULT_LOGLEVEL);

    /**
     * \brief Constructor with destination and logfile
     * Default open mode is std::ofstream::out.
     *
     * \param[in]   pDestination Destination for output.
     * \param[in]   pLogFile Logfile for output.
     * \param[in]   pOpen Open state for the log.  Default is CLOSED.
     * \param[in]   pLogLevel Logging level for the log.  Default is DEFAULT_LOGLEVEL.
     */
    Log(Destination pDestination, const char* pLogFile, OpenState pOpen=CLOSED, LogLevel pLogLevel=DEFAULT_LOGLEVEL);

    /**
     * \brief Constructor with destination, logfile, and open mode
     *
     * \param[in]   pDestination Destination for output.
     * \param[in]   pLogFile Logfile for output.
     * \param[in]   pMode Open mode for logfile.
     * \param[in]   pOpen Open state for the log.  Default is CLOSED.
     * \param[in]   pLogLevel Logging level for the log.  Default is DEFAULT_LOGLEVEL.
     */
    Log(Destination pDestination, const char* pLogFile, const std::ios_base::openmode pMode, OpenState pOpen=CLOSED, LogLevel pLogLevel=DEFAULT_LOGLEVEL);

    /**
     * \brief Destructor
     */
    virtual ~Log();

private:

    // Inlined non-static methods

    /**
     * \brief Return indicator if output should go to the boost logging facility
     *
     * \param[in]   pDestination Destination for this output.
     * \return      int  0 -> not to console;  1 -> to console
     */
    int toBoost(const Destination pDestination) { return (pDestination & 0x00000001); };

    /**
     * \brief Return indicator if output should go to console
     *
     * \param[in]   pDestination Destination for this output.
     * \return      int  0 -> not to console;  1 -> to console
     */
    int toConsole(const Destination pDestination) { return ((pDestination & 0x00000002) >> 1); };

    /**
     * \brief Retrun indicator if output should go to a logfile
     *
     * \param[in]   pDestination Destination for this output.
     * \return      int  0 -> not to logfile;  1 -> to logfile
     */
    int toLogFile(const Destination pDestination) { return ((pDestination & 0x00000004) >> 2); };

    // Non-static methods

    /**
     * \brief Return current time
     *
     * \param[in]   pBuffer Buffer for time value.
     * \param[in]   pSize Size available for time value.
     */
    void getCurrentTime(char* pBuffer, const int32_t pSize);

    /**
     * \brief Initialize the log
     *
     * \param[in]   pOpen Initial open state
     * \param[in]   pLogLevel Logging level to compare against as entries are sent to the log
     */
    void init(OpenState pOpen, LogLevel pLogLevel);

    /**
     * \brief Process the output buffer to destinations
     *
     * \param[in]   pBuffer Buffer of data.
     * \param[in]   pLogLevel Log level for this data to output.
     * \param[in]   pType Type for the entry.
     * \param[in]   pDestination Destination for the output.
     */
    int processBuffer(const char* pBuffer, const LogLevel pLogLevel, const Type pType, const Destination pDestination);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pLog
     * \note Not implemented
     */
    Log(const Log &pLog) {};

    // Data members

    Destination destination;                //! Default destination of log

    Indent                  myIndent;       //! Current indent value for the log
    int32_t                 numberOfOpens;  //! Number of opens for the log
                                            //! \note Can only go to 1 today...
    LogLevel                msgLogLevel;    //! Log level for this log.  Messages being
                                            //! sent to this log are compared to this value.
    bool                    isOpen;         //! Is open flag
    char                    logFile[256];   //! Log file name
    std::ios_base::openmode logFileMode;    //! Log file open mode
    std::ofstream           logFileStream;  //! Log file stream

};

} // namespace

#endif /* LOG_H_ */

