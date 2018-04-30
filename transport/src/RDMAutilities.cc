/*******************************************************************************
 |    RDMAcoral.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


//! \file  RDMAUtilities.cc
//! \brief utility methods for printing RDMA information

#include "fshipMacros.h"
#include <string.h>

#include "../include/RDMACMfship.h"


#if  0
struct ibv_device {
	struct ibv_device_ops	ops;
	enum ibv_node_type	node_type;
	enum ibv_transport_type	transport_type;
	/* Name of underlying kernel IB device, eg "mthca0" */
	char			name[IBV_SYSFS_NAME_MAX];
	/* Name of uverbs device, eg "uverbs0" */
	char			dev_name[IBV_SYSFS_NAME_MAX];
	/* Path to infiniband_verbs class device in sysfs */
	char			dev_path[IBV_SYSFS_PATH_MAX];
	/* Path to infiniband class device in sysfs */
	char			ibdev_path[IBV_SYSFS_PATH_MAX];
};
#endif
/**
 * ibv_get_device_list - Get list of IB devices currently available
 * @num_devices: optional.  if non-NULL, set to the number of devices
 * returned in the array.
 *
 * Return a NULL-terminated array of IB devices.  The array can be
 * released with ibv_free_device_list().
 */
static struct ibv_device ** deviceListPtr = NULL;
static int numberIBVdevices = 0;
void list_devices(){
    if (deviceListPtr) return; //do once
    deviceListPtr  = ibv_get_device_list(&numberIBVdevices);
    LOG(txp,always) << "Number of IBV devices=" << numberIBVdevices;
    if (!deviceListPtr ) LOG(txp,always)<<"NULL on ibv_get_device_list.  errno="<<errno<<":"<<strerror(errno);
}  

void printDeviceList(){
  if (!deviceListPtr ) list_devices();
  if (!numberIBVdevices) return;
  for (int i=0;i<numberIBVdevices;i++)
  {
      struct ibv_device* device = deviceListPtr[i];

      LOG(txp,always) 
       <<   "device i=" << i 
       << " name=" << device->name 
       <<" ibdev_path=" << device->ibdev_path 
       << " guid=" << std::hex << ibv_get_device_guid(device) << std::dec 
       <<   " verbs dev_path=" << device->dev_path
          ;
  }
}

void printPath(struct rdma_cm_id& pRDMAcmid){
   LOG(txp,always) 
       <<"cmid rdma_route.num_paths="<<pRDMAcmid.route.num_paths
       <<" port_num="<<(int)pRDMAcmid.port_num
       ;
   LOG(txp,always) 
       //<<" kernel device="<<pRDMAcmid.verbs->device->name
       <<" verbs-device "<< pRDMAcmid.verbs->device->dev_path
       <<" class-device "<< pRDMAcmid.verbs->device->ibdev_path
       ;
   for(int i = 0;i<pRDMAcmid.route.num_paths;i++){
       LOG(txp,always)
          <<" dlid="<<htons(pRDMAcmid.route.path_rec[i].dlid)
          << std::hex
          <<" dgid="<<htonll(pRDMAcmid.route.path_rec[i].dgid.global.subnet_prefix)<<":"<<htonll(pRDMAcmid.route.path_rec[i].dgid.global.interface_id)
          << std::dec
          <<" slid="<<htons(pRDMAcmid.route.path_rec[i].slid)
          << std::hex
          <<" sgid="<<htonll(pRDMAcmid.route.path_rec[i].sgid.global.subnet_prefix)<<":"<<htonll(pRDMAcmid.route.path_rec[i].sgid.global.interface_id)
          << std::dec
          
          ;
   }
}

/**
 * rdma_get_devices - Get list of RDMA devices currently available.
 * @num_devices: If non-NULL, set to the number of devices returned.
 * Description:
 *   Return a NULL-terminated array of opened RDMA devices.  Callers can use
 *   this routine to allocate resources on specific RDMA devices that will be
 *   shared across multiple rdma_cm_id's.
 * Notes:
 *   The returned array must be released by calling rdma_free_devices.  Devices
 *   remain opened while the librdmacm is loaded.
 * See also:
 *   rdma_free_devices
struct ibv_context {
	struct ibv_device      *device;
	struct ibv_context_ops	ops;
	int			cmd_fd;
	int			async_fd;
	int			num_comp_vectors;
	pthread_mutex_t		mutex;
	void		       *abi_compat;
};
 */
 
static struct ibv_context ** contextlistPtr=NULL;
static int numberRDMAdevices = 0;

//http://linux.die.net/man/3/ibv_get_device_list
// ibv_free_device_list() frees the array of devices list returned by ibv_get_device_list(). 
// until freed, the file descriptors for async_fd and cmd_fd are valid
void list_rdma_devices(){
    if (contextlistPtr) {  
         rdma_free_devices(contextlistPtr);
         contextlistPtr=NULL;
    }
    contextlistPtr  = rdma_get_devices(&numberRDMAdevices);
    LOG(txp,always) << "Number of RDMA devices=" << numberRDMAdevices;
    if (!contextlistPtr ) LOG(txp,always)<<"NULL on ibv_get_device_list.  errno="<<errno<<":"<<strerror(errno);
} 
//http://linux.die.net/man/3/ibv_get_async_event
static int mtu[]={0,256,512,1024,2048,4096};

std::string getPortType(uint8_t pSpeedEnum){//enum ib_port_speed to string
    std::string typeString = "Unknown";
    switch(pSpeedEnum){
case 1: typeString ="IB_SPEED_SDR (data 2 Gbps/x)"; break;
case 2: typeString ="IB_SPEED_DDR (data 4 Gbps/x)"; break;
case 4: typeString ="IB_SPEED_QDR (data 8 Gbps/x)"; break;
case 8: typeString ="IB_SPEED_FDR10 (data 10 Gbps/x)"; break;
case 16: typeString= "IB_SPEED_FDR (data 13.64 Gbps/x)"; break;
case 32: typeString= "IB_SPEED_EDR (data 24.24 Gbps/x)"; break;

    }
    return typeString;
}

int getLanes(uint8_t pWidthEnum){//enum ib_port_width to number of lanes
    int lanes=0;
    switch(pWidthEnum){
case 1: return 1;
case 2: return 4;
case 4: return 8;
case 8: return 12;
    }
    return lanes;
}

void printRdmaDeviceList(){
  if (!contextlistPtr ) list_rdma_devices();
  if (!numberRDMAdevices) return;
  struct ibv_device_attr device_attr;
  struct ibv_port_attr port_attr;
  for (int i=0;i<numberRDMAdevices;i++){
      struct ibv_context * context = contextlistPtr[i];
      struct ibv_device * device = context->device;
  
      LOG(txp,always)<<"rdma device i="<<i<<" name="<<device->name<<" ibdev_path="<<device->ibdev_path<<" guid="<<std::hex<<ibv_get_device_guid(device)<<std::dec<<" verbs dev_path="<<device->dev_path;     
      LOG(txp,always)<<"name="<<device->name<<" type="<<ibv_node_type_str(device->node_type)<<"::transport="<<device->transport_type<< (device->transport_type ? ":nonIB":":IB");
       LOG(txp,always)<<"name="<<device->name<<" cmd_fd="<<context->cmd_fd<<" async_fd="<<context->async_fd<<" num_comp_vectors="<<context->num_comp_vectors;
      int queryErrno =  ibv_query_device(context,&device_attr);
      if (queryErrno){  LOG(txp,always)<<"rdma device i="<<i<<" name="<<device->name<<" queryErrno="<<queryErrno<<":"<<strerror(queryErrno);}
      else{
        LOG(txp,always)<<"name="<<device->name<<" max_mr_size="<< device_attr.max_mr_size<<" page_size_cap="<<device_attr.page_size_cap<<" num ports="<<(int)device_attr.phys_port_cnt;
        for (int i=1;i<=(int)device_attr.phys_port_cnt;i++){
          queryErrno=ibv_query_port(context, i,&port_attr);
          if (!queryErrno){
             LOG(txp,always)<<"name="<<device->name<<" port="<<i
             <<" base port lid="<<port_attr.lid
             <<" state="<<ibv_port_state_str(port_attr.state)
             <<" enum@mtu="<<port_attr.active_mtu<<":"<<mtu[port_attr.active_mtu]
             <<" max_mut="<<mtu[port_attr.max_mtu]
             <<" @portType="<<getPortType(port_attr.active_speed)
             <<" lanes x=" << getLanes(port_attr.active_width)
             ;
          }
        
        }
      }

  }
}



