/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/InvGetNodeInventory.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "InvGetNodeInventory.h"
#include "include/csm_event_type_definitions.h"

#ifndef logprefix
#define logprefix "INV_MASTER"
#define logprefix_local
#endif
#include "csm_pretty_log.h"


#include "csmd/src/inv/include/inv_get_node_inventory.h"
#include "csmd/src/inv/include/inv_get_node_inventory_serialization.h"
#include "csmi/include/csm_api_consts.h"

#include <sstream>

using std::string;
using std::to_string;
using std::endl;

// TODO REMOVE THIS
typedef struct {
    char *nodeName;
    uint32_t numCpus;
} csm_node_attributes_test_t;

void
InvGetNodeInventory::Process( const csm::daemon::CoreEvent &aEvent,
                              std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  /* todo: this is a temporary workaround to incrementally add the utility node inventory to the master's DaemonState::_NodeStatusMap
   * to avoid a lot of rewrite and copy of csmi_db_base function, we do some duplicate checking and event data extraction here
   * but eventually just call CSMI_DB_BASE::Process()
   */

  // only try to extract the data if this is a db response
  // we only want to add utility/aggregator node info if the db access was successful
  if( isDBRespEvent(aEvent) )
  {
    EventContextDBBase_sptr ctx = std::dynamic_pointer_cast<EventContextDBBase> (aEvent.GetEventContext() );

    if( ( ctx != nullptr ) && ( ctx->GetAuxiliaryId() == db_access_stages::RCV_DB))
    {
      // add the inventory msg regardless of the db result because this is for connection book keeping and not for the API queries
      csm::daemon::NetworkEvent *ev = dynamic_cast<csm::daemon::NetworkEvent*>( ctx->GetReqEvent() );
      csm::network::MessageAndAddress reqContent = ev->GetContent();

      csm::network::Address_sptr addr = reqContent.GetAddr();
      if(( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_UTILITY ) ||
          ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR ))
      {
        // if the response was good, then we add the utility or aggregator node's data
        csm::daemon::DaemonStateMaster *daemonState = dynamic_cast<csm::daemon::DaemonStateMaster*>( GetDaemonState() );

        // Try to unpack the csm_node_inventory_t structure and add a node name to the node-Info
        csm_full_inventory_t inventory;
        uint32_t bytes_unpacked;

        bytes_unpacked = get_node_inventory_unpack(reqContent._Msg.GetData(), inventory);
        if( bytes_unpacked )
        {
          // Filter out compute inventory received on aggregator connections 
          // by confirming that the addr type matches the inventory type
          const string inv_type(inventory.node.type);
          if( (( inv_type == CSM_NODE_TYPE_AGGREGATOR ) && 
               ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR )) ||
              (( inv_type == CSM_NODE_TYPE_UTILITY ) && 
               ( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_UTILITY )) )
          {
            daemonState->SetNodeInfo( addr, inventory.node.node_name, &reqContent._Msg );
            NodeConnectRasEvent( inventory.node.node_name, postEventList );
          }
        }
      }
    }
  }
  
  if ( isSystemEvent( aEvent ) )
  {
    switch( getSystemEventType( aEvent ) )
    {
      case csm::daemon::SystemContent::DISCONNECTED:
      {
        const csm::network::Address_sptr addr = GetAddrFromSystemEvent(aEvent);
        csm::daemon::ConnectedNodeStatus *nInfo = GetDaemonState()->GetNodeInfo( addr );

        if( nInfo == nullptr )
        {
          CSMLOG( csmd, error ) << "Disconnected event without peer address. Ignoring...";
          break;
        }
        CSMLOG(csmd, error) << "InvGetNodeInventory: detected "  << nInfo->_ConnectionType
            << " connection failure of address type: " << addr->GetAddrType();
        NodeDisconnectRasEvent(addr, postEventList);
        break;
      }

      default:
        CSMLOG( csmd, warning ) << "InvGetNodeInventory: unexpected system event type.";
    }
  }
  else
  { 
    // go through the regular process function
    // Note: CSMI_DB_BASE::Process() will throw an exception if called to process a SystemEvent 
    CSMI_DB_BASE::Process( aEvent, postEventList );
  }
}


bool InvGetNodeInventory::CreateSqlStmt(const std::string& arguments, const uint32_t len,
                std::string &stmt, int &errcode, std::string &errmsg, bool compareDataForPrivateCheckRes)
{
   CSMLOG(csmd, debug) << "Enter " << __PRETTY_FUNCTION__;
 
   static const string NODE_TABLE_NAME("csm_node");
   static const string DIMM_TABLE_NAME("csm_dimm");
   static const string GPU_TABLE_NAME("csm_gpu");
   static const string HCA_TABLE_NAME("csm_hca");
   static const string SSD_TABLE_NAME("csm_ssd");
   static const string PROCESSOR_TABLE_NAME("csm_processor_socket");
      
   // Try to unpack the csm_node_inventory_t structure
   csm_full_inventory_t inventory;
   uint32_t bytes_unpacked;

   bytes_unpacked = get_node_inventory_unpack(arguments, inventory);

   if ( bytes_unpacked == 0 ) 
   {
      CSMLOG(csmd, error) << "CreateSqlStmt: failed to unpack new inventory message.";
      errcode = CSMERR_MSG_UNPACK_ERROR;
      errmsg.append("unpackFunc failed");
      return false;
   }
   else
   {
      const string node_name(inventory.node.node_name);
      CSMLOG(csmd, info) << "Received new inventory from " << node_name;
      
      const string NODE_INV = string("NODE_INV: ") + node_name + "      ";
      CSMLOGp(csmd, info, NODE_INV) << "node_name:             " << inventory.node.node_name;
      CSMLOGp(csmd, info, NODE_INV) << "machine_model:         " << inventory.node.machine_model;
      CSMLOGp(csmd, info, NODE_INV) << "serial_number:         " << inventory.node.serial_number;
      CSMLOGp(csmd, info, NODE_INV) << "collection_time:       " << inventory.node.collection_time;
      CSMLOGp(csmd, info, NODE_INV) << "kernel_release:        " << inventory.node.kernel_release;
      CSMLOGp(csmd, info, NODE_INV) << "kernel_version:        " << inventory.node.kernel_version;
      CSMLOGp(csmd, info, NODE_INV) << "installed_memory:      " << inventory.node.installed_memory;
      CSMLOGp(csmd, info, NODE_INV) << "installed_swap:        " << inventory.node.installed_swap;
      CSMLOGp(csmd, info, NODE_INV) << "discovered_sockets:    " << inventory.node.discovered_sockets;
      CSMLOGp(csmd, info, NODE_INV) << "discovered_cores:      " << inventory.node.discovered_cores;
      CSMLOGp(csmd, info, NODE_INV) << "discovered_gpus:       " << inventory.node.discovered_gpus;
      CSMLOGp(csmd, info, NODE_INV) << "discovered_hcas:       " << inventory.node.discovered_hcas;
      CSMLOGp(csmd, info, NODE_INV) << "discovered_dimms:      " << inventory.node.discovered_dimms;
      CSMLOGp(csmd, info, NODE_INV) << "discovered_ssds:       " << inventory.node.discovered_ssds;
      CSMLOGp(csmd, info, NODE_INV) << "os_image_name:         " << inventory.node.os_image_name;
      CSMLOGp(csmd, info, NODE_INV) << "os_image_uuid:         " << inventory.node.os_image_uuid;
      
      std::ostringstream stmt_out;
 
      // We need to treat this whole update as a single transaction
      stmt_out << "BEGIN;" << endl;

      // Create a temporary table containing the current node_name and collection_time
      // to use for the rest of this transaction
      // This is needed because each of the individual updates needs to know
      // what the current timestamp is, but the timestamp gets updated as part
      // of the transaction
      stmt_out << "CREATE TEMP TABLE csm_node_temp ON COMMIT DROP AS "; 
      stmt_out << "SELECT node_name, collection_time FROM csm_node "; 
      stmt_out << "WHERE node_name='" << inventory.node.node_name << "' "; 
      stmt_out << "AND collection_time!='" << inventory.node.collection_time << "';" << endl;
   
      // We need to first try to update an existing row, 
      // and if that fails instead insert a new row
      // Only insert if the collection_time has changed
      // Do not touch the state as part of an update 
      // The first part of the stmt is the row update:
      // UPDATE csm_node SET machine_model='8335-GTA', serial_number='210317A', 
      // collection_time='2016-05-24 10:09:12.843910',
      // type='compute', kernel_release='3.10.0-327.el7.ppc64le', 
      // kernel_version='#1 SMP Thu Oct 29 17:31:13 EDT 2015', installed_memory=263655424, 
      // installed_swap=4194240, discovered_sockets=2, discovered_cores=20, 
      // discovered_gpus=4, discovered_hcas=1 
      // WHERE node_name='c931f04p16' AND collection_time!='2016-05-24 10:09:12.843910';
      stmt_out << "UPDATE " << NODE_TABLE_NAME << " SET ";
      stmt_out << "machine_model='" << inventory.node.machine_model << "', ";
      stmt_out << "serial_number='" << inventory.node.serial_number << "', ";
      stmt_out << "collection_time='" << inventory.node.collection_time << "', ";
      stmt_out << "type='" << inventory.node.type << "', ";
      stmt_out << "kernel_release='" << inventory.node.kernel_release << "', ";
      stmt_out << "kernel_version='" << inventory.node.kernel_version << "', ";
      stmt_out << "installed_memory=" << inventory.node.installed_memory << ", ";
      stmt_out << "installed_swap=" << inventory.node.installed_swap << ", ";
      stmt_out << "discovered_sockets=" << inventory.node.discovered_sockets << ", ";
      stmt_out << "discovered_cores=" << inventory.node.discovered_cores << ", ";
      stmt_out << "discovered_gpus=" << inventory.node.discovered_gpus << ", ";
      stmt_out << "discovered_hcas=" << inventory.node.discovered_hcas << ", ";
      stmt_out << "discovered_dimms=" << inventory.node.discovered_dimms << ", ";
      stmt_out << "discovered_ssds=" << inventory.node.discovered_ssds << ", ";
      stmt_out << "os_image_name='" << inventory.node.os_image_name << "', ";
      stmt_out << "os_image_uuid='" << inventory.node.os_image_uuid << "' ";
      stmt_out << "WHERE node_name='" << inventory.node.node_name << "' "; 
      stmt_out << "AND collection_time!='" << inventory.node.collection_time << "';" << endl;
      
      // The second part of the stmt is the insert, in case this is the first time this node has been seen:
      // Only set the state if this is the first time the node has been seen
      // INSERT INTO csm_node (node_name, machine_model, serial_number, collection_time, state, 
      // type, kernel_release, kernel_version, installed_memory, installed_swap, discovered_sockets, 
      // discovered_cores, discovered_gpus, discovered_hcas) 
      // SELECT 'c931f04p16', '8335-GTA', '210317A', 
      // '2016-05-24 10:09:12.843910', 'DISCOVERED', 'compute', '3.10.0-327.el7.ppc64le', 
      // '#1 SMP Thu Oct 29 17:31:13 EDT 2015', 263655424, 4194240, 2, 20, 4, 1  
      // WHERE NOT EXISTS (SELECT 1 FROM csm_node WHERE node_name='c931f04p16');
 
      // First build the keys and values parts of the insert
      std::ostringstream keys_out;
      std::ostringstream values_out;
      keys_out << "(node_name, ";           values_out << "'" << inventory.node.node_name << "', ";
      keys_out << "machine_model, ";        values_out << "'" << inventory.node.machine_model << "', ";
      keys_out << "serial_number, ";        values_out << "'" << inventory.node.serial_number << "', ";
      keys_out << "collection_time, ";      values_out << "'" << inventory.node.collection_time << "', ";
      keys_out << "state, ";                values_out << "'" << CSM_NODE_STATE_DISCOVERED << "', ";
      keys_out << "type, ";                 values_out << "'" << inventory.node.type << "', ";
      keys_out << "kernel_release, ";       values_out << "'" << inventory.node.kernel_release << "', ";
      keys_out << "kernel_version, ";       values_out << "'" << inventory.node.kernel_version << "', ";
      keys_out << "installed_memory, ";     values_out << inventory.node.installed_memory << ", ";
      keys_out << "installed_swap, ";       values_out << inventory.node.installed_swap << ", ";
      keys_out << "discovered_sockets, ";   values_out << inventory.node.discovered_sockets << ", ";
      keys_out << "discovered_cores, ";     values_out << inventory.node.discovered_cores << ", ";
      keys_out << "discovered_gpus, ";      values_out << inventory.node.discovered_gpus << ", ";
      keys_out << "discovered_hcas, ";      values_out << inventory.node.discovered_hcas << ", ";
      keys_out << "discovered_dimms, ";     values_out << inventory.node.discovered_dimms << ", ";
      keys_out << "discovered_ssds, ";      values_out << inventory.node.discovered_ssds << ", ";
      keys_out << "os_image_name, ";        values_out << "'" << inventory.node.os_image_name << "', ";
      keys_out << "os_image_uuid)";         values_out << "'" << inventory.node.os_image_uuid << "' ";
      
      // Now build the statement
      stmt_out << "INSERT INTO " << NODE_TABLE_NAME << " " << keys_out.str();
      stmt_out << " SELECT " << values_out.str(); 
      stmt_out << " WHERE NOT EXISTS (SELECT 1 FROM " << NODE_TABLE_NAME;
      stmt_out << " WHERE node_name='" << inventory.node.node_name << "');" << endl;
      
      // Used to delete any old rows from the csm_dimm table
      std::ostringstream dimmserials_out;

      // Update/Insert the DIMM inventory
      for ( uint32_t i = 0; i < inventory.node.discovered_dimms; i++ )
      {
        const string DIMM_INV = "DIMM_INV: " + node_name + "-dimm" + to_string(i);
        CSMLOGp(csmd, info, DIMM_INV) << " - serial_number:         " << inventory.dimm[i].serial_number;
        CSMLOGp(csmd, info, DIMM_INV) << "   size:                  " << inventory.dimm[i].size;
        CSMLOGp(csmd, info, DIMM_INV) << "   physical_location:     " << inventory.dimm[i].physical_location;
      
        // Build the list of dimm serial numbers in the current inventory
        if ( !dimmserials_out.str().empty() )
        {
          dimmserials_out << ", ";  // Add a comma between serial numbers
        }
        dimmserials_out << "'" << inventory.dimm[i].serial_number << "'";

        // Build the update statement 
        stmt_out << "UPDATE " << DIMM_TABLE_NAME << " SET ";
        stmt_out << "size='" << inventory.dimm[i].size << "', ";
        stmt_out << "physical_location='" << inventory.dimm[i].physical_location << "'";
        stmt_out << " WHERE node_name IN (SELECT t.node_name FROM csm_node_temp t WHERE ";
        stmt_out << "t.collection_time!='" << inventory.node.collection_time << "') "; 
        stmt_out << "AND serial_number='" << inventory.dimm[i].serial_number << "';" << endl;
      
        // Build the insert statement
        // First build the keys and values parts of the insert
        // Clear the keys and values first
        keys_out.str("");
        keys_out.clear();
        values_out.str("");
        values_out.clear();

        keys_out << "(node_name, ";           values_out << "'" << inventory.node.node_name << "', ";
        keys_out << "serial_number, ";        values_out << "'" << inventory.dimm[i].serial_number << "', ";
        keys_out << "size, ";                 values_out << "'" << inventory.dimm[i].size  << "', ";
        keys_out << "physical_location)";     values_out << "'" << inventory.dimm[i].physical_location << "'";
        
        // Now build the statement
        stmt_out << "INSERT INTO " << DIMM_TABLE_NAME << " " << keys_out.str();
        stmt_out << " SELECT " << values_out.str(); 
        stmt_out << " WHERE NOT EXISTS (SELECT 1 FROM " << DIMM_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number='" << inventory.dimm[i].serial_number << "');" << endl;
      }
     
      // Delete any DIMMs that are no longer present in the current inventory
      if ( dimmserials_out.str().empty() )
      {
        // No DIMMs in current inventory, delete all existing rows for this node
        stmt_out << "DELETE FROM " << DIMM_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "';" << endl;
      }
      else
      {
        // Only delete rows that do not have records in the current inventory
        stmt_out << "DELETE FROM " << DIMM_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number NOT IN (" << dimmserials_out.str() << ");" << endl;
      }

      // Used to delete any old rows from the csm_gpu table
      std::ostringstream gpuids_out;

      // Update/Insert the GPU inventory
      for ( uint32_t i = 0; i < inventory.node.discovered_gpus; i++ )
      {
        const string GPU_INV = " GPU_INV: " + node_name + "-gpu" + to_string(i) + " ";
        CSMLOGp(csmd, info, GPU_INV) << " - gpu_id:                " << inventory.gpu[i].gpu_id;
        CSMLOGp(csmd, info, GPU_INV) << "   device_name:           " << inventory.gpu[i].device_name;
        CSMLOGp(csmd, info, GPU_INV) << "   pci_bus_id:            " << inventory.gpu[i].pci_bus_id;
        CSMLOGp(csmd, info, GPU_INV) << "   serial_number:         " << inventory.gpu[i].serial_number;
        CSMLOGp(csmd, info, GPU_INV) << "   uuid:                  " << inventory.gpu[i].uuid;
        CSMLOGp(csmd, info, GPU_INV) << "   vbios:                 " << inventory.gpu[i].vbios;
        CSMLOGp(csmd, info, GPU_INV) << "   inforom_image_version: " << inventory.gpu[i].inforom_image_version;
        CSMLOGp(csmd, info, GPU_INV) << "   hbm_memory:            " << inventory.gpu[i].hbm_memory;
      
        // Build the list of gpuids in the current inventory
        if ( !gpuids_out.str().empty() )
        {
          gpuids_out << ", ";  // Add a comma between ids
        }
        gpuids_out << inventory.gpu[i].gpu_id;

        // Build the update statement 
        stmt_out << "UPDATE " << GPU_TABLE_NAME << " SET ";
	stmt_out << "device_name='" << inventory.gpu[i].device_name << "', ";
	stmt_out << "pci_bus_id='" << inventory.gpu[i].pci_bus_id << "', ";
        stmt_out << "serial_number='" << inventory.gpu[i].serial_number << "', ";
        stmt_out << "uuid='" << inventory.gpu[i].uuid << "', ";
        stmt_out << "vbios='" << inventory.gpu[i].vbios << "', ";
        stmt_out << "inforom_image_version='" << inventory.gpu[i].inforom_image_version << "', ";
        stmt_out << "hbm_memory='" << inventory.gpu[i].hbm_memory << "'";
        stmt_out << " WHERE node_name IN (SELECT t.node_name FROM csm_node_temp t WHERE ";
        stmt_out << "t.collection_time!='" << inventory.node.collection_time << "') "; 
        stmt_out << "AND gpu_id=" << inventory.gpu[i].gpu_id << ";" << endl;
      
        // Build the insert statement
        // First build the keys and values parts of the insert
        // Clear the keys and values first
        keys_out.str("");
        keys_out.clear();
        values_out.str("");
        values_out.clear();

        keys_out << "(node_name, ";           values_out << "'" << inventory.node.node_name << "', ";
        keys_out << "gpu_id, ";               values_out << inventory.gpu[i].gpu_id << ", ";
        keys_out << "device_name, ";          values_out << "'" << inventory.gpu[i].device_name << "', ";
        keys_out << "pci_bus_id, ";           values_out << "'" << inventory.gpu[i].pci_bus_id  << "', ";
        keys_out << "serial_number, ";        values_out << "'" << inventory.gpu[i].serial_number << "', ";
        keys_out << "uuid, ";                 values_out << "'" <<inventory.gpu[i].uuid  << "', ";
        keys_out << "vbios, ";                values_out << "'" <<inventory.gpu[i].vbios  << "', ";
        keys_out << "inforom_image_version,"; values_out << "'" << inventory.gpu[i].inforom_image_version << "', ";
        keys_out << "hbm_memory)";            values_out << "'" << inventory.gpu[i].hbm_memory << "'";
        
        // Now build the statement
        stmt_out << "INSERT INTO " << GPU_TABLE_NAME << " " << keys_out.str();
        stmt_out << " SELECT " << values_out.str(); 
        stmt_out << " WHERE NOT EXISTS (SELECT 1 FROM " << GPU_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND gpu_id=" << inventory.gpu[i].gpu_id << ");" << endl;
      }
     
      // Delete any GPUs that are no longer present in the current inventory
      if ( gpuids_out.str().empty() )
      {
        // No GPUs in current inventory, delete all existing rows for this node
        stmt_out << "DELETE FROM " << GPU_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "';" << endl; 
      }
      else
      {
        // Only delete rows that do not have records in the current inventory
        stmt_out << "DELETE FROM " << GPU_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND gpu_id NOT IN (" << gpuids_out.str() << ");" << endl;
      }
      
      // Used to delete any old rows from the csm_hca table (using hca serial numbers)
      std::ostringstream hca_serials_out;

      // Update/Insert the HCA inventory
      for ( uint32_t i = 0; i < inventory.node.discovered_hcas; i++ )
      {
        const string HCA_INV = " HCA_INV: " + node_name + "-hca" + to_string(i) + " ";
        CSMLOGp(csmd, info, HCA_INV) << " - serial_number:         " << inventory.hca[i].serial_number;
        CSMLOGp(csmd, info, HCA_INV) << "   device_name:           " << inventory.hca[i].device_name;
        CSMLOGp(csmd, info, HCA_INV) << "   pci_bus_id:            " << inventory.hca[i].pci_bus_id;
        CSMLOGp(csmd, info, HCA_INV) << "   guid:                  " << inventory.hca[i].guid;
        CSMLOGp(csmd, info, HCA_INV) << "   part_number:           " << inventory.hca[i].part_number;
        CSMLOGp(csmd, info, HCA_INV) << "   fw_ver:                " << inventory.hca[i].fw_ver;
        CSMLOGp(csmd, info, HCA_INV) << "   hw_rev:                " << inventory.hca[i].hw_rev;
        CSMLOGp(csmd, info, HCA_INV) << "   board_id:              " << inventory.hca[i].board_id;
        //CSMLOGp(csmd, info, HCA_INV) << "   switch_guid_port_1:    " << inventory.hca[i].switch_guid_port_1;
        //CSMLOGp(csmd, info, HCA_INV) << "   switch_guid_port_2:    " << inventory.hca[i].switch_guid_port_2;
        
        // Build the list of hca serial numbers in the current inventory
        if ( !hca_serials_out.str().empty() )
        {
          hca_serials_out << ", ";  // Add a comma between ids
        }
        hca_serials_out << "'" << inventory.hca[i].serial_number << "'";

        // Build the update statement 
        stmt_out << "UPDATE " << HCA_TABLE_NAME << " SET ";
	stmt_out << "device_name='" << inventory.hca[i].device_name << "', ";
	stmt_out << "pci_bus_id='" << inventory.hca[i].pci_bus_id << "', ";
        stmt_out << "guid='" << inventory.hca[i].guid << "', ";
        stmt_out << "part_number='" << inventory.hca[i].part_number << "', ";
        stmt_out << "fw_ver='" << inventory.hca[i].fw_ver << "', ";
        stmt_out << "hw_rev='" << inventory.hca[i].hw_rev << "', ";
        stmt_out << "board_id='" << inventory.hca[i].board_id << "'";
        stmt_out << " WHERE node_name IN (SELECT t.node_name FROM csm_node_temp t WHERE ";
        stmt_out << "t.collection_time!='" << inventory.node.collection_time << "') "; 
        stmt_out << "AND serial_number='" << inventory.hca[i].serial_number << "';" << endl;
      
        // Build the insert statement
        // First build the keys and values parts of the insert
        // Clear the keys and values first
        keys_out.str("");
        keys_out.clear();
        values_out.str("");
        values_out.clear();
        
        keys_out << "(node_name, ";           values_out << "'" << inventory.node.node_name << "', ";
        keys_out << "serial_number, ";        values_out << "'" << inventory.hca[i].serial_number << "', ";
        keys_out << "device_name, ";          values_out << "'" << inventory.hca[i].device_name << "', ";
        keys_out << "pci_bus_id, ";           values_out << "'" << inventory.hca[i].pci_bus_id  << "', ";
        keys_out << "guid, ";                 values_out << "'" << inventory.hca[i].guid  << "', ";
        keys_out << "part_number, ";          values_out << "'" << inventory.hca[i].part_number  << "', ";
        keys_out << "fw_ver, ";               values_out << "'" << inventory.hca[i].fw_ver  << "', ";
        keys_out << "hw_rev, ";               values_out << "'" << inventory.hca[i].hw_rev  << "', ";
        keys_out << "board_id) ";             values_out << "'" << inventory.hca[i].board_id  << "'";
        
        // Now build the statement
        stmt_out << "INSERT INTO " << HCA_TABLE_NAME << " " << keys_out.str();
        stmt_out << " SELECT " << values_out.str(); 
        stmt_out << " WHERE NOT EXISTS (SELECT 1 FROM " << HCA_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number='" << inventory.hca[i].serial_number << "');" << endl;
      }
      
      // Delete any HCAs that are no longer present in the current inventory
      if ( hca_serials_out.str().empty() )
      {
        // No HCAs in current inventory, delete all existing rows for this node
        stmt_out << "DELETE FROM " << HCA_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "';" << endl; 
      }
      else
      {
        // Only delete rows that do not have records in the current inventory
        stmt_out << "DELETE FROM " << HCA_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number NOT IN (" << hca_serials_out.str() << ");" << endl;
      }

      // Used to delete any old rows from the csm_ssd table (using ssd serial numbers)
      std::ostringstream ssd_serials_out;

      // Update/Insert the SSD inventory
      for ( uint32_t i = 0; i < inventory.node.discovered_ssds; i++ )
      {
        const string SSD_INV = " SSD_INV: " + node_name + "-ssd" + to_string(i) + " ";
        CSMLOGp(csmd, info, SSD_INV) << " - serial_number:                 " << inventory.ssd[i].serial_number;
        CSMLOGp(csmd, info, SSD_INV) << "   device_name:                   " << inventory.ssd[i].device_name;
        CSMLOGp(csmd, info, SSD_INV) << "   pci_bus_id:                    " << inventory.ssd[i].pci_bus_id;
        CSMLOGp(csmd, info, SSD_INV) << "   fw_ver:                        " << inventory.ssd[i].fw_ver;
        CSMLOGp(csmd, info, SSD_INV) << "   size:                          " << inventory.ssd[i].size;
        // TODO: Add these to inventory collection 
        //CSMLOGp(csmd, info, SSD_INV) << "   wear_lifespan_used:            " << inventory.ssd[i].wear_lifespan_used;
        //CSMLOGp(csmd, info, SSD_INV) << "   wear_total_bytes_written:      " << inventory.ssd[i].wear_total_bytes_written;
        //CSMLOGp(csmd, info, SSD_INV) << "   wear_total_bytes_read:         " << inventory.ssd[i].wear_total_bytes_read;
        //CSMLOGp(csmd, info, SSD_INV) << "   wear_percent_spares_remaining: " << inventory.ssd[i].wear_percent_spares_remaining;
        
        // Build the list of ssd serial numbers in the current inventory
        if ( !ssd_serials_out.str().empty() )
        {
          ssd_serials_out << ", ";  // Add a comma between ids
        }
        ssd_serials_out << "'" << inventory.ssd[i].serial_number << "'";

        // Build the update statement 
        stmt_out << "UPDATE " << SSD_TABLE_NAME << " SET ";
	stmt_out << "device_name='" << inventory.ssd[i].device_name << "', ";
	stmt_out << "pci_bus_id='" << inventory.ssd[i].pci_bus_id << "', ";
        stmt_out << "fw_ver='" << inventory.ssd[i].fw_ver << "', ";
        stmt_out << "update_time='" << inventory.node.collection_time << "', ";
        stmt_out << "size=" << inventory.ssd[i].size << ", ";
        // Defaulting wear_lifespan_used to 0 for PRPQ
        // TODO: Replace this with real data
        stmt_out << "wear_lifespan_used=0 ";
        stmt_out << " WHERE node_name IN (SELECT t.node_name FROM csm_node_temp t WHERE ";
        stmt_out << "t.collection_time!='" << inventory.node.collection_time << "') "; 
        stmt_out << "AND serial_number='" << inventory.ssd[i].serial_number << "';" << endl;
      
        // Build the insert statement
        // First build the keys and values parts of the insert
        // Clear the keys and values first
        keys_out.str("");
        keys_out.clear();
        values_out.str("");
        values_out.clear();
        
        keys_out << "(node_name, ";           values_out << "'" << inventory.node.node_name << "', ";
        keys_out << "serial_number, ";        values_out << "'" << inventory.ssd[i].serial_number << "', ";
        keys_out << "update_time, ";          values_out << "'" << inventory.node.collection_time << "', ";
        keys_out << "device_name, ";          values_out << "'" << inventory.ssd[i].device_name << "', ";
        keys_out << "pci_bus_id, ";           values_out << "'" << inventory.ssd[i].pci_bus_id  << "', ";
        keys_out << "fw_ver, ";               values_out << "'" << inventory.ssd[i].fw_ver  << "', ";
        keys_out << "size, ";                 values_out << inventory.ssd[i].size << ", ";
        // Defaulting wear_lifespan_used to 0 for PRPQ
        // TODO: Replace this with real data
        keys_out << "wear_lifespan_used) ";   values_out << 0;
        
        // Now build the statement
        stmt_out << "INSERT INTO " << SSD_TABLE_NAME << " " << keys_out.str();
        stmt_out << " SELECT " << values_out.str(); 
        stmt_out << " WHERE NOT EXISTS (SELECT 1 FROM " << SSD_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number='" << inventory.ssd[i].serial_number << "');" << endl;
      }
      
      // Delete any SSDs that are no longer present in the current inventory
      if ( ssd_serials_out.str().empty() )
      {
        // No SSDs in current inventory, delete all existing rows for this node
        stmt_out << "DELETE FROM " << SSD_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "';" << endl; 
      }
      else
      {
        // Only delete rows that do not have records in the current inventory
        stmt_out << "DELETE FROM " << SSD_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number NOT IN (" << ssd_serials_out.str() << ");" << endl;
      }
      
      // Used to delete any old rows from the csm_processor_socket table
      std::ostringstream procserials_out;

      // Update/Insert the processor inventory
      for ( uint32_t i = 0; i < inventory.processor_count; i++ )
      {
        const string PROC_INV = "PROC_INV: " + node_name + "-proc" + to_string(i);
        CSMLOGp(csmd, info, PROC_INV) << " - serial_number:         " << inventory.processor[i].serial_number;
        CSMLOGp(csmd, info, PROC_INV) << "   physical_location:     " << inventory.processor[i].physical_location;
        CSMLOGp(csmd, info, PROC_INV) << "   discovered_cores:      " << inventory.processor[i].discovered_cores;
      
        // Build the list of processor serial numbers in the current inventory
        if ( !procserials_out.str().empty() )
        {
          procserials_out << ", ";  // Add a comma between serial numbers
        }
        procserials_out << "'" << inventory.processor[i].serial_number << "'";

        // Build the update statement 
        stmt_out << "UPDATE " << PROCESSOR_TABLE_NAME << " SET ";
        stmt_out << "physical_location='" << inventory.processor[i].physical_location << "', ";
        stmt_out << "discovered_cores='"  << inventory.processor[i].discovered_cores << "'";
        stmt_out << " WHERE node_name IN (SELECT t.node_name FROM csm_node_temp t WHERE ";
        stmt_out << "t.collection_time!='" << inventory.node.collection_time << "') "; 
        stmt_out << "AND serial_number='" << inventory.processor[i].serial_number << "';" << endl;
      
        // Build the insert statement
        // First build the keys and values parts of the insert
        // Clear the keys and values first
        keys_out.str("");
        keys_out.clear();
        values_out.str("");
        values_out.clear();

        keys_out << "(node_name, ";           values_out << "'" << inventory.node.node_name << "', ";
        keys_out << "serial_number, ";        values_out << "'" << inventory.processor[i].serial_number << "', ";
        keys_out << "physical_location, ";    values_out << "'" << inventory.processor[i].physical_location  << "', ";
        keys_out << "discovered_cores)";      values_out << "'" << inventory.processor[i].discovered_cores << "'";
        
        // Now build the statement
        stmt_out << "INSERT INTO " << PROCESSOR_TABLE_NAME << " " << keys_out.str();
        stmt_out << " SELECT " << values_out.str(); 
        stmt_out << " WHERE NOT EXISTS (SELECT 1 FROM " << PROCESSOR_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number='" << inventory.processor[i].serial_number << "');" << endl;
      }
     
      // Delete any processsors that are no longer present in the current inventory
      if ( procserials_out.str().empty() )
      {
        // No processors in current inventory, delete all existing rows for this node
        stmt_out << "DELETE FROM " << PROCESSOR_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "';" << endl;
      }
      else
      {
        // Only delete rows that do not have records in the current inventory
        stmt_out << "DELETE FROM " << PROCESSOR_TABLE_NAME;
        stmt_out << " WHERE node_name='" << inventory.node.node_name << "' "; 
        stmt_out << "AND serial_number NOT IN (" << procserials_out.str() << ");" << endl;
      }

      // We need to treat this whole update as a single transaction
      stmt_out << "COMMIT;";

      //CSMLOG(csmd, info) << "update_stmt_out: " << stmt_out.str();
      stmt = stmt_out.str();
   }
 
   CSMLOG(csmd, debug) << "CreateSqlStmt: " << stmt;
   CSMLOG(csmd, debug) << "Exit " << __PRETTY_FUNCTION__;
   errcode = CSMI_SUCCESS;
   return true;
}

// will return csmi defined error code
int InvGetNodeInventory::CreateByteArray(std::vector<csm::db::DBTuple *>&tuples,
                                          char **buf, uint32_t &bufLen, bool compareDataForPrivateCheckRes)
{
    CSMLOG(csmd, debug) << "Enter " << __PRETTY_FUNCTION__;
    
    CSMLOG(csmd, debug) << "tuples.size() = " << tuples.size();
    for (uint32_t i = 0; i < tuples.size(); i++)
    {
      CSMLOG(csmd, info) << "tuples[" << i << "]->nfields = " << tuples[i]->nfields;
      for (int32_t j = 0; j < tuples[i]->nfields; j++)
      {
        CSMLOG(csmd, info) << "tuples[" << i << "][" << j << "] " << tuples[i]->data[j];
      }
    }
    
    *buf = NULL;
    bufLen = 0;
    int errcode = CSMI_SUCCESS;

    uint32_t nrows = tuples.size();
    if (nrows == 0)
    {
      // no need to do anything here. Just return the SUCCESS code
      CSMLOG(csmd, info) << "Successfully inserted new inventory into DB, nrows=" << nrows;
      return errcode;
    }
    else
    {
      CSMLOG(csmd, warning) << "Unexpected response from DB when inserting new inventory, nrows=" << nrows;
    }
 
    int expectedCols = 2;
    uint32_t idx;

    if (_packFunc == NULL) {
      CSMLOG(csmd, error) << "ConvertTupleToBuf: _packFunc NULL";
      *buf = csmi_err_pack(CSMERR_MSG_PACK_ERROR, "packFunc is NULL", &bufLen);
      return CSMERR_MSG_PACK_ERROR;
     
    }

    // convert from DB tuple results to c data structure
    csm_node_attributes_test_t *list = (csm_node_attributes_test_t *) malloc(sizeof(csm_node_attributes_test_t)*nrows);
    for (idx=0; idx<nrows; idx++) {
        csm::db::DBTuple* fields = tuples[idx];

        // to-do: need to change this checking
        if (fields->nfields != expectedCols) break;

        list[idx].nodeName = strdup(fields->data[0]);
        list[idx].numCpus = atoi(fields->data[1]);

    }

    // something wrong in the DB table
    if (idx  != nrows ) {
      errcode = CSMERR_DB_ERROR;
      *buf = csmi_err_pack(CSMERR_DB_ERROR, "Unexpected Number of Columns", &bufLen);
       
    } else {
      // packing the c data structure to a buffer
      csmi_cdata_t csmi_cdata;
      csmi_cdata.cdata = list;
      csmi_cdata.cdataLen = nrows;

      csmi_buf_t *retVal = _packFunc(_cmdType, &csmi_cdata);
      if (retVal == NULL) {
           CSMLOG(csmd,error) << "ConvertTupleToBuf: packFunc failed...";
           errcode = CSMERR_MSG_PACK_ERROR;
           *buf = csmi_err_pack(CSMERR_MSG_PACK_ERROR, "Pack error in CSMINodeAttributes", &bufLen);

      } else {
        bufLen = retVal->bufLen;
        *buf = retVal->buf;

        free(retVal);
      }

      //to-do: need to hook up the free routine in the csmi side
      // free memory
      for (uint32_t i=0; i<nrows; i++) {
          csm_node_attributes_test_t *elem = &list[i];
          free(elem->nodeName);
      }
    }

    free(list);
    CSMLOG(csmd, debug) << "Exit " << __PRETTY_FUNCTION__;

    return errcode; 
}

void InvGetNodeInventory::NodeConnectRasEvent(const std::string &nodeName,
                                                 std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  csm::daemon::NetworkEvent* ne = CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_UP,    // &msg_id, 
                                                        nodeName,                    // &location_name,
                                                        "",                          // &raw_data,
                                                        "");                         // &kvcsv)

  if (ne != nullptr)
  {
    CSMLOG(csmd, info) << "InvGetNodeInventory::NodeConnectRasEvent for " << nodeName;
    postEventList.push_back(ne);
  }
}

void InvGetNodeInventory::NodeDisconnectRasEvent(csm::network::Address_sptr addr,
                                                    std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  string nodeName = GetDaemonState()->GetCNUidFromAddr(addr);
  csm::daemon::NetworkEvent* ne = CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_DOWN,  // &msg_id, 
                                                        nodeName,                    // &location_name,
                                                        "",                          // &raw_data,
                                                        "");                         // &kvcsv)

  if (ne != nullptr)
  {
    CSMLOG(csmd, info) << "InvGetNodeInventory::NodeDisconnectRasEvent for " << nodeName;
    postEventList.push_back(ne);
  }
}

