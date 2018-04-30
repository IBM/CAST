/*================================================================================

    csmutil/include/timing.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifdef __cplusplus
#include <chrono>

//#define TIME_COUNT( time_point )
//    std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() 
#define TIME_COUNT( time_point )\
    time_point.time_since_epoch().count() 

#define START_TIMING(  ) \
    auto start = std::chrono::system_clock::now();

/**
 * @brief Generates a csv string representing the timing of the section.
 * 
 * @param[in] target_log The target for the log (e.g. csmapi)
 * @param[in] log_level The log level that this trace should occur on.
 * @param[in] run_id The run id for the profile target (allows profile runs to be collated).
 * @param[in] api_id The api this profile was a snapshot of.
 * @param[in] context_str The context of this particular run (e.g. state 1 or api)
 *
 * @returns
 *      | Run ID        | API ID                     | Context                                           | Start Time              | End Time             | Run Time                                    |
 *      |---------------|----------------------------|---------------------------------------------------|-------------------------|----------------------|---------------------------------------------|
 *      | The trace id. | The Identifier for the API | The context of the profile (typically the state). | The start time ( epoch) | The end time (epoch) | The difference between  start and end time. |
 *      
 */
#define END_TIMING( target_log, log_level, run_id, api_id, context_str )            \
    auto end = std::chrono::system_clock::now();                                    \
                                                                                    \
    LOG( target_log, log_level ) << "TIMING: " << run_id << "," << api_id << "," << \
        context_str << "," << TIME_COUNT(start) << "," << TIME_COUNT(end) << "," << \
        (end - start).count();

#define START_TIMING_CUSTOM( var ) \
    auto start_#var = std::chrono::system_clock::now();

#define END_TIMING_CUSTOM( target_log, log_level, run_id, api_id, context_str, var ) \
    auto end_time = std::chrono::system_clock::now();                               \
                                                                                    \
    LOG( target_log, log_level ) << "TIMING: " << run_id << "," << api_id << "," << \
        context_str << "," << TIME_COUNT(start_##var) << "," <<                     \
        TIME_COUNT(end_##var) << "," << (end_##var - start_##var).count();
#else
#include <time.h>

#define SECOND_OFFSET 1000000000L

#define START_TIMING(  )                    \
    struct timespec start, end;             \
    clock_gettime( CLOCK_REALTIME, &start );

/**
 * @brief Generates a csv string representing the timing of the section.
 * 
 * @param[in] target_log The target for the log (e.g. csmapi)
 * @param[in] log_level The log level that this trace should occur on.
 * @param[in] run_id The run id for the profile target (allows profile runs to be collated).
 * @param[in] api_id The api this profile was a snapshot of.
 * @param[in] context_str The context of this particular run (e.g. state 1 or api)
 *
 * @returns
 *      | Run ID        | API ID                     | Context                                           | Start Time              | End Time             | Run Time                                    |
 *      |---------------|----------------------------|---------------------------------------------------|-------------------------|----------------------|---------------------------------------------|
 *      | The trace id. | The Identifier for the API | The context of the profile (typically the state). | The start time ( epoch) | The end time (epoch) | The difference between  start and end time. |
 *      
 */
#define END_TIMING( target_log, log_level, run_id, api_id, context_str ) \
    clock_gettime( CLOCK_REALTIME, &end );                               \
                                                                         \
    csmutil_logging( log_level, "TIMING: %llu,%d," #context_str ",%llu,%llu,%llu", \
         run_id, api_id, SECOND_OFFSET * start.tv_sec + start.tv_nsec,            \
         SECOND_OFFSET * end.tv_sec + end.tv_nsec,                                \
         SECOND_OFFSET * ( end.tv_sec - start.tv_sec ) + ( end.tv_nsec - start.tv_nsec));

#define START_TIMING_CUSTOM( var ) \
    struct timespec var##_start, var##_end;\
    clock_gettime( CLOCK_REALTIME, &var##_start );

#define END_TIMING_CUSTOM( target_log, log_level, run_id, api_id, context_str, var ) \
    clock_gettime( CLOCK_REALTIME, &var##_end );                               \
    csmutil_logging( log_level, "TIMING: %llu,%d," #context_str ",%llu,%llu,%llu", \
         run_id, api_id, SECOND_OFFSET * var##_start.tv_sec + var##_start.tv_nsec,            \
         SECOND_OFFSET * var##_end.tv_sec + var##_end.tv_nsec,                                \
         SECOND_OFFSET * ( var##_end.tv_sec - var##_start.tv_sec ) + ( var##_end.tv_nsec - var##_start.tv_nsec));


#endif
