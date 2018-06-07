/*================================================================================

    csmi/include/csm_api_inventory.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_INVENTORY_H__
#define __CSM_API_INVENTORY_H__

#include <stdint.h>
#include "csmi_type_inv_funct.h"

#ifdef __cplusplus
extern "C" {    
#endif

/** @file csm_api_inventory.h
 *  @brief Function prototypes and structures for @ref inv_apis
 *
 *  This contains the prototypes and struct definitions for the CSM Inventory
 *  APIs and any macros, constants, or global variables you will need.
 *
 *	@author Nick Buonarota (nbuonar@us.ibm.com)
 *  @author Jon Cohn (jcohn@us.ibm.com)
 *  @author John Dunham (jdunham@us.ibm.com)
 */

/** @ingroup inv_apis 
 * @brief Used by someone or some application that wants to know everything there is to know about a node(s).
 *
 * Retrieves information about the nodes specified in the node_names field of @p input from the *csm_node* table in the
 * CSM Database. The results field of @p output will be populated with the records
 * retrieved from the SQL query.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo dataCount being an in/out might be a design error, from a clarity perspective.
 *
 * ## Database Tables ##
 * Table    | Description 
 * ---------|-------------
 * csm_node | Aggregation of the nodes in the cluster.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_node_attributes_query API.
 * @param[out] output Used to contain the output parameters for the csm_node_attributes_query API, consult @ref csm_node_attributes_query_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_node* table.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_attributes_query(csm_api_object **handle,
                              csm_node_attributes_query_input_t* input,
							  csm_node_attributes_query_output_t** output);


/** @ingroup inv_apis 
 * @brief Used by someone or some application that wants to know everything there is to know about all the nodes in the system.
 *
 * Retrieves information about all the nodes in the CSM Database *csm_node* table.
 * The @p node_attributes array will be populated with the contents of the rows 
 * retrieved and @p data_count will be populated with the size of this array. In the event of
 * an error @p data_count will be zero and @p node_attributes will be null.
 *
 * @p node_attributes must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo CSMI_NO_RESULTS
 *
 * @todo Should the order of the function definition be swapped? The order is swapped with @ref csm_node_attributes_query.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *             csm_node | Aggregation of the nodes in the cluster.
 *
 * @param[out]             handle An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[out]         data_count The number of nodes in @p node_attributes.
 *                                  In the event of an error this will be set to zero.
 * @param[out]    node_attributes The results of the query, each index corresponds to an entry
 *                                  in the *csm_node* table, null in the event of an error.
 *                                  Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_node* table.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_attributes_query_all(csm_api_object **handle); 

/**
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 */
int csm_node_attributes_query_details(csm_api_object **handle,
										csm_node_attributes_query_details_input_t* input,
										csm_node_attributes_query_details_output_t** output);

/** @ingroup inv_apis 
 * @brief Used by someone or some application to query the database for the attribute history of a single node.
 *
 * Retrieves the attribute history of the node specified by @p node_name in the cluster in the 
 * CSM Database *csm_node_history* table. The @p node_history array will be populated with all
 * the entries matching the specified node and @p data_count will contain the number of entries
 * found. In the event of an error @p data_count will be zero and @p node_history will null.
 *
 * @p node_history must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo CSMI_NO_RESULTS
 *
 * @todo Should the order of the function definition be swapped? The order is swapped with @ref csm_node_attributes_query.
 *
 * ## Database Tables ##
 * Table            | Description 
 * -----------------|-------------
 * csm_node_history | Tracks historic states of nodes in the cluster.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_node_attributes_query_history API.
 * @param[out] output Used to contain the output parameters for the csm_node_attributes_query_history API, 
 *                      consult @ref csm_node_attributes_query_history_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_node_history* table.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_attributes_query_history(csm_api_object **handle,
                                      csm_node_attributes_query_history_input_t* input,
									  csm_node_attributes_query_history_output_t** output);
									  
/** @ingroup inv_apis 
 * @brief Used by someone or some application to query the database for the state history of a single node over its lifetime.
 *
 * gets the state history of a node over its lifetime in CSM system. 
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo CSMI_NO_RESULTS
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_node_query_state_history API.
 * @param[out] output Used to contain the output parameters for the csm_node_query_state_history API, 
 *                      consult @ref csm_node_query_state_history_outnput_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the database.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_query_state_history(csm_api_object **handle,
                                      csm_node_query_state_history_input_t* input,
									  csm_node_query_state_history_output_t** output);

/** @ingroup inv_apis 
 * @brief Used by the CSM daemons to edit attributes of a node in the "csm_node" table of the CSM Database.
 *
 * Updates an existing node entry in the CSM Database *csm_node* table using the contents of the 
 * @p input struct. For details on configuration consult @ref csm_node_attributes_update_input_t.
 * After executing the update @p output will be populated with the status of the query.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *             csm_node | Aggregation of the nodes in the cluster.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]   input A configuration struct containing the fields that may be updated in the query.
 *                      Consult @ref csm_node_attributes_update_input_t for usage.
 * @param[out] output A record of the status of the update, in the event of a failure, the nodes 
 *                      that were not updated will be recorded here.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully updated the *csm_node* table.
 * @ref CSMERR_UPDATE_MISMATCH  | The number of nodes updated did not match the supplied count.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_attributes_update(csm_api_object **csm_obj,
                               csm_node_attributes_update_input_t* input, 
                               csm_node_attributes_update_output_t** output);
							   
/** @ingroup inv_apis 
 * @brief Used by the system administrator to remove a node(s) from the CSM database.
 *
 * Removes nodes specified in the node_names field of @p input from the *csm_node* table in the
 * CSM Database. Also removes all related table information on these nodes. For example, GPU 
 * and SSD inventory. A node can not be deleted if it is in use. For example, a part of an active 
 * allocation. @p output will be populated with the results of the deletion.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 * ## Database Tables ##
 * Table         | Description 
 * --------------|-------------
 * csm_node      | Aggregation of the nodes in the cluster.
 * csm_dimm      |
 * csm_gpu       | 
 * csm_hca       | 
 * csm_processor | 
 * csm_ssd       | 
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_node_delete API.
 * @param[out] output Output of the API, in the event of a failure, the nodes 
 *                      that were not deleted will be recorded here.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully deleted the specified nodes in the CSM database.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_DEL_MISMATCH     | Number of deleted records is less than expected.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_delete(csm_api_object **handle,
                        csm_node_delete_input_t* input,
						csm_node_delete_output_t** output);

/** @ingroup inv_apis 
 * @brief Used by the system administrator to get all current information on a list of switches.
 *
 * Retrieves information about the switches specified in @p input from the *csm_switch* table in the
 * CSM Database. The @p output struct will be populated with the contents of the rows 
 * retrieved in the event of an error @p output will be null.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo add multi switch support.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *           csm_switch | Aggregation of the switches in the cluster.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]   input Specifies the switches to retrieve from the *csm_switch* table.
 * @param[out] output The results of the query, consult @ref csm_switch_attributes_query_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_switch* table.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_switch_attributes_query(csm_api_object **handle, 
            csm_switch_attributes_query_input_t* input, 
            csm_switch_attributes_query_output_t** output);

/** @ingroup inv_apis 
 * @brief Used by the system administrator to get all current information on a switch and its related children tables.
 *
 * Retrieves information about the switch specified in @p input from the *csm_switch*, *csm_switch_inventory*, and *csm_switch_ports* tables in the
 * CSM Database. The @p output struct will be populated with the contents of the rows 
 * retrieved in the event of an error @p output will be null.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *
 *  ## Database Tables ##
 *  Table                | Description 
 *  ---------------------|-------------
 *  csm_switch           | Aggregation of the switches in the cluster.
 *  csm_switch_inventory | contains information about the switch inventory
 *  csm_switch_ports     | contains information about the switch ports
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]   input Specifies the switch to retrieve from the *csm_switch*, *csm_switch_inventory*, and *csm_switch_ports* tables.
 * @param[out] output The results of the query, consult @ref csm_switch_attributes_query_details_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns 
 *                        Error Code | Description
 *                        -----------|-------------
 *      @ref CSMI_SUCCESS            | Successfully queried the *csm_switch* table.
 *      @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 *      @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 *      @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 *      @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_switch_attributes_query_details(csm_api_object **handle, 
        csm_switch_attributes_query_details_input_t* input, 
		csm_switch_attributes_query_details_output_t** output);

/** @ingroup inv_apis 
 * @brief Used by the system administrator get the attribute history of a specific switch.
 *
 * Retrieves information about the switch specified in @p input from the *csm_switch_history* 
 * table in the CSM Database. The @p output struct will be populated with the contents of the rows 
 * retrieved in the event of an error @p output will be null.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *   csm_switch_history | Aggregation of the historic states of switches in the cluster.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]   input Specifies the switch to retrieve from the *csm_switch_history* table.
 * @param[out] output The results of the query, consult @ref csm_switch_attributes_query_history_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_switch_history* table.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_switch_attributes_query_history (csm_api_object **handle, 
                                         csm_switch_attributes_query_history_input_t* input, 
                                         csm_switch_attributes_query_history_output_t** output);

/** @ingroup inv_apis
 * @brief Used by the system administrator to update records in the "csm_switch" table of the CSM database.
 *
 * Updates an existing switch entry in the CSM Database *csm_switch* table using the contents of the 
 * @p input struct. For details on configuration consult @ref csm_node_attributes_update_input_t.
 * After executing the update @p output will be populated with the status of the query. 
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo document interactions with history table?
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *           csm_switch | Aggregation of the switches in the cluster.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]   input A configuration struct containing the fields that may be updated in the query.
 *                      Consult @ref csm_switch_attributes_update_input_t for usage.
 * @param[out] output A record of the status of the update, in the event of a failure, the switches
 *                      that were not updated will be recorded here.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully updated the *csm_switch* table.
 * @ref CSMERR_UPDATE_MISMATCH  | The number of switches updated did not match the supplied count.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_switch_attributes_update (csm_api_object **handle, 
                                  csm_switch_attributes_update_input_t* input, 
                                  csm_switch_attributes_update_output_t* *output);

/** @ingroup inv_apis undefined
 * @brief Used by the system admin to get the environmental data of a list of switches.
 *
 * @param handle A pointer to return a pointer to internally managed api data.
 * @param switchList A pointer to a list of switch names. The api will return the environmental data of all switches in this list.
 * @param dataCount A pointer to the number of entries in the *switchList[] array.
 * @param envData A pointer to an array of "csmi_switch_env_data_t" structs. On a successful call, there will be an entry in this 
 * 					array for each entry in the "*switchList[]" parameter.
 *
 * @return 0 Success
 * @return -1 Error: use "csm_api_object_errcode_get" and "csm_api_object_errmsg_get" 
 *					to retrieve additional information.
 */
int csm_switch_env_data_query (csm_api_object **handle, const char *switchList[], uint32_t *dataCount, csmi_switch_env_data_t *envData[]);

/** @ingroup inv_apis
 * @brief Used to collect switch inventory and insert into the 'csm_ib_cable' table of the CSM database.
 *
 * populates switch inventory.
 * 
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table   | Description 
 *           -------------|-------------
 *           csm_switch   | Contains information about the switch.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_ib_cable_inventory_collection API.
 * @param[out] output Used to contain the output parameters for the csm_ib_cable_inventory_collection API, consult @ref csm_ib_cable_query_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code          | Description
 * --------------------|-------------
 * @ref CSMI_SUCCESS   | Successfully inserted inventory into the *csm_switch* table.
 * @ref CSMERR_GENERIC | Default error case.
 */
int csm_switch_inventory_collection(csm_api_object **handle, csm_switch_inventory_collection_input_t* input, csm_switch_inventory_collection_output_t* *output);

/** @ingroup inv_apis
 * @brief Used to collect switch sub inventory and insert into the 'csm_switch_inventory' table of the CSM database.
 *
 * populates switch sub inventory.
 * 
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table   | Description 
 *           -------------|-------------
 *           csm_switch_inventory   | Contains information about the inventory on a switch.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the API.
 * @param[out] output Used to contain the output parameters for the API, consult @ref csm_switch_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns 
 * Error Code          | Description
 * --------------------|-------------
 * @ref CSMI_SUCCESS   | Successfully inserted inventory into the *csm_switch_inventory* table.
 * @ref CSMERR_GENERIC | Default error case.
 */
int csm_switch_children_inventory_collection(csm_api_object **handle, csm_switch_inventory_collection_input_t* input, csm_switch_children_inventory_collection_output_t* *output);

/** @ingroup inv_apis
 * @brief Used to collect ib cable inventory and insert into the 'csm_ib_cable' table of the CSM database.
 *
 * populates ib cable inventory and tables.
 * 
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table   | Description 
 *           -------------|-------------
 *           csm_ib_cable | Contains information about the Infiniband cable.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_ib_cable_inventory_collection API.
 * @param[out] output Used to contain the output parameters for the csm_ib_cable_inventory_collection API, consult @ref csm_ib_cable_query_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code          | Description
 * --------------------|-------------
 * @ref CSMI_SUCCESS   | Successfully inserted inventory into the *csm_ib_cable* table.
 * @ref CSMERR_GENERIC | Default error case.
 */
int csm_ib_cable_inventory_collection(csm_api_object **handle, csm_ib_cable_inventory_collection_input_t* input, csm_ib_cable_inventory_collection_output_t* *output);

/** @ingroup inv_apis
 * @brief Used to query the 'csm_ib_cable' table of the CSM database.
 *
 * Retrieves information about the ib cables specified in @p input from the *csm_ib_cable* table in the
 * CSM Database. The @p output struct will be populated with the contents of the rows 
 * retrieved. In the event of an error @p output will be null.
 * 
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table   | Description 
 *           -------------|-------------
 *           csm_ib_cable | Contains information about the Infiniband cable.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the csm_ib_cable_query API.
 * @param[out] output Used to contain the output parameters for the csm_ib_cable_query API, consult @ref csm_ib_cable_query_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_switch* table.
 */
int csm_ib_cable_query(csm_api_object **handle, csm_ib_cable_query_input_t* input, csm_ib_cable_query_output_t* *output);

/** @ingroup inv_apis undefined
 * @brief Used by the sys admin to get information on all entries in the "csm_ib_cables" table of the CSM DB.
 *
 * only a stub.
 *
 * This api returns the current attributes of all Infiniband (IB) cables in the system. This api queries the "csm_ib_cables" 
 * table of the CSM DB and collects the data for each entry in the table. Each cable’s attributes are stored in a "csm_ib_cable_t" 
 * struct and placed as an entry in *data[]. On a successful call, there is an entry in *data[] for each entry in the table. 
 * If there is a failure, then *data[] will not contain any data.
 *
 * Attributes include everything that we know about each ib cable.
 * Note that CSM will allocate the memory for cableData.
 * Callers must free this array when data is no longer needed.
 *
 * @param handle A pointer to return a pointer to internally managed api data.
 * @param data A pointer to return a pointer to an array of "csm_ib_cable _t" structs. There will be an entry in this 
 * 						array for each entry in the "csm_ib_cables" table of the CSM DB.
 * @param dataCount A pointer to return the number of entries in "*data[]"
 *
 * @return 0 Success
 * @return -1 Error: use "csm_api_object_errcode_get" and "csm_api_object_errmsg_get" 
 *					to retrieve additional information.
 */
int csm_ib_cable_query_active_all(csm_api_object **handle);

/** @ingroup inv_apis
 * @brief Used to query the 'csm_ib_cable_history' table of the CSM database.
 *
 * History includes everything that we know about an ib cable over the course of its lifetime.
 * Retrieves information about the ib cable specified in @p input from the *csm_ib_cable_history* table in the
 * CSM Database. The @p output struct will be populated with the contents of the rows 
 * retrieved. In the event of an error @p output will be null.
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table           | Description 
 *           ---------------------|-------------
 *           csm_ib_cable_history | Contains historical information about the Infiniband cable
 *
 * @param handle A pointer to return a pointer to internally managed api data.
 * @param[in]  input Used to contain the input parameters for the csm_ib_cable_query_history API.
 * @param[out] output Used to contain the output parameters for the csm_ib_cable_query_history API, consult @ref csm_ib_cable_query_history_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_ib_cable_history* table.
 */
int csm_ib_cable_query_history(csm_api_object **handle, csm_ib_cable_query_history_input_t* input, csm_ib_cable_query_history_output_t* *output);

/** @ingroup inv_apis
 * @brief Used to update records in the "csm_ib_cable" table of the CSM database.
 *
 * Updates an existing ib cable entry in the CSM Database *csm_ib_cable* table using the contents of the 
 * @p input struct. For details on configuration consult @ref csm_ib_cable_update_input_t.
 * After executing the update @p output will be populated with information related to the results of the SQL update. 
 *
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 * @todo document interactions with history table?
 *
 *  ## Database Tables ##
 *  Table                | Description 
 *  ---------------------|-------------
 *  csm_ib_cable         | Contains information about the Infiniband cable.
 *  csm_ib_cable_history | Contains historical information about the Infiniband cable
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]   input A configuration struct containing the fields that may be updated in the query.
 *                      Consult @ref csm_ib_cable_update_input_t for usage.
 * @param[out] output A record of the status of the update, in the event of a failure, the ib cables
 *                      that were not updated will be recorded here.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 *            @ref CSMI_SUCCESS | Successfully updated the *csm_ib_cable* table.
 */
int csm_ib_cable_update(csm_api_object **handle, csm_ib_cable_update_input_t* input, csm_ib_cable_update_output_t* *output);

/** @ingroup inv_apis undefined
 * @brief Gets the fabric topology;
 *
 * Note that CSM will allocate the memory for fabricTopology.
 * Callers must free this memory when data is no longer needed.
 *
 * @param fabricTopology ptr to the fabric topology data
 *
 * @return 0 Success
 * @return -1 Error: use "csm_api_object_errcode_get" and "csm_api_object_errmsg_get" 
 *					to retrieve additional information.
 */
int csm_fabric_topology_get(csmi_fabric_topology_t **fabricTopology);

/** @ingroup inv_apis undefined
 * @brief Used by the system admin to get the environmental data of requested node list.
 *
 * Retrieves environmental information about the nodes specified in @p node_list from
 * the **todo populate** table in the CSM Database. 
 *
 * @p env_data must be destroyed using @ref csm_api_object_destroy.
 *
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *
 * @param[out]             handle An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[in]           node_list  A configuration struct containing the  list of nodes to retrieve
 *                                  environmental data for.
 * @param[in,out]      data_count The number of nodes specified in @p node_list. At the end of 
 *                                  execution contains the number of nodes in @p env_data.
 *                                  In the event of an error this will be set to 0.
 * @param[out]    node_attributes The results of the query, **todo**
 *                                  Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the **todo** table.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_env_data_query(csm_api_object **handle, const char *node_list[], 
                            uint32_t *data_count, csmi_node_env_data_t *env_data[]);
							
							
/** @ingroup inv_apis
 * @brief Used to query the CSM database get information about all the nodes in the system and what allocaitons they are apart of.
 *
 * 
 * @p output must be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table   | Description 
 *           -------------|-------------
 *           csm_node | Contains information about the node.
 *           csm_allocation_node | Contains information about the allocation node.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                      with @ref csm_api_object_destroy.
 * @param[in]  input Used to contain the input parameters for the this API.
 * @param[out] output Used to contain the output parameters for the this API, consult @ref csm_cluster_query_state_output_t
 *                      for details. Null in the event of an error.
 *                      Destroy using @ref csm_api_object_destroy
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the *csm_switch* table.
 */
int csm_cluster_query_state(csm_api_object **handle, csm_cluster_query_state_input_t* input, csm_cluster_query_state_output_t* *output);

#ifdef __cplusplus
}
#endif

#endif

