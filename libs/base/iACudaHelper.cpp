/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
