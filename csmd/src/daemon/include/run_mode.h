/*================================================================================

    csmd/src/daemon/include/run_mode.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_RUN_MODE_H_
#define CSMD_SRC_DAEMON_INCLUDE_RUN_MODE_H_

#include <signal.h>
#include <mutex>

#ifndef logprefix
#define logprefix "RUNMODE"
#define logprefix_local
#endif
#include "csm_pretty_log.h"

#include "include/csm_daemon_exception.h"
#include "csm_daemon_role.h"

namespace csm {
namespace daemon {

typedef enum {
  REASON_UNSPEC = 0,     // unspecified reason or not important
  REASON_TRANSITION,     // general, default reason to transition to next state
  REASON_ERROR,          // reason for state transition was an error
  REASON_DISCONNECT,     // a redundant connection got disconnected
  REASON_CONNECT,        // a redundant connection got re-/established
  REASON_CONFIG,         // reason for state transition was a configuration change
  REASON_EXIT,            // reason to signal that we're going to exit
  REASON_JOB             // reason for state transition was job start or end
} run_mode_reason_t;

static inline
std::string runmode_reason_to_string( const csm::daemon::run_mode_reason_t aReason )
{
  switch( aReason )
  {
    case REASON_UNSPEC: return "UNSPECIFIED";
    case REASON_TRANSITION: return "TRANSITION";
    case REASON_ERROR: return "ERROR";
    case REASON_DISCONNECT: return "DISCONNECT";
    case REASON_CONNECT: return "CONNECT";
    case REASON_CONFIG: return "CONFIGURATION";
    case REASON_EXIT: return "EXIT";
    case REASON_JOB: return "JOB Start|End";
    default: return "UNRECOGNIZED TRANSITION REASON";
  }
}

namespace RUN_MODE {

typedef enum
{
  STARTED = 0,   ///< daemon started but not configured
  CONFIGURED,    ///< daemon started and configuration parsed properly
  DISCONNECTED,  ///< daemon got disconnected/not yet connected
  READY_RUNNING, ///< normal operation
  CLEANUP,       ///< daemon is cleaning up any state, connections, etc. to prepare a reset
  EXIT,           ///< daemon exiting
// compute node specific modes:
  READY_RUNNING_JOB,   ///< compute daemon is running a job: needs to observe jitter; aggregator shouldn't send stuff
  PARTIAL_CONNECT,    ///< compute daemon looses aggregator connection while operating in running mode
  UNSPECIFIED     ///< any case where the runmode is unclear - this should cause an error for the caller
} mode_t;

static inline
std::string to_string( const csm::daemon::RUN_MODE::mode_t aMode )
{
  switch( aMode )
  {
    case csm::daemon::RUN_MODE::STARTED: return "RUNMODE_STARTED";
    case csm::daemon::RUN_MODE::CONFIGURED: return "RUNMODE_CONFIGURED";
    case csm::daemon::RUN_MODE::DISCONNECTED: return "RUNMODE_DISCONNECTED";
    case csm::daemon::RUN_MODE::READY_RUNNING: return "RUNMODE_READY_RUNNING";
    case csm::daemon::RUN_MODE::CLEANUP: return "RUNMODE_CLEANUP";
    case csm::daemon::RUN_MODE::EXIT: return "RUNMODE_EXIT";
    case csm::daemon::RUN_MODE::READY_RUNNING_JOB: return "RUNMODE_READY_RUNNING_JOB";
    case csm::daemon::RUN_MODE::PARTIAL_CONNECT: return "RUNMODE_PARTIAL_CONNECT";
    default: return "UNRECOGNIZED RUNMODE";
  }
}

}   // namespace RUN_MODE

class RunMode
{
  volatile sig_atomic_t _KeepRunning;
  volatile RUN_MODE::mode_t _RunMode;
  std::mutex _RunModeLock;
  volatile run_mode_reason_t _Reason;
  int _ErrorCode;
  enum CSMDaemonRole _Role;

public:
  RunMode( const RUN_MODE::mode_t i_Mode = RUN_MODE::STARTED )
  : _KeepRunning( 1 ),
    _RunMode( i_Mode ),
    _Reason( REASON_TRANSITION ),
    _ErrorCode( 0 )
  {
    _Role = CSM_DAEMON_ROLE_UNKNOWN;
  }
  RunMode& operator=( const RunMode &i_Other )
  {
    _KeepRunning = i_Other._KeepRunning;
    _RunMode = i_Other._RunMode;
    _Reason = i_Other._Reason;
    return *this;
  }
  inline RUN_MODE::mode_t Get() const { return _RunMode; }
  inline void SetReason( const csm::daemon::run_mode_reason_t i_Reason ) { _Reason = i_Reason; }
  inline csm::daemon::run_mode_reason_t GetReason() const { return _Reason; }
  inline bool KeepRunning() const { return _KeepRunning != 0; }
  inline void StopRunning() { _KeepRunning = 0; }
  inline void ResetErrorCode() { _ErrorCode = 0; }
  inline int GetErrorCode() const { return _ErrorCode; }
  inline void SetRole( const CSMDaemonRole aRole ) { _Role = aRole; }
  inline bool IsARunningMode() const
  {
    return ((_RunMode == csm::daemon::RUN_MODE::READY_RUNNING) ||
        (_RunMode == csm::daemon::RUN_MODE::READY_RUNNING_JOB) ||
        (_RunMode == csm::daemon::RUN_MODE::PARTIAL_CONNECT));
  }

  void Transition( const run_mode_reason_t aReason = run_mode_reason_t::REASON_UNSPEC,
                   const int aErrorCode = 0 )
  {
    std::lock_guard<std::mutex> Guard( _RunModeLock );
    csm::daemon::run_mode_reason_t InputReason = aReason == run_mode_reason_t::REASON_UNSPEC ? _Reason : aReason;
    csm::daemon::RUN_MODE::mode_t oldMode = Get();

    // preserve any existing error code. Explicit reset required via ResetErrorCode()
    if( aErrorCode != 0 )
      _ErrorCode = aErrorCode;

    // Exit if we're no longer supposed to keep running
    if( ! _KeepRunning )
    {
      CSMLOG( csmd, debug ) << "Transition to exit. KEEPRUNNING=false. ";
      InputReason = REASON_EXIT;
    }

    if( InputReason == REASON_UNSPEC )
      throw csm::daemon::Exception("BUG: Invalid RunMode transition.");

    // transitions:
    switch( Get() )
    {
      case RUN_MODE::STARTED:
        switch( InputReason )
        {
          case REASON_TRANSITION:  SetRunMode( RUN_MODE::CONFIGURED ); break;
          default: SetRunMode( RUN_MODE::CLEANUP ); break;
        }
        break;

      case RUN_MODE::CONFIGURED:
        switch( InputReason )
        {
          case REASON_TRANSITION: SetRunMode( RUN_MODE::DISCONNECTED ); break;
          case REASON_ERROR: SetRunMode( RUN_MODE::CLEANUP ); break;  // TODO: Should become retry later!
          case REASON_EXIT:
          default: SetRunMode( RUN_MODE::CLEANUP ); break;
        }
        break;

      case RUN_MODE::DISCONNECTED:
        switch( InputReason )
        {
          case REASON_CONFIG: CSMLOG( csmd, error ) << "Reconfigure not implemented yet."; break;
          case REASON_TRANSITION: SetRunMode( RUN_MODE::READY_RUNNING ); break; // all connectivity established
          case REASON_ERROR:      SetRunMode( RUN_MODE::DISCONNECTED ); break; // unable to connect yet
          case REASON_DISCONNECT: break;  // unusual, but if it happens, we better stay disconnected
          case REASON_CONNECT:
            if( _Role == CSM_DAEMON_ROLE_AGENT )
              SetRunMode( RUN_MODE::PARTIAL_CONNECT );  // compute started already with only one aggregator
            else
              SetRunMode( RUN_MODE::READY_RUNNING );
            break;
          case REASON_EXIT:
          default: SetRunMode( RUN_MODE::CLEANUP ); break;
        }
        break;

      case RUN_MODE::READY_RUNNING:
      case RUN_MODE::READY_RUNNING_JOB:
        switch ( InputReason )
        {
          case REASON_ERROR:      SetRunMode( RUN_MODE::DISCONNECTED ); break;
          case REASON_JOB:        SetRunMode( RUN_MODE::READY_RUNNING ); break;
          case REASON_DISCONNECT:
            if( _Role == CSM_DAEMON_ROLE_AGENT )
              SetRunMode( RUN_MODE::PARTIAL_CONNECT ); // compute left with one aggregator
            else
              SetRunMode( RUN_MODE::READY_RUNNING );
            break;
          case REASON_CONNECT:
            CSMLOG( csmd, warning ) << "Unexpected run-mode transition. CONNECT while in fully connected mode. Ignoring...";
            break;
          case REASON_TRANSITION: break; // do nothing if we just transition
          case REASON_CONFIG: CSMLOG( csmd, error ) << "Reconfigure not implemented yet."; break;
          case REASON_EXIT:
          default: SetRunMode( RUN_MODE::CLEANUP );  break;
        }
        break;

      case RUN_MODE::PARTIAL_CONNECT:
        if( _Role != CSM_DAEMON_ROLE_AGENT )
          throw csm::daemon::Exception("BUG: only compute daemons should be able to enter PARTIAL_CONNECT mode.");
        switch( InputReason )
        {
          case REASON_TRANSITION: break; // do nothing if we're in regular transition
          case REASON_ERROR:      SetRunMode( RUN_MODE::DISCONNECTED ); break;
          case REASON_JOB:        SetRunMode( RUN_MODE::PARTIAL_CONNECT ); break;
          case REASON_DISCONNECT: SetRunMode( RUN_MODE::DISCONNECTED ); break;
          case REASON_CONNECT:    SetRunMode( RUN_MODE::READY_RUNNING ); break;
          case REASON_EXIT:
          case REASON_CONFIG:
          default: SetRunMode( RUN_MODE::CLEANUP );  break;
        }
        break;
      case RUN_MODE::CLEANUP:
        switch( InputReason )
        {
          case REASON_CONFIG:  SetRunMode( RUN_MODE::STARTED ); break;
          default: SetRunMode( RUN_MODE::EXIT ); break;
        }
        break;

      case RUN_MODE::EXIT:
        if(( InputReason == REASON_EXIT )||( InputReason == REASON_TRANSITION ))
          break;
        CSMLOG( csmd, warning ) << "Invalid transition while in EXIT mode. Reason=" << runmode_reason_to_string( InputReason );
        break;
      default:
        throw csm::daemon::Exception("Invalid RunMode during mode transition!");
    }
    _Reason = InputReason;
    if( oldMode != Get() )
      CSMLOG( csmd, info ) << "Transition from " << csm::daemon::RUN_MODE::to_string( oldMode )
        << " to: " << csm::daemon::RUN_MODE::to_string( Get() )
        << " Reason: " << runmode_reason_to_string( InputReason );
  }

private:
  inline void SetRunMode( const RUN_MODE::mode_t i_NewMode ) { _RunMode = i_NewMode; }
};

}  // namespace daemon
} // namespace csm

/** \brief output operator for address type */
template<class stream>
static stream&
operator<<( stream &out, const csm::daemon::RUN_MODE::mode_t &i_Mode )
{
  out << csm::daemon::RUN_MODE::to_string( i_Mode );
  return (out);
}

#ifdef logprefix_local
#undef logprefix
#undef logprefix_local
#endif

#endif /* CSMD_SRC_DAEMON_INCLUDE_RUN_MODE_H_ */
