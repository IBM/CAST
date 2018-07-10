/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_environmental_handler.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_environmental_handler.h"
#include "logging.h"
#include "csmd/src/inv/include/inv_dcgm_access.h"

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
    envData.Collect_Node_Data();

    while ( it != list.end() )
    {
        csm::daemon::BucketItemType nextItem = *it;
        LOG(csmenv, debug) << "Processing BucketItem: " << nextItem ;
        switch (nextItem)
        {
          case csm::daemon::GPU:
          {
            // local variables
            bool dcgm_installed_flag;
            bool dlopen_flag;
            bool dcgm_init_flag;

            // setting variables
            dcgm_installed_flag = true;
            dlopen_flag = false;
            dcgm_init_flag = true;

            LOG(csmenv, debug) << "Collecting GPU data.";

            // checking if dcgm is installed
            dcgm_installed_flag = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetDCGMInstalledFlag();
            if ( dcgm_installed_flag == false )
            {

               LOG(csmenv, info) << "The right DCGM version is not installed or the framework was configured without DCGM support, no GPU env data collection";

            } else {

               // checking if the class that collects the data was not able to use dlopen
               dlopen_flag = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetDlopenFlag();
               if ( dlopen_flag == true )
               {

                LOG(csmenv, error) << "The class that collects the data was not able to use dlopen, no GPU env data collection";

               } else {

                  // checking if the class that collects the data was not able to initialize DCGM env
                  dcgm_init_flag = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetDCGMInitFlag();
                  if ( dcgm_init_flag == false )
                  {

                   LOG(csmenv, error) << "The class that collects the data was not able to initialize DCGM, no GPU env data collection";

                  } else {

                   csm::daemon::INV_DCGM_ACCESS::GetInstance()->LogGPUsEnviromentalData();

                   Environmental_data->Get_GPU_Double_DCGM_Field_Values_And_Set_Bit();
                   Environmental_data->Get_GPU_Long_DCGM_Field_Values_And_Set_Bit();
                   //Environmental_data->Get_GPU_Double_DCGM_Field_String_Identifiers_And_Set_Bit();
                   //Environmental_data->Get_GPU_Long_DCGM_Field_String_Identifiers_And_Set_Bit();

                   /*
		   Environmental_data->Print_GPU_Double_DCGM_Field_Values();
                   Environmental_data->Print_GPU_Double_DCGM_Field_Values();
                   Environmental_data->Print_GPU_Int64_DCGM_Field_Values();
                   Environmental_data->Print_GPU_Double_DCGM_Field_String_Identifiers();
                   Environmental_data->Print_GPU_Int64_DCGM_Field_String_identifiers();
                   */
                   
                  }

               }

            }
            {

              GPU_Double_Data = Environmental_data->Return_GPU_Double_Data_Object();
              GPU_Long_Data = Environmental_data->Return_GPU_Long_Data_Object();
              //GPU_Double_Label_Data = Environmental_data->Return_GPU_Double_Label_Data_Object();
              //GPU_Long_Label_Data = Environmental_data->Return_GPU_Long_Label_Data_Object();

              envData.Set_Data( GPU_Double_Data );
              envData.Set_Data( GPU_Long_Data );
              //envData.Set_Data( GPU_Double_Label_Data );
              //envData.Set_Data( GPU_Long_Label_Data );

            }
            break;
          }
          case csm::daemon::CPU:
          {
            LOG(csmenv, debug) << "Collecting CPU data.";
            CSM_CPU_Data CPU_Data;
            CPU_Data.DummyReadCPU();
            envData.Set_Data( CPU_Data );
            break;
          }
          case csm::daemon::ENVIRONMENTAL:
          {
            LOG(csmenv, debug) << "Collecting node environmental data.";
            envData.Collect_Environmental_Data();
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
