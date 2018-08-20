/*================================================================================
   
    csmi/include/csmi_type_inv.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_inv.h
 * @brief A collection of structs for @ref inv_apis.
 */
#ifndef _CSMI_INV_TYPES_H_
#define _CSMI_INV_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// Begin special_preprocess directives
// Used for node_attributes
// Include the database constants for the CSM_STATE_MAX.
#include <time.h>
#include "csm_api_common.h"

/** @addtogroup inv_apis
 * @{
 */

// For csmi_node_env_data_t
/// Number of GPUs expected on a node.
#define CSM_NUM_GPUS_PER_NODE    6
/// Number of CPU sockets on a node.
#define CSM_NUM_SOCKETS_PER_NODE 2
/// Number of Cores per node.
#define CSM_NUM_CORES_PER_NODE   48
/// Number of memory DIMMs per node.
#define CSM_NUM_DIMMS_PER_NODE   16


// End special_preprocess directives

/** defgroup csmi_node_alteration_t csmi_node_alteration_t
 * @{
 */
/**
  * @brief The various causes of a node to change its state.
 *
  */
typedef enum {
   CSM_NODE_ALTERATION_NO_DEF=0, ///< 0 - csmi_node_alteration_t has not been set yet or value is unknown. Node has no specified alteration.
   CSM_NODE_ALTERATION_DATABASE_NULL=1, ///< 1 - Used to represent a NULL value for CSM Database.
   CSM_NODE_ALTERATION_CSM_API=2, ///< 2 - Node state was altered by a CSM API.
   CSM_NODE_ALTERATION_RAS_EVENT=3, ///< 3 - Node state was altered by a ras event. Usually set to SOFT_FAILURE.
   CSM_NODE_ALTERATION_SYS_ADMIN=4, ///< 4 - Node state was altered by a system administrator.
   CSM_NODE_ALTERATION_CSM_INVENTORY=5, ///< 5 - Node state was altered by CSM's inventory.
   csmi_node_alteration_t_MAX=6 ///< 6 - Bounding Value
} csmi_node_alteration_t;



/**
 * @brief Maps enum csmi_node_alteration_t value to string.
 */
extern const char* csmi_node_alteration_t_strs [];
/** @}*/


/**
 * @brief Used to return data from a query of the **csm_dimm** table of the CSM Database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* serial_number; /**< Unique identifier for this DIMM. */
    char* node_name; /**< The DIMM is installed on this node. */
    char* physical_location; /**< Physical location where the hca is installed. */
    int32_t size; /**< The size of the memory DIMM, in GB. */
    char status; /**< Deprecated after CSM_VERSION_0_4_1. Status of the DIMM - [a]ctive, [d]econfigured, [f]ailure. */
} csmi_dimm_record_t;
/**
 * @brief A gpu record in the **csm_gpu** table of the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t gpu_id; /**< Gpu identification number. */
    int64_t hbm_memory; /**< High bandwidth memory: amount of available memory on this gpu (in kB). */
    char* node_name; /**< The name of the node containing the GPU. */
    char* device_name; /**< The name of the GPU device.*/
    char* inforom_image_version; /**< Version of the infoROM installed on the GPU. */
    char* pci_bus_id; /**< The bus id of the GPU device on the node. */
    char* serial_number; /**< The serial number of the GPU.*/
    char* status; /**< Deprecated after CSM_VERSION_0_4_1. The status of the GPU - active, deconfigured, failure */
    char* uuid; /**< The unique id reported by the GPU. */
    char* vbios; /**< The video BIOS of the GPU. */
} csmi_gpu_record_t;
/**
 * @brief A HCA record in the **csm_hca** table of the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* serial_number; /**< Serial number reported by the HCA.*/
    char* node_name; /**< The node containing the HCA.*/
    char* board_id; /**< Board id of the ib adapter.*/
    char* device_name; /**< Product device name for the HCA.*/
    char* fw_ver; /**< Firmware version of the HCA.*/
    char* guid; /**< The sys_image_guid of the HCA.*/
    char* hw_rev; /**< Hardware revision of this HCA.*/
    char* part_number; /**< Part number of the HCA. */
    char* pci_bus_id; /**< The bus id of the HCA on the node.*/
} csmi_hca_record_t;
/**
 * @brief Used to contain a 'csm_ib_cable' database record.
 *
 * @author Nick Buonarota nbuonar@us.ibm.com
 * First included in Beta 2
 * Last edited: Nov. 28, 2017
 *
 *
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* serial_number; /**< Identifies the cable's serial number. */
    char* discovery_time; /**< First time the ib cable was found in the system. */
    char* collection_time; /**< Last time the ib cable inventory was collected. */
    char* comment; /**< Comment can be generated for this field. */
    char* guid_s1; /**< guid: side 1 of the cable. */
    char* guid_s2; /**< guid: side 2 of the cable. */
    char* identifier; /**< TBD. */
    char* length; /**< Length of the cable. */
    char* name; /**< TBD. */
    char* part_number; /**< Part number of this particular ib cable. */
    char* port_s1; /**< port: side 1 of the cable. */
    char* port_s2; /**< port: side 2 of the cable. */
    char* revision; /**< Hardware revision associated with this ib cable. */
    char* severity; /**< TBD. */
    char* type; /**< Specific type of cable used. */
    char* width; /**< TBD. */
} csmi_ib_cable_record_t;
/**
 * @brief Used to contain custom record information returned from the database.
 *
 * @author Nick Buonarota nbuonar@us.ibm.com
 * First included in Beta 2
 * Last edited: December 6, 2017
 *
 *
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* history_time; /**< When the cable enters the history table. */
    char* serial_number; /**< Identifies the cable's serial number. */
    char* discovery_time; /**< First time the ib cable was found in the system. */
    char* collection_time; /**< Last time the ib cable inventory was collected. */
    char* comment; /**< Comment can be generated for this field. */
    char* guid_s1; /**< guid: side 1 of the cable. */
    char* guid_s2; /**< guid: side 2 of the cable. */
    char* identifier; /**< TBD. */
    char* length; /**< Length of the cable. */
    char* name; /**< TBD. */
    char* part_number; /**< Part number of this particular ib cable. */
    char* port_s1; /**< port: side 1 of the cable. */
    char* port_s2; /**< port: side 2 of the cable. */
    char* revision; /**< Hardware revision associated with this ib cable. */
    char* severity; /**< TBD. */
    char* type; /**< Specific type of cable used. */
    char* width; /**< TBD. */
} csmi_ib_cable_history_record_t;
/**
 * @brief A node record in the **csm_node** table of the CSM database.
 * @todo Post-PRPQ: Only one field different from @ref csmi_node_attributes_history_record_t.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t available_cores; /**< Deprecated after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    int32_t available_gpus; /**< Deprecated after CSM_VERSION_0_4_1. Number of gpus available. */
    int32_t available_processors; /**< Deprecated after CSM_VERSION_0_4_1. Number of processors available on the node.*/
    int32_t discovered_hcas; /**< Number of IB HCAs discovered on this node during the most recent inventory collection. */
    int32_t hard_power_cap; /**< The hard power capacity for the node, the node may not exceed this power capacity. */
    int64_t installed_memory; /**< Amount of installed memory on this node in kB. */  
    int64_t installed_swap; /**< Amount of available swap space on this node in kB. */
    csmi_node_state_t state; /**< Deprecated after CSM_VERSION_0_4_1. State of the node, see @ref csmi_node_state_t for details. */
    csmi_node_type_t type; /**< The type of the node, see @ref csmi_node_state_t for details.*/
    csm_bool ready; /**< Deprecated after CSM_VERSION_0_4_1. The ready state of the node, used for workload management. */
    char* node_name; /**< The node hostname the record represents. */
    char* comment; /**< Comment field for system administrators. */
    char* feature_1; /**< Reserved field for future use.*/
    char* feature_2; /**< Reserved field for future use.*/
    char* feature_3; /**< Reserved field for future use.*/
    char* feature_4; /**< Reserved field for future use.*/
    char* kernel_release; /**< Kernel release being run on the node. */
    char* kernel_version; /**< Kernel version being run on the node. */
    char* machine_model; /**< Machine type model information for the node. */
    char* os_image_name; /**< xCAT os image name being run on this node, diskless images only. */
    char* os_image_uuid; /**< xCAT os image uuid being run on this node, diskless images only. */
    char* physical_frame_location; /**< Physical frame number where the node is located. */
    char* physical_u_location; /**< Physical u location where the node is located. */
    char* primary_agg; /**< Primary aggregator of the node. */
    char* secondary_agg; /**< Secondary aggregator of the node. */
    char* serial_number; /**< The serial number of the node. */
    char* discovery_time; /**< The time the node was discovered. */
    char* update_time; /**< The time the node record was last updated. */
    int32_t discovered_cores; /**< replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    int32_t discovered_gpus; /**< replacement for 'available_gpus' after CSM_VERSION_0_4_1. Number of gpus on node. */
    int32_t discovered_sockets; /**< replacement for 'available_processors' after CSM_VERSION_0_4_1. Number of processors on the node.*/
    char* collection_time; /**< replacement for 'discovery_time' after CSM_VERSION_0_4_1. the inventory information for this node was last collected at this time. */
    int32_t discovered_dimms; /**< Number of dimms discovered via inventory on this node. */
    int32_t discovered_ssds; /**< Number of ssds discovered via inventory on this node. */
} csmi_node_attributes_record_t;
/**
 * @brief A node record in the **csm_node_history** table of the CSM database.
 * @todo Post-PRPQ: Only one field different from @ref csmi_node_attributes_record_t
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t available_cores; /**< Deprecated after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    int32_t available_gpus; /**< Deprecated after CSM_VERSION_0_4_1. Number of gpus available. */
    int32_t available_processors; /**< Deprecated after CSM_VERSION_0_4_1. Number of processors available on the node. */
    int32_t discovered_hcas; /**< Number of IB HCAs discovered on this node during the most recent inventory collection. */
    int32_t hard_power_cap; /**< The hard power capacity for the node, the node may not exceed this power capacity. */ 
    int64_t installed_memory; /**< Amount of installed memory on this node in kB. */
    int64_t installed_swap; /**< Amount of available swap space on this node in kB. */
    csmi_node_state_t state; /**< State of the node, see @ref csmi_node_state_t for details. */
    csmi_node_type_t type; /**<The type of the node, see @ref csmi_node_state_t for details.*/
    csm_bool ready; /**< Deprecated after CSM_VERSION_0_4_1. The ready state of the node, used for workload management. */
    char* node_name; /**< The node hostname the record represents. */
    char* comment; /**< Comment field for system administrators. */
    char* feature_1; /**< Reserved field for future use.*/
    char* feature_2; /**< Reserved field for future use.*/
    char* feature_3; /**< Reserved field for future use.*/
    char* feature_4; /**< Reserved field for future use.*/
    char* kernel_release; /**< Kernel release being run on the node. */
    char* kernel_version; /**< Kernel version being run on the node. */
    char* machine_model; /**< Machine type model information for the node. */
    char* os_image_name; /**< xCAT os image name being run on this node, diskless images only. */
    char* os_image_uuid; /**< xCAT os image uuid being run on this node, diskless images only. */
    char* physical_frame_location; /**< Physical frame number where the node is located. */
    char* physical_u_location; /**< Physical u location where the node is located. */
    char* primary_agg; /**< Primary aggregator of the node. */
    char* secondary_agg; /**< Secondary aggregator of the node. */
    char* serial_number; /**< The serial number of the node. */
    char* discovery_time; /**< Deprecated after CSM_VERSION_0_4_1. The time the node was discovered. */
    char* update_time; /**< The time the node record was last updated. */
    char* history_time; /**< Time the record entered the history table. */
    int32_t discovered_cores; /**< replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    int32_t discovered_gpus; /**< replacement for 'available_gpus' after CSM_VERSION_0_4_1. Number of gpus on node. */
    int32_t discovered_sockets; /**< replacement for 'available_processors' after CSM_VERSION_0_4_1. Number of processors on the node.*/
    char* collection_time; /**< replacement for 'discovery_time' after CSM_VERSION_0_4_1. the inventory information for this node was last collected at this time. */
    int32_t discovered_dimms; /**< Number of dimms discovered via inventory on this node. */
    int32_t discovered_ssds; /**< Number of ssds discovered via inventory on this node. */
} csmi_node_attributes_history_record_t;
/**
 * @brief A custom record for querying the state history of a node in the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* history_time; /**< Time the record entered the history table. */
    csmi_node_state_t state; /**< State of the node, see @ref csmi_node_state_t for details. */
    csmi_node_alteration_t alteration; /**< Reason for the state change. */
    char* ras_rec_id; /**< If Reason for the state change is ras. this is the unique ras id. */
    char* ras_msg_id; /**< If Reason for the state change is ras. this is the ras msg id. */
} csmi_node_query_state_history_record_t;
/**
 * @brief A processor_socket record in the **csm_processor_socket** table of the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t available_cores; /**< Deprecated after CSM_VERSION_0_4_1. The number of physical cores available on this processor.*/
    int32_t socket; /**< Deprecated after CSM_VERSION_0_4_1. The socket number of the processor. */
    char status; /**< Deprecated after CSM_VERSION_0_4_1. Status of the processor - [a]ctive, [d]econfigured, [f]ailure */
    char* serial_number; /**< Serial number of the processor. */
    char* node_name; /**< The node name of the processor. */
    int32_t discovered_cores; /**< replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. */
    char* physical_location; /**< replacement for 'socket' after CSM_VERSION_0_4_1. Physical location of the processor. */
} csmi_processor_record_t;
/**
 * @brief A ssd record in the **csm_ssd** table of the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t total_size; /**< Deprecated after CSM_VERSION_0_4_1. Physical total capacity (volume group total capacity). */
    int64_t size; /**< Total capacity (in bytes) of this ssd. */
    int64_t wear_total_bytes_read; /**< Number of bytes read from the SSD over the life of the device. */
    int64_t wear_total_bytes_written; /**< Number of bytes written to the SSD over the life of the device. */
    double wear_lifespan_used; /**< Estimate of the amount of SSD life consumed (w.l.m. will use. valid range 0-255 percent) 0 = new, 100 = completely used, 100+ = over est life time. */
    double wear_percent_spares_remaining; /**< Amount of SSD capacity over-provisioning that remains.*/
    char status; /**< Deprecated after CSM_VERSION_0_4_1. The state of the ssd - [a]ctive, [d]econfigured, [f]ailure */
    char* serial_number; /**< Serial number of the ssd. */
    char* node_name; /**< The node of the ssd. */
    char* discovery_time; /**< Deprecated after CSM_VERSION_0_4_1. Timestamp when this ssd was discovered. */
    char* update_time; /**< Timestamp when this ssd was last updated. */
    char* device_name; /**< product device name. */
    char* pci_bus_id; /**< PCI bus id. */
    char* fw_ver; /**< firmware version. */
} csmi_ssd_record_t;
/**
 * @brief A switch history record in the **csm_switch** table of the CSM database.
 * @todo Post-PRPQ: Only one field different from @ref csmi_switch_history_record_t.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* switch_name; /**< switch name: Identification of the system For hosts, it is caguid, For 1U switch, it is switchguid, For modular switches, is it sysimgguid */
    char* serial_number; /**< Serial number for this switch. identifies the switch this information is for */
    char* discovery_time; /**< First time the switch was found in the system. */
    char* collection_time; /**< Last time the switch inventory was collected. */
    char* comment; /**< System administrator comment about the switch. */
    char* description; /**< description of system ... system type of this systems (More options: SHArP, MSX1710 , CS7520). */
    char* fw_version; /**< firmware version of the Switch or HCA */
    char* gu_id; /**< Node guid of the system. In case of HCA, it is the caguid. In case of Switch, it is the switchguid  */
    csm_bool has_ufm_agent; /**< indicate if system (Switch or Host) is running a UFM Agent */
    char* hw_version; /**< hardware version related to the switch */
    char* ip; /**< ip address of the system (Switch or Host)  (0.0.0.0 in case ip address not available) */
    char* model; /**< system model ... in case of switch, it is the switch model, For hosts ... Computer */
    int32_t num_modules; /**< number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name. */
    char* physical_frame_location; /**< The switch frame location. */
    char* physical_u_location; /**< The switch u location in the frame. */
    char* ps_id; /**< PSID (Parameter-Set IDentification) is a 16-ascii character string embedded in the firmware image which provides a unique identification for the configuration of the firmware. */
    char* role; /**< Type/Role of system in the current fabric topology: Tor / Core / Endpoint (host). (Optional Values: core, tor, endpoint) */
    char* server_operation_mode; /**< Operation mode of system. (Optional Values: Stand_Alone, HA_Active, HA_StandBy, Not_UFM_Server, Router, Gateway, Switch) */
    char* sm_mode; /**< Indicate if SM is running on that system. (Optional Values: noSM, activeSM, hasSM) */
    char* state; /**< ??? (active, missing, error, service, softfailure)? */
    char* sw_version; /**< software version of the system ... full MLNX_OS version. Relevant only for MLNX-OS systems (Not available for Hosts) */
    char* system_guid; /**< system image guid for that system */
    char* system_name; /**< system name as it appear on the system node description */
    int32_t total_alarms; /**< total number of alarms which are currently exist on the system */
    char* type; /**< ??? switch */
    char* vendor; /**< system vendor. */
} csmi_switch_record_t;
/**
 * @brief A switch inventory record in the **csm_switch_inventory** table of the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* name;  /**< name (identifier) of this module in UFM. */
    char* host_system_guid;  /**< the system image guid of the hosting system. */
    char* discovery_time;  /**< First time the module was found in the system. */
    char* collection_time;  /**< Last time the module inventory was collected. */
    char* comment;  /**< System administrator comment about this module. */
    char* description;  /**< description type of module - can be the module type: system, FAN, MGMT,PS or the type of module in case of line / spine modules: SIB7510(Barracud line), SIB7520(Barracuda spine) */
    char* device_name;  /**< name of device containing this module. */
    char* device_type;  /**< type of device module belongs to. */
    char* hw_version;  /**< hardware version related to the switch. */
    int32_t max_ib_ports;  /**< maximum number of external ports of this module. */
    int32_t module_index;  /**< index of module. Each module type has separate index: FAN1,FAN2,FAN3...S1,PS2 */
    int32_t number_of_chips;  /**< number of chips which are contained in this module. (relevant only for line / spine modules, for all other modules number_of_chips=0) */
    char* path;  /**< full path of module object. Path format: site-name (number of devices) / device type: device-name / module description module index. */
    char* serial_number;  /**< serial_number of the module. unique identifier. */
    char* severity;  /**< severity of the module according to the highest severity of related events. values: Info, Warning, Minor, Critical */
    char* status;  /**< current module status. valid values: ok, fault */
} csmi_switch_inventory_record_t;
/**
 * @brief A switch ports record in the **csm_switch_ports** table of the CSM database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* name;  /**< UFM name for this module. format: host_number */
    char* parent;  /**< Switch that this port is on. */
    char* discovery_time;  /**< First time the port was found in the system. */
    char* collection_time;  /**< Last time the port inventory was collected. */
    char* active_speed;  /**< ALAN ADD COMMENT */
    char* comment;  /**< System administrator comment about this port. */
    char* description;  /**< ALAN ADD COMMENT */
    char* enabled_speed;  /**< ALAN ADD COMMENT */
    char* external_number;  /**< ALAN ADD COMMENT */
    char* guid;  /**< ALAN ADD COMMENT */
    char* lid;  /**< ALAN ADD COMMENT */
    char* max_supported_speed;  /**< ALAN ADD COMMENT */
    char* logical_state;  /**< ALAN ADD COMMENT */
    char* mirror;  /**< ALAN ADD COMMENT */
    char* mirror_traffic;  /**< ALAN ADD COMMENT */
    char* module;  /**< ALAN ADD COMMENT */
    char* mtu;  /**< ALAN ADD COMMENT */
    char* number;  /**< port number */
    char* physical_state;  /**< ALAN ADD COMMENT */
    char* peer;  /**< ALAN ADD COMMENT */
    char* severity;  /**< ALAN ADD COMMENT */
    char* supported_speed;  /**< ALAN ADD COMMENT */
    char* system_guid;  /**< ALAN ADD COMMENT */
    char* tier;  /**< ALAN ADD COMMENT */
    char* width_active;  /**< ALAN ADD COMMENT */
    char* width_enabled;  /**< ALAN ADD COMMENT */
    char* width_supported;  /**< ALAN ADD COMMENT */
} csmi_switch_ports_record_t;
/**
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    csmi_switch_record_t* switch_data; /**< Basic information for this switch. */
    uint32_t inventory_count; /**< Number of elements in the 'inventory' array. */
    csmi_switch_inventory_record_t** inventory; /**< An array of all the inventory records attached to this switch. */
    uint32_t ports_count; /**< Deprecated after CSM_VERSION_0_4_1. Number of elements in the 'ports' array. */
    csmi_switch_ports_record_t** ports; /**< Deprecated after CSM_VERSION_0_4_1. An array of all the ports records attached to this switch. */
} csmi_switch_details_t;
/**
 * @brief A switch history record in the **csm_switch_history** table of the CSM database.
 * @todo Post-PRPQ: Only one field different from @ref csmi_switch_record_t.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* history_time;  /**< Time the record entered the history table. */
    char* switch_name; /**< switch name: Identification of the system For hosts, it is caguid, For 1U switch, it is switchguid, For modular switches, is it sysimgguid */
    char* serial_number; /**< Serial number for this switch. identifies the switch this information is for */
    char* discovery_time; /**< First time the switch was found in the system. */
    char* collection_time; /**< Last time the switch inventory was collected. */
    char* comment; /**< System administrator comment about the switch. */
    char* description; /**< description of system \u2013 system type of this systems (More options: SHArP, MSX1710 , CS7520). */
    char* fw_version; /**< firmware version of the Switch or HCA */
    char* gu_id; /**< Node guid of the system. In case of HCA, it is the caguid. In case of Switch, it is the switchguid  */
    csm_bool has_ufm_agent; /**< indicate if system (Switch or Host) is running a UFM Agent */
    char* hw_version; /**< hardware version related to the switch */
    char* ip; /**< ip address of the system (Switch or Host)  (0.0.0.0 in case ip address not available) */
    char* model; /**< system model \u2013 in case of switch, it is the switch model, For hosts \u2013 Computer */
    int32_t num_modules; /**< number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name. */
    char* physical_frame_location; /**< The switch frame location. */
    char* physical_u_location; /**< The switch u location in the frame. */
    char* ps_id; /**< PSID (Parameter-Set IDentification) is a 16-ascii character string embedded in the firmware image which provides a unique identification for the configuration of the firmware. */
    char* role; /**< Type/Role of system in the current fabric topology: Tor / Core / Endpoint (host). (Optional Values: core, tor, endpoint) */
    char* server_operation_mode; /**< Operation mode of system. (Optional Values: Stand_Alone, HA_Active, HA_StandBy, Not_UFM_Server, Router, Gateway, Switch) */
    char* sm_mode; /**< Indicate if SM is running on that system. (Optional Values: noSM, activeSM, hasSM) */
    char* state; /**< ?(active, missing, error, service, softfailure)? */
    char* sw_version; /**< software version of the system \u2013 full MLNX_OS version. Relevant only for MLNX-OS systems (Not available for Hosts) */
    char* system_guid; /**< system image guid for that system */
    char* system_name; /**< system name as it appear on the system node description */
    int32_t total_alarms; /**< total number of alarms which are currently exist on the system */
    char* type; /**< ??? switch */
    char* vendor; /**< system vendor. */
    char* operation; /**< operation of transaction (I - INSERT), (U - UPDATE), (D - DELETE) */
    char* archive_history_time; /**< timestamp when the history data has been archived and sent to: BDS, archive file, and or other */
} csmi_switch_history_record_t;
/**
 * @brief Node environmental data in the CSM database.
 * @todo Post-PRPQ: Implement.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char field_01; /**< Placeholder value. */
} csmi_node_env_data_t;
/**
 * @brief Environmental data aggregated from a switch.
 * @todo Post-PRPQ: Implement @ref csmi_switch_env_data_t.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char field_01; /**< Unused. */
} csmi_switch_env_data_t;
/**
 * @brief A fabric topology record from the CSM database.
 * @todo Post-PRPQ: This needs to be implemented.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char field_01; /**< */
} csmi_fabric_topology_t;
/**
 * @brief Aggregates **csm_node** table and sister tables in the CSM Database.
 * Consult each struct array for nodes contained in this structure.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t dimms_count; /**< Number of dimms associated with @ref node, size of @ref dimms. */
    uint32_t gpus_count; /**< Number of gpus associated with @ref node, size of @ref gpus.*/
    uint32_t hcas_count; /**< Number of hcas associated with @ref node, size of @ref hcas.*/
    uint32_t processors_count; /**< Number of processors associated with @ref node, size of @ref processors.*/
    uint32_t ssds_count; /**< Number of ssds associated with @ref node, size of @ref ssds. */
    csmi_node_attributes_record_t* node; /**< Basic information for the node. */
    csmi_dimm_record_t** dimms; /**< The dimms associated with @ref node, size defined by @ref dimms_count.*/
    csmi_gpu_record_t** gpus; /**< The gpus associated with @ref node, size defined by @ref gpus_count.*/ 
    csmi_hca_record_t** hcas; /**< The hcas associated with @ref node, size defined by @ref hcas_count.*/
    csmi_processor_record_t** processors; /**< The processors associated with @ref node, size defined by @ref processors_count.*/
    csmi_ssd_record_t** ssds; /**< The ssds associated with @ref node, size defined by @ref ssds_count.*/
} csmi_node_details_t;
/**
 * @brief A custom cluster query record from the database.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* node_name; /**< The node hostname the record represents. */
    char* collection_time; /**< replacement for 'discovery_time' after CSM_VERSION_0_4_1. the inventory information for this node was last collected at this time. */
    char* update_time; /**< The time the node record was last updated. */
    csmi_node_state_t state; /**< Deprecated after CSM_VERSION_0_4_1. State of the node, see @ref csmi_node_state_t for details. */
    csmi_node_type_t type; /**< The type of the node, see @ref csmi_node_state_t for details.*/
    uint32_t num_allocs; /**< Number of allocations that this node is participating in. also the length member for the following arrays */
    int64_t* allocs; /**< Array of allocation ids this node is participating in. */
    char** states; /**< Array of states the allocations on this node are in (order matches the allocation ids). */
    char** shared; /**< Array of "is this allocation shared" . */
} csmi_cluster_query_state_record_t;
/**
 * @brief An input wrapper for @ref csm_ib_cable_inventory_collection
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t inventory_count; /**< Number of ib cable records, size of @ref inventory. */
    csmi_ib_cable_record_t** inventory; /**< The ib cable records to be inserted into the database, size defined in @ref inventory_count.*/
} csm_ib_cable_inventory_collection_input_t;
/**
 * @brief A wrapper for the output of @ref csm_ib_cable_inventory_collection
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t insert_count; /**< number of new records inserted into the database. */
    int32_t update_count; /**< number of old records updated in the database. */
} csm_ib_cable_inventory_collection_output_t;
/**
 * @brief An input wrapper for @ref csm_ib_cable_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    uint32_t serial_numbers_count; /**< Number of serial numbers to query on, size of @ref serial_numbers. */
    char** serial_numbers; /**< Listing of serial numbers to query the database for matches of, size defined by @ref serial_numbers_count. */
} csm_ib_cable_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_ib_cable_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of records retrieved by the query, size of @ref results.*/
    csmi_ib_cable_record_t** results; /**< The list of records retrieved by the query, size defined in @ref results_count.*/
} csm_ib_cable_query_output_t;
/**
 * @brief An input wrapper for @ref csm_ib_cable_query_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    char* serial_number; /**< The serial number of the Infiniband cable.*/
} csm_ib_cable_query_history_input_t;
/**
 * @brief A wrapper for the output of @ref csm_ib_cable_query_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of records retrieved, size of @ref results. */
    csmi_ib_cable_history_record_t** results; /**< Records retrieved from the query, size defined by @ref results_count.*/
} csm_ib_cable_query_history_output_t;
/**
 *  @brief An input wrapper for @ref csm_ib_cable_update.
 *  @todo Post-PRPQ: Do we really need an array of serial numbers?
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t serial_numbers_count; /**< Number of serial numbers to update, size of @ref serial_numbers.*/
    char** serial_numbers; /**< List of ib cable serial numbers to update, size defined by @ref serial_numbers_count.*/
    char* comment; /**< Comment can be generated for this field. Can be reset to NULL in CSM DB via \"#CSM_NULL\". */
    char* guid_s1; /**< guid: side 1 of the cable. - Deprecated as of CSM_VERSION_1_2_0 */
    char* guid_s2; /**< guid: side 2 of the cable. - Deprecated as of CSM_VERSION_1_2_0 */
    char* port_s1; /**< port: side 1 of the cable. - Deprecated as of CSM_VERSION_1_2_0 */
    char* port_s2; /**< port: side 2 of the cable. - Deprecated as of CSM_VERSION_1_2_0 */
} csm_ib_cable_update_input_t;
/**
 * @brief A wrapper for the output of @ref csm_ib_cable_update.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t failure_count; /**< The number of ib cables that failed to be updated, size of @ref failure_ib_cables.*/
    char** failure_ib_cables; /**< A list of ib cable serial numbers which failed to update, size defined by @ref failure_count.*/
} csm_ib_cable_update_output_t;
/**
 * @brief An input wrapper for @ref csm_node_attributes_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    uint32_t node_names_count; /**< Number of names to query, size of @ref node_names. */
    csm_bool ready; /**< Deprecated after CSM_VERSION_0_4_1. Query the 'ready' field in the database. API will ignore @ref CSM_UNDEF_BOOL for this field.  Valid values: 0,1,2. API checks for invalid values and fails if invalid values are passed. Database description: Is the node ready for workload manager? ('y' or 'n') */
    char** node_names; /**< List of nodes to perform query on, size defined in @ref node_names_count.*/
    char* comment; /**< Query the 'comment' field in the database. API will ignore NULL values for this field. Database description: Comment field for system administrators.*/
    csmi_node_type_t type; /**< Query the 'type' field in the database. API will ignore @ref csmi_node_state_t::CSM_NODE_NO_TYPE values for this fields, see @ref csmi_node_state_t for details.*/
    csmi_node_state_t state; /**< replacement for 'ready' after CSM_VERSION_0_4_1. Query the 'state' field in the database. API will ignore @ref CSM_NODE_NO_DEF for this field. API checks for invalid values and fails if invalid values are passed. State of the node, see @ref csmi_node_state_t for details. */
} csm_node_attributes_query_input_t;
/**
 * @brief A wrapper for the output of @ref csm_node_attributes_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of node records retrieved, size of @ref results. */
    csmi_node_attributes_record_t** results; /**< A list of records retrieved from the queries, size defined by @ref results_count.*/
} csm_node_attributes_query_output_t;
/**
 * @brief An input wrapper for @ref csm_node_attributes_query_details.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* node_name; /**< Identifies which node will be queried. If left NULL, then API exit early.*/
} csm_node_attributes_query_details_input_t;
/**
 * @brief A wrapper for the output of @ref csm_node_attributes_query_details.
 * @todo Post-PRPQ: Should this be an array?
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t result_count; /**< Number of node records, size of @ref result. */
    csmi_node_details_t** result; /**< Nodes found by the query, size defined in @ref result_count. */
} csm_node_attributes_query_details_output_t;
/**
 * @brief An input wrapper for @ref csm_node_attributes_query_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    char* node_name; /**< Identifies which node will be queried. If left NULL, the API will exit early.*/
    char order_by; /**< Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY history_time DESC NULLS LAST'. VALID VALUES: [d] =  'ORDER BY history_time DESC NULLS LAST', [a] = 'ORDER BY history_time ASC NULLS LAST'. @TODO should this be a boolean?*/
} csm_node_attributes_query_history_input_t;
/**
 * @brief A wrapper for the output of @ref csm_node_attributes_query_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of history records retrieved by the query, size of @ref results. */
    csmi_node_attributes_history_record_t** results; /**< The list of history records retrieved from the query, size defined by @ref results_count. */
} csm_node_attributes_query_history_output_t;
/**
 * @brief An input wrapper for @ref csm_node_query_state_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    char* node_name; /**< Identifies which node will be queried. If left NULL, the API will exit early.*/
    char order_by; /**< Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY history_time DESC NULLS LAST'. VALID VALUES: [d] =  'ORDER BY history_time DESC NULLS LAST', [a] = 'ORDER BY history_time ASC NULLS LAST'. @TODO should this be a boolean?*/
} csm_node_query_state_history_input_t;
/**
 * @brief A wrapper for the output of @ref csm_node_query_state_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* node_name; /**< The name of the node that was queried. */
    uint32_t results_count; /**< Number of history records retrieved by the query, size of @ref results. */
    csmi_node_query_state_history_record_t** results; /**< The list of history records retrieved from the query, size defined by @ref results_count. */
} csm_node_query_state_history_output_t;
/**
 * @brief An input wrapper for @ref  csm_node_attributes_update.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t node_names_count; /**< The number of nodes to update, size of @ref node_names. */
    csmi_node_state_t state; /**< State of the node, see @ref csmi_node_state_t for details. */
    csm_bool ready; /**< Deprecated after CSM_VERSION_0_4_1. Query the 'ready' field in the database. API will ignore @ref CSM_UNDEF_BOOL for this field.  Valid values: 0,1,2. API checks for invalid values and fails if invalid values are passed. Database description: Is the node ready for workload manager? ('y' or 'n') */
    char* comment; /**< Update the 'comment' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Comment field for system administrators. Can be reset to NULL in CSM DB via \"#CSM_NULL\".*/
    char* feature_1; /**< Update the 'feature_1' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Reserved field for future use. Can be reset to NULL in CSM DB via \"#CSM_NULL\". */
    char* feature_2; /**< Update the 'feature_2' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Reserved field for future use. Can be reset to NULL in CSM DB via \"#CSM_NULL\". */
    char* feature_3; /**< Update the 'feature_3' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Reserved field for future use. Can be reset to NULL in CSM DB via \"#CSM_NULL\". */
    char* feature_4; /**< Update the 'feature_4' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Reserved field for future use. Can be reset to NULL in CSM DB via \"#CSM_NULL\". */
    char* physical_frame_location; /**< Update the 'physical_frame_location' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Physical frame number where the node is located */
    char* physical_u_location; /**< Update the 'physical_u_location' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Database description: Physical u location (position in the frame) where the node is located */
    char** node_names; /**< Identifies which nodes will be updated. Must contain at least one node_name. If left NULL, then API will exit early. Size defined by @ref node_names_count. */
} csm_node_attributes_update_input_t;
/**
 * @brief A wrapper for the output of @ref csm_node_attributes_update.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t failure_count; /**< The number of nodes which failed to be updated, size of @ref failure_node_names. */
    char** failure_node_names; /**< The names of the nodes which failed to be updated, size defined by @ref failure_count. */
} csm_node_attributes_update_output_t;
/**
 * @brief An input wrapper for @ref csm_node_delete.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t node_names_count; /**< Number of nodes to be deleted, size of @ref node_names. */
    char** node_names; /**< List of nodes to delete, one or more nodes must be specified. Size defined by @ref node_names_count. */
} csm_node_delete_input_t;
/**
 * @brief A wrapper for the output of @ref csm_node_delete.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t failure_count; /**< The number of nodes which failed to be deleted, size of @ref failure_node_names. */
    char** failure_node_names; /**< A list of nodes which failed to be deleted, size defined by @ref failure_count. */
} csm_node_delete_output_t;
/**
 * @brief An input wrapper for @ref csm_switch_attributes_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    uint32_t switch_names_count; /**< Number of switches being queried, size of @ref switch_names. */
    char* state; /**< Optionally filter results to only a specific state - active, rebooting, down, error (failed to reboot)-- according to Mellanox */
    char** switch_names; /**< List of switches to perform query on, must specify at least one switch. Size defined by @ref switch_names_count. */
    char* serial_number; /**< Optionally filter results to only a specific serial_number - unique identifier for a switch */
    char order_by; /**< Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY switch_name ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY switch_name ASC NULLS LAST', [b] =  'ORDER BY switch_name DESC NULLS LAST' */
} csm_switch_attributes_query_input_t;
/**
 *  @brief A wrapper for the output of @ref csm_switch_attributes_query.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count;  /**< Number of switch records, size of @ref results. */
    csmi_switch_record_t** results;  /**< List of switch records found by the query, size defined by @ref results_count. */
} csm_switch_attributes_query_output_t;
/**
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* switch_name; /**< Identifies which node will be queried. If left NULL, then API will fail.*/
} csm_switch_attributes_query_details_input_t;
/**
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int result_count; /**< Number of records returned from the SQL query, and length of the result array. only values this should return are 1 and 0. 1 = success, 0 = no records match. */
    csmi_switch_details_t** result; /**< A pointer to the record returned from the SQL query. NULL if no record found.*/
} csm_switch_attributes_query_details_output_t;
/**
 * @brief An input wrapper for @ref csm_switch_attributes_query_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    char* switch_name; /**< The switch name to perform the query on, must be specified. */
} csm_switch_attributes_query_history_input_t;
/**
 * @brief  A wrapper for the output of @ref csm_switch_attributes_query_history.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count;  /**< Number of records retrieved by the query, size of @ref results. */
    csmi_switch_history_record_t** results;  /**< The list of records retrieved by the query, size defined by @ref results_count. */
} csm_switch_attributes_query_history_output_t;
/**
 * @brief An input wrapper for @ref csm_switch_attributes_update.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t switch_names_count; /**< Number of switch records to update, size of @ref switch_names. */
    char* comment; /**< System administrator comment field for this switch. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Can be reset to NULL in CSM DB via \"#CSM_NULL\".*/
    char* physical_frame_location; /**< the frame where the switch is located. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. */
    char* physical_u_location; /**< the u number in the frame where the switch is located. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. */
    char* state; /**< Deprecated after CSM_VERSION_0_4_1. Update the 'state' field in the database. API will ignore NULL values for this field. if this field is left as NULL, then the API will keep the current value that is in the database. Valid Values: (active, error, missing, soft failure, service )*/
    char** switch_names; /**< List of switches to update in the database, one or more switches mus be specified. Size defined by @ref switch_names_count.*/
} csm_switch_attributes_update_input_t;
/**
 * @brief A wrapper for the output of @ref csm_switch_attributes_update.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t failure_count; /**< The number of failures which occurred in the update, size of @ref failure_switches. */
    char** failure_switches; /**< The list of switches that failed to be updated, size defined by @ref failure_count. */
} csm_switch_attributes_update_output_t;
/**
 * @brief An input wrapper for @ref csm_switch_inventory_collection.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t inventory_count; /**< The number of switch records to insert, size of @ref inventory. */
    csmi_switch_details_t** inventory; /**< A list of switch records to insert into the database, size defined by @ref inventory_count. */
} csm_switch_inventory_collection_input_t;
/**
 * @brief A wrapper for the output of @ref csm_switch_inventory_collection.
 * @todo Post-PRPQ: Implement
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char TBD; /**< TBD. */
} csm_switch_inventory_collection_output_t;
/**
 * @brief Used to contain the input parameters for the csm_switch_inventory_collection API.
 *
 * @author Nick Buonarota nbuonar@us.ibm.com
 * First included in PRPQ
 * Last edited: October 13, 2017
 *
 *
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int inventory_count; /**< Number of elements in the 'inventory' array. */
    csmi_switch_details_t** inventory; /**< An array of all the inventory to be inserted into the CSM database. */
} csm_switch_children_inventory_collection_input_t;
/**
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char TBD; /**< TBD. */
} csm_switch_children_inventory_collection_output_t;
/**
 * @brief An input wrapper for @ref csm_cluster_query_state.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t num_allocs; /**< Filter query by the 'num_allocs' field in the database.. API will ignore values less than 0.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    char order_by; /**< Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY node_name ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY node_name ASC NULLS LAST', [b] =  'ORDER BY node_name DESC NULLS LAST', [c] = 'ORDER BY state ASC NULLS LAST', [d] =  'ORDER BY state DESC NULLS LAST', [e] = 'ORDER BY type ASC NULLS LAST', [f] =  'ORDER BY type DESC NULLS LAST', [g] = 'ORDER BY num_allocs ASC NULLS LAST', [h] =  'ORDER BY num_allocs DESC NULLS LAST'. */
    csmi_node_state_t state; /**< Query the 'state' field in the database. API will ignore @ref csmi_node_state_t::CSM_NODE_NO_DEF values for this fields, see @ref csmi_node_state_t for details.*/
    csmi_node_type_t type; /**< Query the 'type' field in the database. API will ignore @ref csmi_node_type_t::CSM_NODE_NO_TYPE values for this fields, see @ref csmi_node_type_t for details.*/
} csm_cluster_query_state_input_t;
/**
 * @brief A wrapper for the output of @ref csm_cluster_query_state.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of records retrieved, size of @ref results. */
    csmi_cluster_query_state_record_t** results; /**< A list of records retrieved from the queries, size defined by @ref results_count.*/
} csm_cluster_query_state_output_t;
/** @} */

#ifdef __cplusplus
}
#endif
#endif
