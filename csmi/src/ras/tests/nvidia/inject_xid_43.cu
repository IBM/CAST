/*================================================================================

    csmi/src/ras/tests/nvidia/inject_xid_43.cu

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdio.h>

// Compile:
// /usr/local/cuda-8.0/bin/nvcc inject_xid_43.cu -o inject_xid_43

// Run:
// ./inject_xid_43
//
// Run on specific GPU:
// CUDA_VISIBLE_DEVICES=0 ./inject_xid_43
// CUDA_VISIBLE_DEVICES=1 ./inject_xid_43
// CUDA_VISIBLE_DEVICES=2 ./inject_xid_43
// CUDA_VISIBLE_DEVICES=3 ./inject_xid_43

// Sample output:
// dmesg | tail -n 1
// [1991019.552551] NVRM: Xid (PCI:0002:01:00): 43, Ch 00000010, engmask 00000101
// [1991393.936281] NVRM: Xid (PCI:0003:01:00): 43, Ch 00000010, engmask 00000101
// [1991428.078670] NVRM: Xid (PCI:0006:01:00): 43, Ch 00000010, engmask 00000101
// [1991437.093019] NVRM: Xid (PCI:0007:01:00): 43, Ch 00000010, engmask 00000101

__global__ void inject_xid_43(int* gpu_ptr)
{
  int * gpu_ptr2 = NULL;
  gpu_ptr[0] = *gpu_ptr2;
}

int main(void)
{
  int rc(0);
  int* gpu_ptr;
  int size = 8;

  cudaMalloc( (void **) &gpu_ptr, size);

  int num_threads = 8;
  int num_blocks = 1;

  inject_xid_43<<<num_blocks,num_threads+1>>>(gpu_ptr);
  
  rc = cudaThreadSynchronize();
  if (rc != cudaSuccess)
  {
    fprintf(stderr,"cudaThreadSynchronize() returned status = %d\n",rc);
    return 1;
  }

  return 0;
}

