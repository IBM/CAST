/*================================================================================

    csmd/src/db/include/csm_db_event_content.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_DB_EVENT_CONTENT_H_
#define CSM_DAEMON_SRC_CSM_DB_EVENT_CONTENT_H_

#include "include/csm_core_event.h"           //: DBResult_sptr (XXX I think -John Dunham)
#include "csmd/src/db/include/DBConnection.h" //: DBConn source.
#include "logging.h"                          //: For logging funtionality.
#include <endian.h>                           //: Used to change the endianess of binary numbers.
#include <libpq-fe.h>                         //: Postgresql header.

/** @brief A helper macro for adding optional sql parameters.
 *
 * @warning ONLY USE FOR SQL PARAMETERS THAT ARE OPTIONAL
 * 
 * @param[out] stmt A std::string to append the values to.
 * @param[in]  condition A conditional statement that determines whether this gets executed.
 * @param[in]  index The index of this part of the statement, ++index will result in 
 *                  the index being incremented only once.
 * @param[in]  before_index The string to append before the parameter index.
 * @param[in]  after_index The string to append after the parameter index.
 */
#define add_param_sql( stmt, condition, index, before_index, after_index ) \
    if ( condition ) stmt.append(before_index).append(std::to_string(index)).append(after_index);


/** @brief A helper macro for adding required sql parameters with variable ordinality.
 *
 * 
 * @param[out] stmt A std::string to append the values to.
 * @param[in]  index The index of this part of the statement, ++index will result in 
 *                  the index being incremented only once.
 * @param[in]  before_index The string to append before the parameter index.
 * @param[in]  after_index The string to append after the parameter index.
 */
#define add_param_sql_fixed(stmt, index, before_index, after_index ) \
   stmt.append(before_index).append(std::to_string(index)).append(after_index);

namespace csm {
namespace db {

/** @brief An enum for tracking the type of content held by DBContent class.
 */
enum DBContentType {
    CSM_DB_REQ,
    CSM_DB_RESP
};

/**
 * Base class for content interacting with the database.
 */
class DBContent
{
private:
  DBContentType _ContentType;
  DBConnection_sptr _DBConn;

protected:

  DBContent(const DBContentType aType, DBConnection_sptr aDBConn = nullptr)
  : _ContentType(aType),
    _DBConn(aDBConn)
  { }
  
public:
    DBContent( const DBContent &obj ):
        _ContentType(obj._ContentType),
        _DBConn( obj._DBConn )
    {}

    DBContentType GetContentType() const { return _ContentType; }
    DBConnection_sptr GetDBConnection() { return _DBConn; }

  virtual ~DBContent()
  {
  }
  
};

/**
 * A container for Database requests.
 * This class can contain either a straight SQL query or paramerterized query.
 */
class DBReqContent : public DBContent
{

private:
    std::string _SqlStmt;      //: A SQL statement, if it is to be parameterized see official documentation for formatting.

    int    _ParamIndex;   //: Helper variable, tracks the current index in building the parameter list.
    int    _NumParams;    //: The number of parameters defined.
    char  **_ParamValues;  //: An array of pointers to parameters, if text acts as a c_str otherwise treated as binary.
    int    *_ParamSizes; //:
    int    *_ParamFormats; //: An array of formats for the parameterization : 0 - text, 1 - binaray

public:

    /** @brief Creates a Request for a plain database query, not recommended for sql injection risks.
     *
     * @param[in] aSqlStmt The SQL Statement, assumed to have no parameters.
     * @param[in] aDBConn The connection to the database, doesn't create a new connection if a nullptr is supplied.
     */
    DBReqContent( std::string aSqlStmt, DBConnection_sptr aDBConn = nullptr)
    : DBContent(CSM_DB_REQ, aDBConn ),
        _SqlStmt(aSqlStmt),
        _ParamIndex(0),
        _NumParams(0),
        _ParamValues(nullptr),
        _ParamSizes(nullptr),
        _ParamFormats(nullptr)
    {}

    /** @brief Creates a Request for a parameterized database query, recommended for safety and code readability.
     *
     * @param[in] aSqlStmt The parameterized SQL statement. Follows the official Postgresql documentation.
     * @param[in] numParams The number of parameters expected for the query.
     * @param[in] aDBConn  The connection to the database, doesn't create a new connection if a nullptr is supplied.
     *
     */
    DBReqContent( 
        std::string aSqlStmt, 
        int numParams, 
        DBConnection_sptr aDBConn = nullptr) : DBContent(CSM_DB_REQ, aDBConn),
        _SqlStmt(aSqlStmt),
        _ParamIndex(0),
        _NumParams(numParams)
    {
        // Initialize the arrays.
        _ParamValues  = (char **)calloc( numParams, sizeof(char*));
        _ParamFormats =    (int*)calloc( numParams, sizeof(int)  );
        _ParamSizes   =    (int*)calloc( numParams, sizeof(int)  );
    }

    DBReqContent( const DBReqContent &obj );


    /** @brief Frees the parameters if specified.
     */
    virtual ~DBReqContent();

    /** @brief Adds a numeric parameter to the parameter list.
     * 
     * If this is built for use on a little endian system this function will use the hotbe byte swap.
     * Postgresql assumes Big Endian for numeric values.
     *
     * @param[in] number The number to register in the parameter list.
     *
     */
    template<typename T>
    void AddNumericParam( T number )
    {
        if ( _NumParams != 0  && _ParamIndex < _NumParams )
        {
            _ParamValues[_ParamIndex] =(char*) malloc(sizeof(T));
    
            #ifdef __LITTLE_ENDIAN__
            switch(sizeof(T))
            {
                case 2:
                {
                    uint16_t beNum = htobe16((uint16_t)number);
                    memcpy(_ParamValues[_ParamIndex],&beNum, sizeof(T));
                    break;
                }
                case 4:
                {
                    uint32_t beNum = htobe32((uint32_t)number);
                    memcpy(_ParamValues[_ParamIndex], &beNum, sizeof(T));
                    break;
                }
                case 8:
                {
                    uint64_t beNum = htobe64((uint64_t)number);
                    memcpy(_ParamValues[_ParamIndex],&(beNum), sizeof(T));
                    break;
                }
                default:
                    memcpy(_ParamValues[_ParamIndex],&(number), sizeof(T));
            }
            #else
            memcpy(_ParamValues[_ParamIndex], &(number), sizeof(T));
            #endif
            
            _ParamFormats[_ParamIndex] = 1; // binary field.
            _ParamSizes[_ParamIndex]   = sizeof(T);
    
            _ParamIndex++;
        }
    }
    
    /** @brief Adds an array of numeric types to the parameter list.
     *
     * @param[in] elements An array of numbers.
     * @param[in] size The size of the array of numbers.
     */
    template<typename T>
    void AddNumericArrayParam( T* nums, uint32_t size )
    {
        if ( _NumParams != 0  && _ParamIndex < _NumParams )
        {
            std::string arrayString = "{";

            if( nums )
            {
                for ( uint32_t i = 0; i < size; )
                {
                    arrayString.append(std::to_string(nums[i]));
                    if ( ++i < size ) arrayString.append(",");
                }
            }
            arrayString.append("}");

            _ParamValues[_ParamIndex]  = (char*)strdup(arrayString.c_str() );
            _ParamFormats[_ParamIndex] = 0;

            // length is not needed, but used for the copy constructor.
            _ParamSizes[_ParamIndex]   = strlen(_ParamValues[_ParamIndex])  + 1;
            _ParamIndex++;
        }
    }

    /** @brief Adds a character parameter to the parameter list.
     * 
     * Mallocs a character to stash the boolean.
     * Use this function for boolean values as well.
     *
     * @param[in] state The boolean value to store for the current parameter in the order.
     */
    void AddCharacterParam( char state );

    /** @brief Inserts a text field into the parameter set. 
     *
     * Format : 0
     * Type   : TEXTOID
     *
     * @param[in] text The text parameter, duplicated through strdup.
     * 
     */
    void AddTextParam( const char* text );

    /** @brief Inserts a fixed width character text field into the parameter set. 
     *
     * Format : 0
     * Type   : TEXTOID
     *
     * @param[in] text The text parameter, duplicated through memcpy.
     * @param[in] size The size of the text, for text arrays, defaults to zero.
     */
    void AddTextParam( const char* text, int size );

    /** @brief Adds an array of c strings to the parameter list.
     *
     * @param[in] elements An array of c strings.
     * @param[in] size The size of the array of c strings.
     */
    void AddTextArrayParam( const char* const * elements, uint32_t size );


    /** @defgroup Getters_and_Setters
     * @{*/
    std::string GetSqlStmt() { return _SqlStmt; }
    
    int GetNumParams() { return _NumParams; }

    char** GetParamValues() { return _ParamValues; }

    int* GetParamFormats() { return _ParamFormats; }
    int* GetParamSizes()   { return _ParamSizes; } 
    /** @} */
};

/** @brief Override of the ostream for the DBReqContent, outputs the content type.
 */
template<class stream>
static stream&
operator<<( stream &out, const csm::db::DBReqContent &req )
{
    out << dynamic_cast<const csm::db::DBContent&>(req).GetContentType();
    return (out);
}



/** Holds a response from a postgresql database.
 */
class DBRespContent : public DBContent
{
public:
/*
  DBRespContent(DBResult *aDBResult)
  : DBContent(CSM_DB_RESP),
    _DBResult(aDBResult)
  { }
*/
  DBRespContent(DBResult_sptr aDBResult, DBConnection_sptr aDBConnection = nullptr )
  : DBContent(CSM_DB_RESP, aDBConnection ),
    _dbres(aDBResult)
  { }
    
    DBResult_sptr GetDBResult() const { return _dbres; };
  
    virtual ~DBRespContent()
    {
        //to-do: cannot clear the DBResult here. Need to be freed at EventHandler!
        //DB_ResClear(_DBResult);
    }
  
private:
    DBResult_sptr _dbres;
};

} //namespace db
} //namespace csm

#endif  // CSM_DAEMON_SRC_CSM_DB_EVENT_CONTENT_H_
