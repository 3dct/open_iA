// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACudaHelper.h"

#if CUDA_AVAILABLE

#include <cuda_runtime_api.h>

bool isCUDAAvailable()
{
	int deviceCount = 0;
	cudaGetDeviceCount(&deviceCount);
	if (deviceCount == 0)
	{
		return false;
	}
	// TODO: Allow choosing which device(s) to use!
	/*
	size_t mostMem = 0;	int idx = -1;
	for (int dev = 0; dev < deviceCount; dev++)
	{
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, dev);
		LOG(lvlInfo, QString("%1. Compute Capability: %2.%3. Clock Rate (kHz): %5. Memory Clock Rate (kHz): %6. Memory Bus Width (bits): %7. Concurrent kernels: %8. Total memory: %9.")
			.arg(deviceProp.name)
			.arg(deviceProp.major)
			.arg(deviceProp.minor)
			.arg(deviceProp.clockRate)
			.arg(deviceProp.memoryClockRate)
			.arg(deviceProp.memoryBusWidth)
			.arg(deviceProp.concurrentKernels)
			.arg(deviceProp.totalGlobalMem)
		);
		if (deviceProp.totalGlobalMem > mostMem)
		{
			mostMem = deviceProp.totalGlobalMem;
			idx = dev;
		}
	}
	astra::SGPUParams gpuParams;
	gpuParams.GPUIndices.push_back(idx);
	gpuParams.memory = mostMem ;
	astra::CCompositeGeometryManager::setGlobalGPUParams(gpuParams);
	*/
	return true;
}

#else

bool isCUDAAvailable()
{
	return false;
}

#endif
