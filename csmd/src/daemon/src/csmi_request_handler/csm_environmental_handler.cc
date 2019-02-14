/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_environmental_handler.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_daemon_config.h"
#include "csm_environmental_handler.h"
#include "logging.h"
#include "csmd/src/inv/include/inv_dcgm_access.h"
#include "csmd/src/inv/include/inv_ssd_inventory.h"
#include "csmd/src/inv/include/inv_ssd_wear_serialization.h"

#include <iostream>
#include <iomanip>

// An example of the environmental handler implementation
// Its process function should allow to be interrupted and resumed the unfinished tasks later
void CSM_ENVIRONMENTAL::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  if ( !isEnvironmentalEvent(aEvent) ) return;
  
  LOG(csmenv, info) << "In CSM_ENVIRONMENTAL...";
  csm::daemon::BitMap currItems = GetBitMap(aEvent);
  std::set<csm::daemon::BucketItemType> list;
  
  /* for debugging */
  currItems.GetMembers(list);
  for (auto it=list.begin(); it !=list.end(); it++)
  {
    LOG(csmenv, trace) << "\t new item = " << *it;
  }
  
  _pendingItems.GetMembers(list);
  for (auto it=list.begin(); it !=list.end(); it++)
  {
    LOG(csmenv, trace) << "\t pending item = " << *it;
  }
  /* end for debugging **/
  
  // \todo: for simplicity, we just merge the current items to the previous pending item list
  // maybe we should do the pending items first?
  _pendingItems += currItems;
  _pendingItems.GetMembers(list);

  auto it = list.begin();
  try {
    CSM_Environmental_Data envData;
    envData.CollectNodeData();

    while ( it != list.end() )
    {
        csm::daemon::BucketItemType nextItem = *it;
        LOG(csmenv, debug) << "Processing BucketItem: " << nextItem ;
        switch (nextItem)
        {
          case csm::daemon::GPU:
          {
            LOG(csmenv, debug) << "Collecting GPU data.";
            bool gpu_success(false);            
            std::list<boost::property_tree::ptree> gpu_data_pt_list;

            gpu_success = csm::daemon::INV_DCGM_ACCESS::GetInstance()->CollectGpuData(gpu_data_pt_list);
            if (gpu_success)
            {
               envData.AddDataItems(gpu_data_pt_list);
            }

            break;
          }
          case csm::daemon::DEBUG:
            LOG( csmenv, info ) << "ENVDATA Collection debug.";
            envData.GenerateTestData();
            break;
          case csm::daemon::CPU:
          {
            LOG(csmenv, debug) << "Collecting CPU data.";
            break;
          }
          case csm::daemon::ENVIRONMENTAL:
          {
            LOG(csmenv, debug) << "Collecting node environmental data.";
            envData.CollectEnvironmentalData();
            break;
          }
          case csm::daemon::SSD:
          {
            LOG(csmenv, debug) << "Collecting SSD Wear data.";
            BuildSsdWearUpdate(postEventList);
            break;
          }
          case csm::daemon::NETWORK:
          case csm::daemon::DEFAULT:
            break;

          default:
            LOG(csmenv, error) << "CSM_ENVIRONMENTAL: Unknown bucket item type!";
            break;
        }
        _pendingItems.RemoveSet(nextItem);
        it++;

        //after processing one item, we call an interruption point here
        //to check whether the thread has been interrupted
        boost::this_thread::interruption_point();
    }
    // only serialize and send if the above sections have added any data to the env data set.
    if( envData.HasData() )
    {

      LOG( csmenv, debug ) << "ENVDATA:" << CSMI_BASE::ConvertToBytes<CSM_Environmental_Data>( envData );

      csm::network::Message msg;
      msg.Init(CSM_environmental_data, 0, CSM_PRIORITY_DEFAULT,
               0, 3253, 1351, geteuid(), getegid(),
               std::string( CSMI_BASE::ConvertToBytes<CSM_Environmental_Data>( envData ) ) );

      //std::cout << msg.GetDataLen() << std::endl;

      csm::network::MessageAndAddress env_data_msg( msg, _AbstractAggregator);

      postEventList.push_back( CreateNetworkEvent( env_data_msg ) );

    }
  }
  catch (const boost::thread_interrupted &e)
  {
    LOG(csmenv, warning) << "CSM_ENVIRONMENTAL: Thread interrupted. This means we violated the jitter window limit";
    if (it != list.end())
    {
      LOG(csmenv, warning) << "CSM_ENVIRONMENTAL: incomplete bucket processing. items=" << list.size()
        << " pending_mask=0x" << std::hex << _pendingItems.GetMask() << std::dec;
    }
  }
  
}

bool CSM_ENVIRONMENTAL::BuildSsdWearUpdate(std::vector<csm::daemon::CoreEvent*>& postEventList)
{
  bool ssd_success(false);
  csm_ssd_wear_t ssd_wear;

  std::string node_name("");
  try
  {
    node_name = csm::daemon::Configuration::Instance()->GetHostname();
  }
  catch (csm::daemon::Exception &e)
  {
    LOG(csmenv, error) << "SSD_WEAR: Caught exception when trying GetHostname()";
  }

  if (!node_name.empty())
  {
    LOG(csmenv, debug) << "SSD_WEAR: ssd_wear.node_name = " << node_name;
    strncpy(ssd_wear.node_name, node_name.c_str(), CSM_NODE_NAME_MAX);
    ssd_wear.node_name[CSM_NODE_NAME_MAX - 1] = '\0';
  }
  else
  {
    ssd_wear.node_name[0] = '\0';
    LOG(csmenv, error) << "SSD_WEAR: Error: could not determine ssd_wear.node_name!";
    return false;
  }

  ssd_success = GetSsdInventory(ssd_wear.ssd, ssd_wear.discovered_ssds); 
  if ( ssd_success )
  {
    LOG(csmenv, info) << "SSD_WEAR: Successfully collected ssd wear data.";
  
    string payload_str("");
    uint32_t bytes_packed(0);
    bytes_packed = ssd_wear_pack(ssd_wear, payload_str);

    if ( bytes_packed == 0 || payload_str.size() == 0 )
    {
      LOG(csmenv, error) << "SSD_WEAR: Failed to pack ssd wear update.";
      return false;
    }

    LOG(csmenv, debug) << "SSD_WEAR: CSM_CMD_ssd_wear_update payload_str.size() = " << payload_str.size();

    csm::network::Message ssd_msg;
    ssd_msg.Init(CSM_CMD_ssd_wear_update, 0, CSM_PRIORITY_DEFAULT,
      0, 3253, 1351, geteuid(), getegid(),
      payload_str );

    csm::network::MessageAndAddress ssd_msg_and_addr( ssd_msg, _AbstractMaster);

    postEventList.push_back( CreateNetworkEvent( ssd_msg_and_addr ) );
  }
  else
  {
    LOG(csmenv, warning) << "SSD_WEAR: Failed to collect ssd wear data.";
  }

  return ssd_success;
}
