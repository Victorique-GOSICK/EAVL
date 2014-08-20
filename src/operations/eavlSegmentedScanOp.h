// Copyright 2010-2013 UT-Battelle, LLC.  See LICENSE.txt for more information.
#ifndef EAVL_SegScan_OP_H
#define EAVL_SegScan_OP_H

#include "eavlCUDA.h"
#include "eavlArray.h"
#include "eavlOpDispatch.h"
#include "eavlOperation.h"
#include "eavlException.h"
#include <time.h>
#ifdef HAVE_OPENMP
#include <omp.h>
#endif

#ifndef DOXYGEN



struct eavlSegScanOp_CPU
{
    static inline eavlArray::Location location() { return eavlArray::HOST; }
    template <class F, class IN, class OUT, class FLAGS>
    static void call(int nitems, int,
                     const IN inputs, OUT outputs,
                     FLAGS flags, F&)
    {
        
    }
};

#if defined __CUDACC__

template<typename T>
class SegAddFunctor
{
  public:
    __device__ SegAddFunctor(){}

    __device__ T static op(T a, T b)
    {
        return a + b;
    }

    __device__ T static getIdentity(){return 0;}
};

template<typename T>
class SegMaxFunctor
{
  public:
    __device__ SegMaxFunctor(){}

    __device__ T static op(T a, T b)
    {
        return max(a, b);
    }

    __device__ T static getIdentity(){return 0;}
};


template<class OP, bool INCLUSIVE, class T>
__device__ T scanWarp(volatile T *data, const unsigned int idx = threadIdx.x)
{
    const unsigned int simdLane = idx & 31;
    if( simdLane >= 1  )  data[idx] = OP::op(data[idx -  1 ], data[idx]);
    if( simdLane >= 2  )  data[idx] = OP::op(data[idx -  2 ], data[idx]);
    if( simdLane >= 4  )  data[idx] = OP::op(data[idx -  4 ], data[idx]);
    if( simdLane >= 8  )  data[idx] = OP::op(data[idx -  8 ], data[idx]);
    if( simdLane >= 16 )  data[idx] = OP::op(data[idx -  16], data[idx]);
    if(INCLUSIVE)
    {
        return data[idx];
    }
    else
    {
        if(simdLane > 0)
        {
            return data[idx - 1];
        }
        else return OP::getIdentity();
    }
}

/*     Warp of 8 threads wide segemented scan example: 
       segFirstIdx is the warp index of the first item in the segment.
       If a segment starts from another warp, it is just 0.
       [1 2 1 1 0 1 0 1] data
       [0 0 0 0 1 0 0 0] flags
       [0 0 0 0 4 0 0 0] new flags (op1)
       [0 0 0 0 4 4 4 4] after max scan (op2)
       [  3 3 2   1 1 1] simdLane >= segFirstIdx + 1
       [    4 5     1 2] simdLane >= segFirstIdx + 2
       [1 3 4 5 0 1 1 2] last section shifts the data as neccesarry
*/
template<class OP, bool INCLUSIVE, class T>
__device__ T segScanWarp(volatile T *data, volatile int *flags, const unsigned int idx = threadIdx.x)
{
    const unsigned int simdLane = idx & 31;
    
    if( flags[idx] ) 
    {
        flags[idx] = simdLane;                                              /*op1*/      
    }
    unsigned int segFirstIdx = scanWarp< SegMaxFunctor<int>, true>(flags);  /*op2*/

    if( simdLane >= segFirstIdx + 1 )  data[idx] = OP::op(data[idx -  1 ], data[idx]);
    if( simdLane >= segFirstIdx + 2 )  data[idx] = OP::op(data[idx -  2 ], data[idx]);
    if( simdLane >= segFirstIdx + 4 )  data[idx] = OP::op(data[idx -  4 ], data[idx]);
    if( simdLane >= segFirstIdx + 8 )  data[idx] = OP::op(data[idx -  8 ], data[idx]);
    if( simdLane >= segFirstIdx + 16)  data[idx] = OP::op(data[idx -  16], data[idx]);

    if(INCLUSIVE)
    {
        return data[idx];
    }
    else
    {
        if(simdLane > 0 && segFirstIdx != simdLane) /*return identity if first in exclusive segment */
        {
            return data[idx - 1];
        }
        else
        {

            return OP::getIdentity();
        } 
    }
}

template<class OP, bool INCLUSIVE, class T>
__device__ T segScanBlock(volatile T *data, volatile int *flags, const unsigned int idx = threadIdx.x)
{
    volatile __shared__ int sm_data[256];
    volatile __shared__ int sm_flags[256];

    int blockId   = blockIdx.y * gridDim.x + blockIdx.x;
    const int threadID     = blockId * blockDim.x + threadIdx.x;
    unsigned int warpIdx = idx >> 5;                              /*drop the first 32 bits */
    unsigned int warpFirstThread = warpIdx << 5;                  /* first warp thread idx, put some zeros back on */
    unsigned int warpLastThread  = warpFirstThread + 31;

    sm_data[idx]  = data[threadID];
    sm_flags[idx] = flags[threadID];
   
    __syncthreads();

    bool openSegment = (sm_flags[warpFirstThread] == 0);             /* open segment running into warp(carry in if warpIdx !=0), need now since segscan will overwrite flags */
    
    __syncthreads();

    
    T result = segScanWarp<OP,INCLUSIVE>(sm_data,sm_flags, idx);

    
    T lastVal = sm_data[warpLastThread];
    int flagResult = sm_flags[idx];
    bool warpHasFlag = sm_flags[warpLastThread] != 0 || !openSegment; /* were there any segment flags in the warp*/
    bool acc = (openSegment && sm_flags[idx] == 0);                   /*this thread should add the carry in to its current result */   
    
    __syncthreads();

    /* the last thread in the warp stores the final value and a flag, if there was one in the warp, into a value in warp 0*/
    /* warp 0 is a storage location since all values are already correct(no carry in) and stored in result */
    if(idx == warpLastThread) 
    {
        sm_data[warpIdx]  = lastVal;
        sm_flags[warpIdx] = warpHasFlag;
        //if (blockIdx.x == 15 ) printf("warpid: %d   %d last value : %d\n",warpIdx, data[warpIdx],lastVal  );
    }

    __syncthreads();
    /*The first warp in each block now seg scans(inclusive) all the warp totals and flags */
    if(warpIdx == 0) segScanWarp<OP,true>(sm_data,sm_flags,idx);
    __syncthreads();

    /* Now add carry over from the previous warp into the individually scanned warps */
    if(warpIdx != 0 && acc)
    {
        result = OP::op(sm_data[warpIdx-1], result);
    }
     __syncthreads();

    sm_data[idx] = result;
    
    __syncthreads();

    data[threadID]  = sm_data[idx];

     __syncthreads();
    return result;
}


template<class OP, class T>
__device__ T scanFlagsBlock(volatile T *flags, const unsigned int idx = threadIdx.x)
{
    unsigned int warpIdx = idx >> 5;                              /*drop the first 32 bits */

    __syncthreads();

    T result = scanWarp<OP,true>(flags);
    
    __syncthreads();

    if(warpIdx !=0 ) result = OP::op(flags[warpIdx-1], result);
  
    __syncthreads();

    flags[idx] = result;//NOO

     __syncthreads();


    return result;
}


template<class OP, class T>
__global__ void scanFlagsBlockKernel(int nItems, T *flags)
{
    const unsigned int idx = threadIdx.x;
    int blockId   = blockIdx.y * gridDim.x + blockIdx.x;

    const int threadID     = blockId * blockDim.x + threadIdx.x;
    unsigned int warpIdx   = idx >> 5;
    unsigned int warpFirstThread = warpIdx << 5;                  /* first warp thread idx, put some zeros back on */
    unsigned int warpLastThread  = warpFirstThread + 31;
    
    if(threadID > nItems) return;
    
    __shared__ T sm_flags[256];
    
    sm_flags[idx] = flags[threadID];

    __syncthreads();

    T result = scanWarp<OP,true>(sm_flags);
    
    __syncthreads();

    if(idx == warpLastThread) sm_flags[warpIdx] = result;
    //if(idx == warpLastThread && blockId == 32) printf("$$$$$$$$$$$$$last index %d storing %d in %d\n", threadIdx.x, result, warpIdx);
    __syncthreads();
    if(warpIdx == 0) scanWarp<OP,true>(sm_flags);
    __syncthreads();

    if(warpIdx !=0 ) result = OP::op(sm_flags[warpIdx-1], result);
    
    __syncthreads();
    sm_flags[idx] = result;
    __syncthreads();
    flags[threadID] = sm_flags[idx];
    //if(threadIdx.x == 223 && blockId == 32) printf("############# theadIdx %d sm_flags %d g_flags %d\n", idx, sm_flags[threadIdx.x], flags[threadID]);
    //if(threadIdx.x == 224 && blockId == 32) printf("############# theadIdx %d sm_flags %d g_flags %d\n", idx, sm_flags[threadIdx.x], flags[threadID]);
     __syncthreads();


}

template <class OP,bool INCLUSIVE, class T, class FLAGS>
__global__ void
SegScanBlock_kernel(int nitems,
                          T* input,
                          FLAGS* flags)
{
    int blockId   = blockIdx.y * gridDim.x + blockIdx.x;
    const int threadID   =  blockId * blockDim.x + threadIdx.x;;
    if(threadID > nitems) return;
    segScanBlock< OP, INCLUSIVE >(input,flags);
}

template <class OP, bool INCLUSIVE, class T, class FLAGS>
__global__ void
SegScanBlockSum_kernel(int nblocks, T* inputs, FLAGS* flags, T* blockSums)
{
    unsigned int blockId = threadIdx.x;
    //const int threadID   = blockId * blockDim.x + threadIdx.x;

    int carry = 0;
    int iterations  = nblocks / 256;
    if (nblocks % 256 > 0) iterations++;
    
    __shared__ T sm_data[256];
    __shared__ T sm_flags[256];

    for(int i = 0; i < iterations ; i++)
    {

        if(blockId >= nblocks) return;

        unsigned int blockFirstIdx = blockId << 8;
        unsigned int blockLastIdx  = blockFirstIdx + 255;

        bool openSegment = flags[blockFirstIdx] == 0;
        __syncthreads();

        bool blockHasFlag = flags[blockLastIdx] != 0 || !openSegment;
        bool acc =  (openSegment && flags[blockId] == 0);
        
        __syncthreads();

        /* load block totals and carry flags into shared memory */
        sm_data[threadIdx.x ] = inputs[blockLastIdx];  
        //printf("Block Sum %d block id %d  \n", sm_data[threadIdx.x ], blockId, blockLastIdx);
        sm_flags[threadIdx.x] = blockHasFlag;

        __syncthreads();
        /*Need to carry in previous values from the last group of blocks */
        if(blockId != 0 && acc) 
        {
            
            if(blockId % 256 == 0) sm_data[threadIdx.x] = OP::op(carry , sm_data[threadIdx.x]);

        }

        __syncthreads();
        /* seg scan the results */
        segScanBlock<OP,true>(sm_data,sm_flags);

        __syncthreads();

        if(blockId % 256 == 255) carry = sm_data[255];  
        __syncthreads();
        /*copy the sums into the block sums array */

        blockSums[blockId] = sm_data[threadIdx.x];
        __syncthreads();

        /*shift the block ids */
        blockId = blockId + 256; 
        __syncthreads();
        

    }
    


    

}


template <class OP, class T, class FLAGS>
__global__ void
SegScanAddSum_kernel(int nitems, T* inputs, T* outputs,T* blockSums, FLAGS* flags)
{
    volatile __shared__ T sm_data[256];
    volatile __shared__ T sm_flags[256];

    
    unsigned int blockId = blockIdx.y * gridDim.x + blockIdx.x ;
    const int threadID   = blockId * blockDim.x + threadIdx.x;
    unsigned int blockFirstThread = blockId << 8;

    if(threadID > nitems) return; 

    T originalVal = inputs[threadID];
    
    sm_flags[threadIdx.x] = flags[threadID];

    __syncthreads();  

    bool openSegment = (flags[blockFirstThread] == 0);
    bool acc = (openSegment && sm_flags[threadIdx.x] == 0);
    //if(threadIdx.x == 224 && blockId == 32) printf("############# openSegment %d acc %d sm_flags %d g_flags %d", openSegment, acc, sm_flags[threadIdx.x], flags[threadID]);
    __syncthreads();
    
    sm_data[threadIdx.x]  = outputs[threadID];
  
    
    __syncthreads();
    
    if(blockId != 0 && acc)
    {
        sm_data[threadIdx.x] = OP::op(blockSums[blockId - 1], sm_data[threadIdx.x]+ originalVal);
    }

    __syncthreads();

    outputs[threadID] =  sm_data[threadIdx.x];
    
    __syncthreads();
    

}






struct eavlSegScanOp_GPU
{
    static inline eavlArray::Location location() { return eavlArray::DEVICE; }
    template <class F, class IN, class OUT, class FLAGS>
    static void call(int nitems, int,
                     IN inputs, OUT outputs,
                     FLAGS flags, F&)
    {
        SegAddFunctor<int> op;
        //bool inclusive = true;
        /*create some temp space as not to destroy the input arrays*/
        int *_flags   = get<0>(flags).array;                     
        int *_inputs  = get<0>(inputs).array;
        int *_outputs = get<0>(outputs).array;
        int *tempFlags;
        int *blockSums;

        int numBlocks = nitems / 256;
        if(nitems%256 > 0) numBlocks++;

        int numBlocksX = numBlocks;
        int numBlocksY = 1;
        if (numBlocks >= 32768)
        {
            numBlocksY = numBlocks / 32768;
            numBlocksX = (numBlocks + numBlocksY-1) / numBlocksY;
        }
        cudaMalloc((void **)&tempFlags,nitems*sizeof(int));
        CUDA_CHECK_ERROR();
        cudaMemcpy(tempFlags, &(_flags[0]),
                   nitems*sizeof(int), cudaMemcpyDeviceToDevice);
        CUDA_CHECK_ERROR();
        cudaMemcpy(_outputs, &(_inputs[0]),
                   nitems*sizeof(int), cudaMemcpyDeviceToDevice);
        CUDA_CHECK_ERROR();
        

         
        int numThreads = 256;
        dim3 threads(numThreads,   1, 1);
        dim3 blocks (numBlocksX,numBlocksY, 1);
        SegScanBlock_kernel< SegAddFunctor<int> ,false,int > <<< blocks, threads >>>(nitems, _outputs,_flags);
        CUDA_CHECK_ERROR();
        scanFlagsBlockKernel< SegMaxFunctor<int>, int > <<< blocks, threads >>>(nitems, _flags);
        
        cudaMalloc((void**)&blockSums, numBlocks*sizeof(int));
        CUDA_CHECK_ERROR();

        dim3 one(1,1,1);
        SegScanBlockSum_kernel< SegAddFunctor<int> ,false,int > <<< one, threads >>>(numBlocks, _outputs, _flags, blockSums);

        CUDA_CHECK_ERROR();

        SegScanAddSum_kernel< SegAddFunctor<int> > <<<blocks, threads >>>(nitems, _inputs, _outputs, blockSums, _flags );
        CUDA_CHECK_ERROR();

        cudaFree(blockSums);
        CUDA_CHECK_ERROR();

    }
};


#endif

#endif

// ****************************************************************************
// Class:  eavlSegScanOp
//
//  Purpose: Segmented scan provides a parallel primitve building block.
///          The operation performs the same operation as scan, but adds
///          a flag array to perform the scan on arbitrary segments within 
///          the input array. Flags are integers that are 0 or 1, where
///          a 1 marks the beginning of a new segment.
//                        
//
// Programmer: Matt Larsen 8/3/2014
//
// Modifications:
//    
// ****************************************************************************
template <class I, class O, class FLAGS>
class eavlSegScanOp : public eavlOperation
{
  protected:
    DummyFunctor functor;
    I            inputs;
    O            outputs;
    FLAGS        flags;
    int          nitems;
  public:
    eavlSegScanOp(I i, O o, FLAGS flgs)
        : inputs(i), outputs(o), flags(flgs), nitems(-1)
    {
    }
    eavlSegScanOp(I i, O o, FLAGS flgs, int itemsToProcess)
        : inputs(i), outputs(o), flags(flgs), nitems(itemsToProcess)
    {
    }
    virtual void GoCPU()
    {
        int dummy;
        int n=0;
        if(nitems > 0) n = nitems;
        else n = inputs.first.length();
        eavlOpDispatch<eavlSegScanOp_CPU>(n, dummy, inputs, outputs, flags, functor);
    }
    virtual void GoGPU()
    {
#ifdef HAVE_CUDA
        int dummy;
        int n=0;
        if(nitems > 0) n = nitems;
        else n = inputs.first.length();
        eavlOpDispatch<eavlSegScanOp_GPU>(n, dummy, inputs, outputs, flags, functor);
#else
        THROW(eavlException,"Executing GPU code without compiling under CUDA compiler.");
#endif
    }
};

// helper function for type deduction
template <class I, class O, class FLAGS>
eavlSegScanOp<I,O,FLAGS> *new_eavlSegScanOp(I i, O o, FLAGS flags) 
{
    return new eavlSegScanOp<I,O,FLAGS>(i,o,flags);
}

template <class I, class O, class FLAGS>
eavlSegScanOp<I,O,FLAGS> *new_eavlSegScanOp(I i, O o, FLAGS flags, int itemsToProcess) 
{
    return new eavlSegScanOp<I,O,FLAGS>(i,o,flags, itemsToProcess);
}


#endif