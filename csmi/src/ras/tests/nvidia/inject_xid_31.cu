/*================================================================================

    csmi/src/ras/tests/nvidia/inject_xid_31.cu

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
 
// Compile:
// /usr/local/cuda-8.0/bin/nvcc inject_xid_31.cu -o inject_xid_31 

// Run:
// ./inject_xid_31
// Run on specific GPU:
// CUDA_VISIBLE_DEVICES=0 ./inject_xid_31
// CUDA_VISIBLE_DEVICES=1 ./inject_xid_31
// CUDA_VISIBLE_DEVICES=2 ./inject_xid_31
// CUDA_VISIBLE_DEVICES=3 ./inject_xid_31

// Sample output:
// dmesg | tail -n 1
// [1992033.683091] NVRM: Xid (PCI:0002:01:00): 31, Ch 00000010, engmask 00000101, intr 10000000
// [1992005.493582] NVRM: Xid (PCI:0003:01:00): 31, Ch 00000010, engmask 00000101, intr 10000000
// [1992013.187501] NVRM: Xid (PCI:0006:01:00): 31, Ch 00000010, engmask 00000101, intr 10000000
// [1992019.406216] NVRM: Xid (PCI:0007:01:00): 31, Ch 00000010, engmask 00000101, intr 10000000

__global__ void
genXid0()
{
  double *p0 = NULL;
 
  int ii = blockIdx.x * blockDim.x + threadIdx.x;
  p0[ii] = 0.0;
}
 
#define VECSIZE 1000
struct cudaDeviceProp cudaDeviceProps;
 
int main(int argc, char** argv)
{
  int device_count(0);
  int rc(0);
 
  cudaGetDeviceCount(&device_count);
  printf("device count = %d\n",device_count);
 
  for (int i = 0; i < device_count; i++)
  {
    cudaGetDeviceProperties(&cudaDeviceProps,i);
    printf("device name = %s\n",cudaDeviceProps.name);
  } 

  dim3 grid(64,64);
  dim3 block(16,16);
  genXid0<<<grid,block>>>();

  rc = cudaThreadSynchronize();
  if (rc != cudaSuccess)
  {
    fprintf(stderr,"cudaThreadSynchronize() returned status = %d\n",rc);
    return 1;
  }
 
  return 0;
}
