/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_db_resp_state.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_DB_RESP_STATE_H__
#define __CSMI_DB_RESP_STATE_H__
#include "csmi_handler_state.h"


/** @brief The base class for states that expect DBResp events. */
class DBRespState: public CSMIHandlerState 
{
    
    using CSMIHandlerState::CSMIHandlerState;
    
public:
    /** @brief Invoked by the Handler's Process Function.
     *
     * Note: This is the finalization of the Process function.
     *
     * @param ctx The context of the event. Cast in the invoking function to 
     *  prevent every implementation of Process from having perform error checking.
     * @param aEvent The event that triggered this call.
     * @param postEventList The list of events to push new events to.
     */
    virtual void Process( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent, 
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final
    {
        LOG(csmapi, trace) << "DBRespState State ID: " << ctx->GetAuxiliaryId();
        
        // If the Content type of this event is not a DBResp, react.
        if ( !aEvent.HasSameContentTypeAs(csm::daemon::helper::DB_RESP_EVENT_TYPE) )
        {
            if ( aEvent.HasSameContentTypeAs(csm::daemon::helper::TIMER_EVENT_TYPE) )
            {
                HandleTimeout(ctx, aEvent, postEventList);
                return;
            }
            
            // TODO better error handling.
            ctx->SetErrorCode(CSMERR_BAD_EVENT_TYPE);
            ctx->AppendErrorMessage("Did not receive a DB_RESP_EVENT_TYPE");
            //HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
            return;
        }
    
        // Declarations:
        int err_code;               //< Error code produced in the execution of Process.
        std::string err_msg;        //< Error message produced in the execution of Process.
    
        // Handle a bad event.
        if ( !csm::daemon::helper::InspectDBResult(aEvent, err_code, err_msg ) )
        {
            ctx->SetErrorCode(CSMERR_DB_ERROR);
            ctx->SetDBErrorCode(err_code);
            ctx->SetErrorMessage("Database Error Message: " + err_msg);

            HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
            return;
        }
    
        // Initializations:
        csm::daemon::DBRespEvent *db_event = (csm::daemon::DBRespEvent *) &aEvent;
        csm::db::DBResult_sptr db_res = db_event->GetContent().GetDBResult(); 
        std::vector<csm::db::DBTuple *> tuples;                              
    
        if( db_res && !(csm::daemon::helper::GetTuplesFromDBResult(db_res, tuples)) )
        {
            ctx->SetErrorCode(CSMERR_DB_ERROR);
            //ctx->SetDBErrorCode(db_res.GetErrCode())
            ctx->SetErrorMessage("Unable to parse the tuples from the database.");
            HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
            return;
        }
        
        // Handle the response, but if an error happened deal with it.
        if ( !HandleDBResp(tuples, postEventList, ctx) )
            HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
        
        // Free the tuples.
        for (uint32_t i = 0; i < tuples.size(); i++)
            csm::db::DB_TupleFree(tuples[i]);
    }

protected:
    /** @brief Handles the parsing and event response to a DBResp.
     *  @note If an error occurs, place the error details in the ctx object and
     *      return false. Allow the process function to perform error handling.
     *  
     *  @param[in] content The message received by the CSMIHandlerState.
     *  
     *  @param[in] postEventList The Event Queue.  
     * @todo is the postEventList Thread-Safe?
     *  @param[in,out] ctx The context of the event, holds details persistent across states.
     *
     *  @return true Everything went as planned and no errors were found.
     *  @return false An error was detected and details were placed in the context.
     */
    virtual bool HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx )   = 0;
};

#endif


