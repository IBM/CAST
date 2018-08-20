//#================================================================================
//#
//#    hcdiag/samples/gpu-health.cu
//#
//#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
//#
//#    This program is licensed under the terms of the Eclipse Public License
//#    v1.0 as published by the Eclipse Foundation and available at
//#    http://www.eclipse.org/legal/epl-v10.html
//#
//#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
//#    restricted by GSA ADP Schedule Contract with IBM Corp.
//#
//#================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>

// 07/26/2018: printing also PCI Bus Id (alda) 

#define MAX_BLOCKS 512
#define THREADS_PER_BLOCK 256

void cuda_dgemm(const char *, const char *, int *, int *, int *, double *, double *, int *, double *, int *, double *, double *, int *);
void cuda_dgemm_free();

#define CUDA_RC(rc) if( (rc) != cudaSuccess ) \
  {printf("Error %s at %s line %d\n", cudaGetErrorString(cudaGetLastError()), __FILE__,__LINE__); exit(1);}

#define CUDA_CHECK()  if( (cudaPeekAtLastError()) != cudaSuccess )        \
  {printf("Error %s at %s line %d\n", cudaGetErrorString(cudaGetLastError()), __FILE__,__LINE__-1); exit(1);}

double walltime(void);

__global__ void daxpy(const double alpha, const double * x, double * y, int npts) 
{
   for (int i = blockDim.x * blockIdx.x + threadIdx.x;  i < npts; i += blockDim.x * gridDim.x) y[i] = alpha*x[i] + y[i];
}

int main(int argc, char * argv[])
{
  int i, npts, iter, maxiter, device, numDevices;
  double * __restrict__ x, * __restrict__ y;
  double * dev_x, * dev_y;
  double * Amat, * Bmat, * Cmat;
  int m, n, k, lda, ldb, ldc;
  double alpha, beta, BW;
  double time1, time2;
  cudaDeviceProp prop;
  char host[240];

  gethostname(host, sizeof(host));

  // set npts large enough to exceed caches for daxpy : 1 GB
  npts = 1024*1024*(1024/8);

  printf("initializing data ...\n\n");

  // set matrix dimensions large enough to reach close to peak Flops
  m = 8192; n = 8192; k = 8192;
  Amat = (double *) malloc(m*k*sizeof(double));
  Bmat = (double *) malloc(k*n*sizeof(double));
  Cmat = (double *) malloc(m*n*sizeof(double));

#pragma omp parallel for
  for (i=0; i<(m*k); i++) Amat[i] = 1.2e-2*((double) (i%100));
#pragma omp parallel for
  for (i=0; i<(k*n); i++) Bmat[i] = 1.5e-3*((double) ((i + 100)%1000));
#pragma omp parallel for
  for (i=0; i<(m*n); i++) Cmat[i] = 1.5e-3*((double) ((i + 500)%1000));

  CUDA_RC(cudaGetDeviceCount(&numDevices));
  printf("found %d GPU devices on host %s\n\n", numDevices, host);

  for (device=0; device<numDevices; device++)
  {
     char pciBusId[256];
     CUDA_RC(cudaSetDevice(device));
     CUDA_RC(cudaGetDeviceProperties(&prop, device));
     CUDA_RC(cudaDeviceGetPCIBusId (pciBusId, 256, device));
     printf("checking device %d = %s ... \n", device, prop.name);
     printf("device on PCI Bus ID: %s\n", pciBusId);
     printf("compute capability major = %d, minor = %d\n", prop.major, prop.minor);
 
     // use pinned memory for x, pageable memory for y
     CUDA_RC(cudaMallocHost((void **)&x, npts*sizeof(double)));
     y = (double *) malloc(npts*sizeof(double));

     CUDA_RC(cudaMalloc((void **)&dev_x, npts*sizeof(double)));
     CUDA_RC(cudaMalloc((void **)&dev_y, npts*sizeof(double)));

   #pragma omp parallel for
     for (i=0; i<npts; i++) x[i] = (double) (i%10);
   #pragma omp parallel for
     for (i=0; i<npts; i++) y[i] = (double) (i%100);

     alpha = 3.0;
     maxiter = 5;

     time1 = walltime();
     CUDA_RC(cudaMemcpy(dev_x, x, npts*sizeof(double), cudaMemcpyHostToDevice));
     CUDA_RC(cudaDeviceSynchronize());
     time2 = walltime();

     BW = 8.0e-9*((double) npts)/(time2 - time1);
     printf("host to device transfer rate from pinned   memory = %.2lf GB/sec\n", BW);

     time1 = walltime();
     CUDA_RC(cudaMemcpy(dev_y, y, npts*sizeof(double), cudaMemcpyHostToDevice));
     time2 = walltime();

     BW = 8.0e-9*((double) npts)/(time2 - time1);
     printf("host to device transfer rate from pageable memory = %.2lf GB/sec\n", BW);

     time1 = walltime();
     CUDA_RC(cudaMemcpy(x, dev_x, npts*sizeof(double), cudaMemcpyDeviceToHost));
     CUDA_RC(cudaDeviceSynchronize());
     time2 = walltime();

     BW = 8.0e-9*((double) npts)/(time2 - time1);
     printf("device to host transfer rate from pinned   memory = %.2lf GB/sec\n", BW);

     time1 = walltime();
     CUDA_RC(cudaMemcpy(y, dev_y, npts*sizeof(double), cudaMemcpyDeviceToHost));
     time2 = walltime();

     BW = 8.0e-9*((double) npts)/(time2 - time1);
     printf("device to host transfer rate from pageable memory = %.2lf GB/sec\n", BW);

     int threadsPerBlock = THREADS_PER_BLOCK;
     int numBlocks = (npts + threadsPerBlock - 1) / threadsPerBlock;
     if (numBlocks > MAX_BLOCKS) numBlocks = MAX_BLOCKS;

     time1 = walltime();
     for (iter=0; iter<maxiter; iter++) {
        daxpy<<<numBlocks, threadsPerBlock>>>(alpha, dev_x, dev_y, npts);
        CUDA_CHECK();
     }
     CUDA_RC(cudaDeviceSynchronize());
     time2 = walltime();

     BW = 3.0*8.0e-9*((double) npts)*((double) maxiter)/(time2 - time1);
     printf("GPU daxpy bandwidth = %.2lf GB/sec\n", BW);

     free(y);
     CUDA_RC(cudaFreeHost(x));
     CUDA_RC(cudaFree(dev_x));
     CUDA_RC(cudaFree(dev_y));

     beta = 0.0; lda = m; ldb = k; ldc = m;
     cuda_dgemm("N", "N", &m, &n, &k, &alpha, Amat, &lda, Bmat, &ldb, &beta, Cmat, &ldc);
     cuda_dgemm_free();

     printf("\n");
  }

  printf("done\n");

  return 0;
}

double walltime(void)
{
  double elapsed;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  elapsed = ((double) tv.tv_sec) + 1.0e-6*((double) tv.tv_usec);
  return elapsed;
}


// variables for cublas dgemm wrapper
static double * d_A, * d_B, * d_C;
static cublasHandle_t handle;

// use the Fortran dgemm argument list
void cuda_dgemm(const char * transa, const char * transb, int * m, int * n, int * k, 
                double * alpha, double * A, int * lda, double * B, int * ldb, 
                double * beta, double * C, int * ldc)
{
   int M, N, K, LDA, LDB, LDC;
   int asize, bsize, csize;
   double time1, time2, GFlops;
   cublasOperation_t opA, opB;

   M = *m; N = *n; K = *k;
   LDA = *lda; LDB = *ldb; LDC = *ldc;

   asize = M*K;
   bsize = K*N;
   csize = M*N;

   cublasCreate(&handle);
   cudaMalloc((void **)&d_A, asize*sizeof(double));
   cudaMalloc((void **)&d_B, bsize*sizeof(double));
   cudaMalloc((void **)&d_C, csize*sizeof(double));

   cublasSetVector(asize, sizeof(double), A, 1, d_A, 1);
   cublasSetVector(bsize, sizeof(double), B, 1, d_B, 1);
   cublasSetVector(csize, sizeof(double), C, 1, d_C, 1);

   if      (transa[0] == 'n' || transa[0] == 'N') opA = CUBLAS_OP_N;
   else if (transa[0] == 't' || transa[0] == 'T') opA = CUBLAS_OP_T;

   if      (transb[0] == 'n' || transb[0] == 'N') opB = CUBLAS_OP_N;
   else if (transb[0] == 't' || transb[0] == 'T') opB = CUBLAS_OP_T;

   // call one time outside the timers, then time it
   cublasDgemm(handle, opA, opB, M, N, K, alpha, d_A, LDA, d_B, LDB, beta, d_C, LDC);
   cudaDeviceSynchronize();

   time1 = walltime();
   cublasDgemm(handle, opA, opB, M, N, K, alpha, d_A, LDA, d_B, LDB, beta, d_C, LDC);
   cudaDeviceSynchronize();
   time2 = walltime();
   GFlops = 2.0e-9*((double) M)*((double) N)*((double) K)/(time2 - time1);
   printf("GPU dgemm TFlops = %.3lf\n", 1.0e-3*GFlops);

   cudaMemcpy(C, d_C, csize*sizeof(double), cudaMemcpyDeviceToHost);

   return;
}

void cuda_dgemm_free()
{
   cudaFree(d_A);
   cudaFree(d_B);
   cudaFree(d_C);
   cublasDestroy(handle);
   return;
}
