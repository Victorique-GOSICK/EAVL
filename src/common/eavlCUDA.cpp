// Copyright 2010-2012 UT-Battelle, LLC.  See LICENSE.txt for more information.
#include "eavlConfig.h"
#include "eavlCUDA.h"
#include <cstdlib>
#include "STL.h"
#include "eavlException.h"
#include "eavlExecutor.h"

#ifdef HAVE_CUDA
#include <cuda_runtime_api.h>

struct DeviceSort
{
    const vector<cudaDeviceProp> &devs;
    DeviceSort(const vector<cudaDeviceProp> &dp) : devs(dp) { }
    bool operator()(int a, int b)
    {
        string namea = devs[a].name;
        string nameb = devs[b].name;
        int    majora = devs[a].major;
        int    majorb = devs[b].major;
        int    minora = devs[a].minor;
        int    minorb = devs[b].minor;
        bool   teslaa = namea.find("Tesla") != string::npos;
        bool   teslab = nameb.find("Tesla") != string::npos;
        if (teslaa && !teslab)
        {
            //cerr << namea << "<" << nameb << " because A has tesla\n";
            return true;
        }
        if (!teslaa && teslab)
        {
            //cerr << namea << ">" << nameb << " because B has tesla\n";
            return false;
        }
        if (majora > majorb)
        {
            //cerr << namea << "<" << nameb << " because A's major version is higher\n";
            return true;
        }
        if (majora < majorb)
        {
            //cerr << namea << ">" << nameb << " because B's major version is higher\n";
            return false;
        }
        if (minora > minorb)
        {
            //cerr << namea << "<" << nameb << " because A's minor version is higher\n";
            return true;
        }
        if (minora < minorb)
        {
            //cerr << namea << ">" << nameb << " because B's minor version is higher\n";
            return false;
        }
        return a < b;
    }
};
#endif


void eavlInitializeGPU()
{
#ifdef HAVE_CUDA
    // Get a list of devices
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount <= 0)
    {
        cerr << "WARNING: CUDA was enabled, but no GPUs found.  "
             << "Forcing CPU execution." << endl;
        eavlExecutor::SetExecutionMode(eavlExecutor::ForceCPU);
        return;
    }

    vector<cudaDeviceProp> devs(deviceCount);
    for (int device=0; device<deviceCount; ++device)
    {
        cudaGetDeviceProperties(&devs[device], device);
    }

    // See if user requested a device explicitly
    int requestedDevice = -1;
    const char *eavlgpu = getenv("EAVLGPU");
    if (eavlgpu)
    {
        requestedDevice = strtol(eavlgpu,NULL,10);
        if (requestedDevice < 0 || requestedDevice >= deviceCount)
        {
            cerr << "WARNING: Requested GPU device out of range.  "
                 << "Forcing CPU-only execution." << endl;
            eavlExecutor::SetExecutionMode(eavlExecutor::ForceCPU);
            return;
        }
    }

    vector<int> deviceOrder;
    for (int device=0; device<deviceCount; ++device)
        deviceOrder.push_back(device);
    std::sort(deviceOrder.begin(), deviceOrder.end(), DeviceSort(devs));
    cerr << "Found " << deviceCount << " devices; preferred order as follows:\n";
    for (int i=0; i<deviceCount; ++i)
    {
        int device = deviceOrder[i];
        cerr << "  -> Device #" << device << ": " << devs[device].name
             << "  capability="<<devs[device].major<< "."<<devs[device].minor
             << endl;
    }

    // Look for Tesla GPUs first
    if (requestedDevice >= 0)
    {
        cerr << "Requesting device #"<<requestedDevice
             << " ("<<devs[requestedDevice].name<<")\n"
             << "   as chosen by EAVLGPU environment variable" << endl;
    }
    else
    {
        requestedDevice = deviceOrder[0];
        cerr << "Requesting device #"<<requestedDevice
             << " ("<<devs[requestedDevice].name<<") "
             << "   as chosen by by preferred order." << endl;
    }

    // okay, set the device and see if the one we got matches what we asked for
    cudaSetDevice(requestedDevice);
    int actualDevice;
    cudaGetDevice(&actualDevice);
    if (actualDevice != requestedDevice)
    {
        cerr << "WARNING: actually got device #"<<actualDevice
             <<" ("<<devs[actualDevice].name<<")\n";
    }

    // we got an actual device; enable GPU execution (when possible)
    eavlExecutor::SetExecutionMode(eavlExecutor::PreferGPU);

    if (devs[actualDevice].major < 2)
    {
#ifndef HAVE_OLD_GPU
        cerr << "WARNING: You did not compile in support for old GPUs, "
             << "but you're about to use one with compute capability < 2.0.  "
             << "If you attempt to use any GPU functions, things will likely "
             << "fail spectacularly.  Therefore, we're forcing CPU "
             << "execution for you." << endl;
        eavlExecutor::SetExecutionMode(eavlExecutor::ForceCPU);
        return;
#else
        cerr << "WARNING: You are using an old GPU with compute capability "
             << "< 2.0.  Some important operations are not supported with "
             << "old GPUs and you may have some operations listed as "
             << "unimplemented for GPUs." << endl;
#endif
    }
#else

    // CUDA not enabled; force CPU execution
    eavlExecutor::SetExecutionMode(eavlExecutor::ForceCPU);
#endif
}
