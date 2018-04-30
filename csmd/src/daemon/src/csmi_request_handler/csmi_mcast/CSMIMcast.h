/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcast.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**
 * @file CSMIAllocationMcast.h
 *
 * @author John Dunham (jdunham@us.ibm)
 */

#ifndef CSMI_ALLOCATION_MCAST
#define CSMI_ALLOCATION_MCAST

#include "csmnet/src/CPP/csm_message_and_address.h"
#include "../helpers/EventHelpers.h"
#include "../csmi_handler_context.h"
#include <string>
#include <vector>
#include <map>

/**
 * @brief A class defining the properties of a multicast.
 * 
 * @tparam DataStruct The type of the struct stored in @ref _Data, assumed to have the following 
 *  fields: compute_nodes ( char ** ) and num_nodes ( uint32_t ).
 */
template<typename DataStruct>
class CSMIMcast
{
protected:
    std::map<std::string, std::pair<int,uint32_t>> _NodeStates; /**< The mapping of node to current 
                                                            state and node index respectively. */

    uint8_t     _CommandType;   /**< The type of the command for the multicast. */

    bool        _Create;        /**< Flag for whether create or delete logic should be used.*/
    bool        _Recovery;      /**< Flag indicating that the multicast is in recovery.*/
    bool        _IsAlternate;   /**< Flag indicating that the multicast must take an alternate path 
                                   of execution.*/
    bool        _EEReply;       /**< Flag indicating that in the early exit condition should be a 
                                   reply to the front end.*/
    bool        _RASPushed;     /**< Flag indicating that the RAS event was pushed (usually not tripped).*/
    DataStruct* _Data;          /**< The data struct containing details about the multicast. */

    std::string _RASMsgId;      /**< The RAS message id for failed multicasts.  */

public:
    /**@brief Initializes a Multicast property object.
     * @param[in] cmdType The command type of the handler responsible for this class.
     * 
     * @param[in] data    A data struct containing details related to the properties
     * @param[in] create  Specify whether this defines properties for create or delete.
     * @param[in] eeReply Specify whether the early exit replies to the frontend.
     */
    CSMIMcast( uint8_t cmdType, 
               DataStruct* data     = nullptr, 
               bool create          = false,
               bool eeReply         = false,
               std::string rasMsgId = ""):
                _CommandType(cmdType), _Create(create),
                _Recovery(false),      _IsAlternate(false),
                _EEReply(eeReply),     _RASPushed(false),
                _Data(data),           _RASMsgId(rasMsgId){ }

    /**
     * @brief Invokes the free for allocation.
     * @note This is a pure virtual to force this into an abstract class.
     * @warning If implementations of this class don't free _Data, objects of this class will be a memory leak source.
     */
    ~CSMIMcast() {};
 
    /**
     * @brief Builds a vector of strings for the nodes defined in @ref _Data's compute_nodes field.
     *
     * @return A vector of strings with the list of nodes to multicast to.
     */
    inline std::vector<std::string> GetTargetListFromData()
    {
        // If the data struct was not null.
        std::vector<std::string> nodeList;
        if ( _Data && _Data->compute_nodes )
        {
            for( uint32_t i=0; i < _Data->num_nodes; ++i)
            {
                if ( _Data->compute_nodes[i] != 0 )
                {
                    std::string node( _Data->compute_nodes[i] );
                    
                    // Add the name to the list and initialize the map.
                    nodeList.push_back( _Data->compute_nodes[i] );
                    _NodeStates[node] = std::pair<int,uint32_t>(-1, i);
                }
            }
        }
        return nodeList;
    }

    /** @brief Updates the nodeStates map based on the recovery state.
     * 
     * If the _Recovery Flag is not set the contents of the map are initialized to -1.
     *
     * Otherwise the map values are set using in the following transition matrix:
     *
     * | Input Value | Exit Value | Description                       |
     * |-------------|------------|-----------------------------------|
     * | -1          | -3         | Node had timed out                |
     * | 0           | -1         | Node had no errors                |
     * | >0          | N/A        | Node reported an error, no change |
     *
     * Each value uses the function *x<=0; f(x)=(x*2)-1* to perform this transition.
     *
     * @return A list of the nodes present in the Map.
     */
    std::vector<std::string> UpdateNodeMap()
    {
        std::vector<std::string> nodeList;
    
        if(!_Recovery)
        {
            nodeList = GetTargetListFromData();
        }
        else
        {
            // Iterate over the map, grab the node name and update the value.
            for ( auto const& node : _NodeStates)
            {
                nodeList.push_back(node.first);
    
                if (node.second.first <= 0) _NodeStates[node.first].first = ( node.second.first * 2 ) -1;
            }
        }
    
        return nodeList;
    }

    /** @brief Sets the error code of a host name in the map.
     * 
     * @param[in] hostName The host name of the node to update the error code for.
     * @param[in] errorCode The new Error Code, this error code does not replace
     *                          the value if the current value is <= 0
     * @return The index of the hostname in the array that generated the mapping.
     */
    uint32_t SetHostError( std::string hostName, int errorCode = 0 )
    {
        int32_t hostIndex = UINT32_MAX;
        // If the hostname is present and doesn't have an error code, update the map.
        if ( _NodeStates.find(hostName) != _NodeStates.end() )
        {
            // If there was an older error, don't overwrite (timeout errors are fine to overwrite)
            if (_NodeStates[hostName].first <= 0 ) 
            {
                // If no error code was supplied simply add 1
                _NodeStates[hostName].first = errorCode == 0 ? 
                    _NodeStates[hostName].first + 1 : errorCode;
            }

            hostIndex = _NodeStates[hostName].second;
        }

        return hostIndex;
    }


    /**
     * @brief Generates a unique identifier for the multicast, using the included data.
     *
     * @return A string containing the unique identifier for the multicast.
     *
     * @warning This must be implemented in a specialization with the appropriate data.
     */
    std::string GenerateIdentifierString()
    {
        return "";
    }

    /** @brief Generates a string detailing any issues with the nodes in the multicast.
     *
     * | Error Code | Description             |
     * |------------|-------------------------|
     * | -3         | Node unreachable        |
     * | -2         | Node was only recovered |
     * | -1         | Node lost in last MTC   |
     * | >0         | Node returned an error  |
     *
     * @return A string containing the nodes that failed and their error codes, ':' delimited.
     */
    inline std::string GenerateErrorListing() const
    {
        std::string failures( "-3 : MTC unreachable twice | -2 : MTC unreachable on primary attempt | -1 : MTC unreachable on final attempt | >0 : Remote node reported an error; "  );
        int failureCount = 0;

        for ( auto const& node : _NodeStates )
        {
            if ( node.second.first != 0 )
            {
                failureCount++;
                failures.append(node.first).append(" : ");
                failures.append(std::to_string(node.second.first)).append(";");
            }
        }

        return failureCount > 0 ? failures : std::string("");
    }
            
    /**
     * @brief Generates a generic RAS event for timeouts.
     *
     * @param[in,out] postEventList The event queue to push the RAS message.
     * @param[in]     ctx The context object with the event handler.
     */
    inline void GenerateRASEvents( 
        std::vector<csm::daemon::CoreEvent*>& postEventList, 
        csm::daemon::EventContextHandlerState_sptr ctx) const
    {
        // EARLY RETURN if ras was already pushed.
        if ( _RASPushed || _RASMsgId == "") return;
        
        CSMI_BASE* handler = static_cast<CSMI_BASE*>(ctx->GetEventHandler());
        for ( auto const& node : _NodeStates )
        {
            if ( node.second.first < 0 )
            {
                csm::daemon::NetworkEvent *reply =
                    csm::daemon::helper::CreateRasEventMessage(_RASMsgId, node.first, "",
                        "rc=" +std::to_string(node.second.first), handler->GetAbstractMaster());

                if(reply)
                    postEventList.push_back(reply);
                else
                    LOG(csmapi, error) <<  "Unable to push RAS";
            }
        }
    }

    /**
     * @brief Prepares the properties object to create a recovery multicast (if supported).
     * Sets #_Recover to true and #_Create is negated.
     */
    inline void PrepRecovery()
    {
        _Recovery = true;
        _Create = !_Create;
    }

    /**@brief Initializes the payload of a multicast message.
     *
     * @param[out] buffer The payload of the message.
     * @param[out] bufferLength The length of the payload.
     *
     * @warning This must be implemented in a specialization with the appropriate data.
     */
    void BuildMcastPayload(char** buffer, uint32_t* bufferLength) 
    {
        *buffer = nullptr;
        *bufferLength = 0;
    }

    /** @brief Retrieves whether execution should take an alternate path, used mcast states.
     * @return Whether an alternate path should be taken.
     */
    inline bool IsAlternate() const { return _IsAlternate; };

    /** @brief Informs any states receiving the mcast props to take an alternate execution path.
     * @param alt Whether the state machine should take a registered alternate path.
     */
    inline void SetAlternate(bool alt)  { _IsAlternate = alt;};

    /**@brief Sets that this is a "create" multicast operation.
     * @param create Whether the state should be interpreted as a create operation.
     */
    inline void SetCreate( bool create) { _Create = create; };

    /**@brief Gets that this is a "create" multicast operation.
     * The definition of the "create" operation is implementation dependent.
     *
     * @return True if this is a "create" operation.
     */
    inline bool IsCreate( ) { return _Create; };

    /** @brief Gets whether or not the API should exit before a multicast.
     *  This is generally used for diagnostics or when the API is being used to stage in.
     *
     *  @return True if the API associated should return early.
     */
    inline bool DoesEarlyExitReply() const { return _EEReply; };

    /** @brief The command type for the multicast.
     *  This should be used to differentiate different multicasts.
     *  Should be something like:
     *      @ref CSM_CMD_allocation_create or @ref CSM_CMD_allocation_delete.
     *
     *  @return The command type of the 
     */
    inline uint8_t GetCommandType() const { return  _CommandType; };

    /** @brief Getter for the data stored by the properties object.
     *  @return A pointer to the data associated with the properties object.
     */
    inline DataStruct* GetData() const { return _Data; }
};


#endif
