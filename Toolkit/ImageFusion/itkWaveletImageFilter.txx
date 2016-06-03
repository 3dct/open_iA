/**********************************************************************
Copyright ©2013 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

//OPENCL Sample: DWT Haar 1D 

#ifndef __itkWaveletImageFilter_txx
#define __itkWaveletImageFilter_txx


#include "itkWaveletImageFilter.h"

#include "itkObjectFactory.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkProgressReporter.h"

#include "itkImageFileWriter.h"

#include <math.h>

namespace itk
{

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::calApproxFinalOnHost()
{
	cl_float *tempOutData = (cl_float*)malloc(signalLength * sizeof(cl_float));
	CHECK_ALLOCATION(tempOutData, "Failed to allocate host memory(tempOutData)");
		
	memcpy(tempOutData, inData, signalLength * sizeof(cl_float));

	for (cl_uint i = 0; i < signalLength; i++)
	{
		tempOutData[i] = tempOutData[i] / sqrt((float)signalLength); 
	}

	cl_uint length = signalLength; 
	while (length > 1u)
	{
		for (cl_uint i = 0; i < length / 2; ++i)
		{
			cl_float data0 = tempOutData[2 * i]; 
			cl_float data1 = tempOutData[2 * i + 1]; 

			hOutData[i] = (data0 + data1) / sqrt((float)2); 
			hOutData[length / 2 + i] = (data0 + data1) / sqrt((float)2); 
		}
		//copy inData to hOutData
		memcpy(tempOutData, hOutData, signalLength * sizeof(cl_float));

		length >>= 1; 
	}

	FREE(tempOutData); 

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::getLevels(unsigned int length, unsigned int *levels)
{
	cl_int returnVal = SDK_FAILURE; 

	for (unsigned int i = 0; i < SIGNAL_LENGTH; i++)
	{
		if (length ==(1 << i ))
		{
			*levels = i; 
			returnVal = SDK_SUCCESS; 
			break; 
		}
	}

	return returnVal; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::setupDwtHaar1D(int idx)
{
	signalLength = roundToPowerOf2<cl_uint>(signalLength); 
	unsigned int levels = 0; 
	int result = getLevels(signalLength, &levels); 

	CHECK_ERROR(result, SDK_SUCCESS, "signalLength > 2^23 not supported");

	//Allocate and init host memory
	inData = (cl_float *)malloc(signalLength * sizeof(cl_float));
	CHECK_ALLOCATION(inData, "Failed to allocate host memory. (inData) ");


	//generate random input data
	srand(time(NULL));

	//inData = this->GetInput(idx); 

	for (unsigned int i = 0; i < signalLength; i++)
	{
		inData[i] = (cl_float)(rand() % 10);
	}

	dOutData = (cl_float*)malloc(signalLength * sizeof(cl_float)); 
	CHECK_ALLOCATION(dOutData, "Failed to allocate host memory. (dOutData)"); 

	memset(dOutData, 0, signalLength * sizeof(cl_float));
		
	dPartialOutData = (cl_float*)malloc(signalLength * sizeof(cl_float));
	CHECK_ALLOCATION(dPartialOutData, "Failed to allocate host memory.  (dPartialOutData)"); 
		
	memset(dPartialOutData, 0, signalLength * sizeof(cl_float));

	hOutData = (cl_float*)malloc(signalLength * sizeof(cl_float));
	CHECK_ALLOCATION(hOutData, "Failed to allocate host memory.  (hOutData)");
		
	memset(hOutData, 0, signalLength * sizeof(cl_float));

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::genBinary()
{
	bifData binaryData; 
	binaryData.kernelName = std::string("DwtHaar1DCPPKernel_Kernels.cl");
	binaryData.flagsStr = std::string("-x clc++"); 

	if (args->isComplierFlagsSpecified())
	{
		binaryData.flagsFileName = std::string(args->flags.c_str());
	}

	binaryData.binaryName = std::string(args->dumpBinary.c_str());
	int status = generateBinaryImage(binaryData);
	return status; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::setupCL()
{
	cl_int status = 0; 
	cl_device_type dType = CL_DEVICE_TYPE_GPU; 

	cl_platform_id platform = NULL; 
	int retVal = getPlatform(platform, args->platformId, args->isPlatformEnabled());
	CHECK_ERROR(retVal, SDK_SUCCESS, "getPlatform() failed.");

	// Display available devices.
	retVal = displayDevices(platform, dType);
	CHECK_ERROR(retVal, SDK_SUCCESS, "displayDevices() failed.");
	

	cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 }; 
	context = clCreateContextFromType(cps, dType, NULL, NULL, &status); 
	CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed."); 

	status = getDevices(context, &devices, args->deviceId, args->isDeviceIdEnabled());
	CHECK_ERROR(status, SDK_SUCCESS, "getDevice() failed.");

	commandQueue = clCreateCommandQueue(context, devices[args->deviceId], 0, &status);
	CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed. ");

	retVal = deviceInfo.setDeviceInfo(devices[args->deviceId]); 
	CHECK_ERROR(retVal, 0, "SDKDeviceInfo::setDeviceInfo() failed.");
	
	cl_mem_flags inMemFlags = CL_MEM_READ_ONLY; 
	if (args->isAmdPlatform()) 
		//inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD; 
	
	inDataBuf = clCreateBuffer(context, inMemFlags, sizeof(cl_float*) *signalLength, NULL, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inDataBuf) ");
	
	dOutDataBuf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, signalLength * sizeof(cl_float*), NULL, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (dOutDataBuf) ");

	dPartialOutDataBuf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, signalLength * sizeof(cl_float), NULL, &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (dPartialOutDataBuf)");

	//create CL programm with specified kernel source
	buildProgramData buildData; 
	buildData.kernelName = std::string("DwtHaar1DCPPKernel_Kernels.cl");
	buildData.devices = devices; 
	buildData.deviceId = args->deviceId; 
	buildData.flagsStr = std::string("-x clc++");
	if (args->isLoadBinaryEnabled()) 
		buildData.binaryName = std::string(args->loadBinary.c_str());
	
	if (args->isComplierFlagsSpecified()) 
		buildData.flagsFileName = std::string(args->flags.c_str());
	
	retVal = buildOpenCLProgram(program, context, buildData); 
	CHECK_ERROR(retVal, 0, "buildOenCLProgram() failed. ");

	//get kernel object handle
	kernel = clCreateKernel(program, "dwtHaar1D", &status); 
	CHECK_OPENCL_ERROR(status, "clClreateKernel failed. "); 

	status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[args->deviceId]); 
	CHECK_ERROR(status, SDK_SUCCESS, " setKernelWorkGroupInfo() failed"); 

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::setWorkGroupSize()
{
	cl_int status = 0; 
	globalThreads = curSignalLength >> 1; 
	localThreads = groupSize; 

	if (localThreads > deviceInfo.maxWorkItemSizes[0] || localThreads > deviceInfo.maxWorkGroupSize)
	{
		std::cout << "Unsupported: Device does not support requested number of work items" << std::endl; 
		return SDK_FAILURE; 
	}

	if (kernelInfo.localMemoryUsed > deviceInfo.localMemSize)
	{
		std::cout << "Unsupported: Insufficient local memory on device. " << std::endl; 
		return SDK_FAILURE; 
	}

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::runDwtHaar1DKernel()
{
	cl_int status = 0; 

	status = this->setWorkGroupSize();
	CHECK_ERROR(status, SDK_SUCCESS, "setWorkGroupSize failed"); 

	cl_event writeEvent; 
	status = clEnqueueWriteBuffer(commandQueue, inDataBuf, CL_FALSE, 0, curSignalLength * sizeof(cl_float), inData, 0, NULL, &writeEvent); 
	CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inDataBuf)"); 

	status = clFlush(commandQueue); 
	CHECK_OPENCL_ERROR(status, "clFlush failed"); 

	status = waitForEventAndRelease(&writeEvent);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt1) Failed");

	ParaClass *paraClass = new ParaClass; 

	this->classObj = clCreateBuffer(context, CL_MEM_USE_HOST_PTR, 
		sizeof(paraClass), paraClass, &status); 
	CHECK_OPENCL_ERROR(status, "clclCreateBuffer failed. (inDataBuf)");

	cl_event mapEvent; 
	paraClass = (ParaClass*)clEnqueueMapBuffer(commandQueue, this->classObj, 
		CL_FALSE, CL_MAP_WRITE, 0, sizeof(ParaClass), 0, NULL, &mapEvent, &status);
	CHECK_OPENCL_ERROR(status, "clEnqueueMapBuffer failed. (classObj)");

	status = clFlush(commandQueue); 
	CHECK_OPENCL_ERROR(status, "clFlush failed. ");

	status = waitForEventAndRelease(&mapEvent); 
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(mapEvt1) Failed");

	paraClass->SetValue(this->totalLevels, this->curSignalLength, 
		this->levelsDone, this->maxLevelsOnDevice); 

	cl_event unmapEvent; 
	status = clEnqueueUnmapMemObject(commandQueue, this->classObj, paraClass, 0, NULL, &unmapEvent); 
	CHECK_OPENCL_ERROR(status, "clEnqueueunMapBuffer failed. (classObj)");

	status = clFlush(commandQueue); 
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = waitForEventAndRelease(&unmapEvent);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(mapEvt1) Failed");


	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inDataBuf); 
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inDataBuf)");

	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&dOutDataBuf); 
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (dOutDataBuf)");


	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&dPartialOutDataBuf);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (dPartialOutData)");

	status = clSetKernelArg(kernel, 3, (localThreads * 2 * sizeof(cl_float)), NULL);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (local memory)");

	status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&this->classObj);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (global memory)");


	//Enqueue a kernel run call
	cl_event ndrEvent; 
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1,
		NULL, &globalThreads, &localThreads, 0, NULL, &ndrEvent); 
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue); 
	CHECK_OPENCL_ERROR(status, "clFlush failed. "); 

	status = waitForEventAndRelease(&ndrEvent);
	CHECK_ERROR(status, SDK_SUCCESS, "waitForEventAndRelease(ndrEvent) failed. ");

	cl_event readEvent; 
	status = clEnqueueReadBuffer(commandQueue, dOutDataBuf, CL_FALSE, 0, signalLength * sizeof(cl_float), dOutData, 0, NULL, &readEvent); 
	CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

	cl_event readEvent2; 
	status = clEnqueueReadBuffer(commandQueue, dPartialOutDataBuf, CL_FALSE, 0, signalLength * sizeof(cl_float), dPartialOutData, 0, NULL, &readEvent2);
	CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

	status = clFlush(commandQueue); 
	CHECK_OPENCL_ERROR(status, "clFlush failed. ");

	status = waitForEventAndRelease(&readEvent); 
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt1) Failed");

	status = waitForEventAndRelease(&readEvent2);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt2) Failed");

	delete paraClass; 
	clReleaseMemObject(this->classObj);

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::runCLKernels()
{
	unsigned int levels = 0; 
	unsigned int curLevels = 0; 
	unsigned int actualLevels = 0; 

	int result = getLevels(signalLength, &levels);
	CHECK_ERROR(result, SDK_SUCCESS, "getLevels() failed");

	actualLevels = levels; 
	int tempVar = (int)(log((float)kernelInfo.kernelWorkGroupSize) / log((float)2)); 
	maxLevelsOnDevice = tempVar + 1; 

	cl_float *temp = (cl_float*)malloc(signalLength * sizeof(cl_float)); 
	memcpy(temp, inData, signalLength*sizeof(cl_float)); 

	levelsDone = 0; 
	int one = 1; 
	while ((unsigned int ) levelsDone < actualLevels)
	{
		curLevels = (levels < maxLevelsOnDevice) ? levels : maxLevelsOnDevice; 

		if (levels == 0)
			curSignalLength = signalLength;
		else
			curSignalLength = (one << levels); 

		//set group size
		groupSize = (1 << curLevels) / 2; 
		totalLevels = levels; 

		//run on GPU
		runDwtHaar1DKernel();

		if (levels <= maxLevelsOnDevice)
		{
			dOutData[0] = dPartialOutData[0]; 
			memcpy(hOutData, dOutData, (one << curLevels) * sizeof(cl_float)); 
			memcpy(dOutData + (one << curLevels), hOutData + (one << curLevels), (signalLength - (one << curLevels)) * sizeof(cl_float)); 
			break; 
		}
		else
		{
			levels -= maxLevelsOnDevice; 
			memcpy(hOutData, dOutData, curSignalLength*sizeof(cl_float)); 
			memcpy(inData, dPartialOutData, (one << levels) * sizeof(cl_float)); 
			levelsDone += (int)maxLevelsOnDevice; 
		}
	}

	memcpy(inData, temp, signalLength * sizeof(cl_float));
	free(temp); 

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::initialize()
{
	if (args->initialize())
		return SDK_FAILURE; 

	Option * length_option = new Option; 
	CHECK_ALLOCATION(length_option, "Error. Failed to allocate memory (length_option)\n"); 

	length_option->_sVersion = "x"; 
	length_option->_lVersion = "signalLength"; 
	length_option->_description = "Length of the signal"; 
	length_option->_type = CA_ARG_INT; 
	length_option->_value = &signalLength; 


	args->AddOption(length_option); 
	delete length_option; 

	Option* iteration_option = new Option;
	CHECK_ALLOCATION(iteration_option,
		"Error. Failed to allocate memory (iteration_option)\n");

	iteration_option->_sVersion = "i";
	iteration_option->_lVersion = "iterations";
	iteration_option->_description = "Number of iterations for kernel execution";
	iteration_option->_type = CA_ARG_INT;
	iteration_option->_value = &iterations;

	args->AddOption(iteration_option); 
	delete iteration_option; 

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::setup()
{
	if (iterations < 1)
	{
		std::cerr << "Error, iterations cannot be less or equal 0\n"; 
		exit(0); 
	}

	if (setupDwtHaar1D(0)!= SDK_SUCCESS)
		return SDK_FAILURE; 
	
	int timer = sampleTimer->createTimer();
	sampleTimer->resetTimer(timer); 
	sampleTimer->startTimer(timer); 

	if (setupCL() != SDK_SUCCESS)
		return SDK_FAILURE; 

	sampleTimer->stopTimer(timer); 
	setupTime = (double)(sampleTimer->readTimer(timer));

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::run()
{
	for (int i = 0; i < 2 && iterations != 1; i++)
	{
		if (runCLKernels()!= SDK_SUCCESS)
			return SDK_FAILURE; 
	}

	std::cout << "executing kernel for " << iterations << " iterations " << std::endl; 
	std::cout << "-------------------------------------------" << std::endl;

	int timer = sampleTimer->createTimer();
	sampleTimer->resetTimer(timer); 
	sampleTimer->startTimer(timer); 

	for (size_t i = 0; i < iterations; i++)
	{
		if (runCLKernels() != SDK_SUCCESS)
			return SDK_FAILURE; 
	}

	sampleTimer->stopTimer(timer); 

	kernelTime = (double)(sampleTimer->readTimer(timer)) / iterations; 


	//output ready to print

	return SDK_SUCCESS; 
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::verifyResults()
{
	if (args->verify)
	{
		// Rreference implementation on host device
		calApproxFinalOnHost();

		// Compare the results and see if they match
		bool result = true;
		for (cl_uint i = 0; i < signalLength; ++i)
		{
			if (fabs(dOutData[i] - hOutData[i]) > 0.1f)
			{
				result = false;
				break;
			}
		}

		if (result)
		{
			std::cout << "Passed!\n" << std::endl;
			return SDK_SUCCESS;
		}
		else
		{
			std::cout << "Failed\n" << std::endl;
			return SDK_FAILURE;
		}
	}

	return SDK_SUCCESS;
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
void WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::printStats()
{
	if (args->timing)
	{
		std::string strArray[3] = { "SignalLength", "Time(sec)", "[Transfer+Kernel]Time(sec)" };
		sampleTimer->totalTime = setupTime + kernelTime;

		std::string stats[3];
		stats[0] = toString(signalLength, std::dec);
		stats[1] = toString(sampleTimer->totalTime, std::dec);
		stats[2] = toString(kernelTime, std::dec);

		printStatistics(strArray, stats, 3);
	}
}

template<class TInputImage1, class TInputImage2, class TOutputImage>
int WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::cleanup()
{
	// Releases OpenCL resources (Context, Memory etc.)
	cl_int status;

	status = clReleaseMemObject(inDataBuf);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inDataBuf)");

	status = clReleaseMemObject(dOutDataBuf);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(dOutDataBuf)");

	status = clReleaseMemObject(dPartialOutDataBuf);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(dPartialOutDataBuf)");


	status = clReleaseKernel(kernel);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

	status = clReleaseProgram(program);
	CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

	status = clReleaseCommandQueue(commandQueue);
	CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

	status = clReleaseContext(context);
	CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

	// Release program resources (input memory etc.)
	FREE(inData);
	FREE(dOutData);
	FREE(dPartialOutData);
	FREE(hOutData);
	FREE(devices);

	return SDK_SUCCESS;
}

template<class TInputImage1, class TInputImage2, class TOutputImage> 
WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::WaveletImageFilter() :
	signalLength(SIGNAL_LENGTH),
	setupTime(0),
	kernelTime(0),
	inData(NULL),
	dOutData(NULL),
	dPartialOutData(NULL),
	hOutData(NULL),
	devices(NULL),
	classObj(NULL),
	iterations(5)
{
	this->SetNumberOfRequiredInputs(2);
	//this->InPlaceOff();

	args = new CLCommandArgs();
	sampleTimer = new SDKTimer();
	args->sampleVerStr = "AMD - APP - SDK - v2.9.233.1";

	if (this->initialize() != SDK_SUCCESS)
		exit(0); 
		//return SDK_FAILURE; 

	int argc = 0; 
	char**argv = NULL; 
		
	if (args->parseCommandLine(argc, argv) != SDK_SUCCESS)
		//return SDK_FAILURE;
	
	//if (args->isDumpBinaryEnabled())
	genBinary();

	if (this->setup() != SDK_SUCCESS)
		exit(0); //return SDK_FAILURE;
	

}


template<class TInputImage1, class TInputImage2, class TOutputImage> 
void WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::SetInputImage1(const TInputImage1 *img)
{
	SetNthInput(0, const_cast<TInputImage1*> (img));
}

template<class TInputImage1, class TInputImage2, class TOutputImage> 
void WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::SetInputImage2(const TInputImage2 *img)
{
	SetNthInput(1, const_cast<TInputImage2*> (img));
}

template<class T> 
void haar_2d (std::vector<T> &u, int m, int n) 
{
	int i;
 	int j;
	int k;
	double s;
	double *v;

	s = sqrt(2.0);

	v = new double[m*n];

	for (j = 0; j < n; j++)
	{
		for (i = 0; i < m; i++)
		{
			v[i + j*m] = u[i + j*m];
		}
	}

	k = 1;
	while (k * 2 <= m)
	{
		k = k * 2;
	}

	while (1 < k)
	{
		k = k / 2;

		for (j = 0; j < n; j++)
		{
			for (i = 0; i < k; i++)
			{
				v[i + j*m] = (u[2 * i + j*m] + u[2 * i + 1 + j*m]) / s;
				v[k + i + j*m] = (u[2 * i + j*m] - u[2 * i + 1 + j*m]) / s;
			}
		}
		for (j = 0; j < n; j++)
		{
			for (i = 0; i < 2 * k; i++)
			{
				u[i + j*m] = v[i + j*m];
			}
		}
	}
	k = 1;
	while (k * 2 <= n)
	{
		k = k * 2;
	}
	while (1 < k)
	{
		k = k / 2;

		for (j = 0; j < k; j++)
		{
			for (i = 0; i < m; i++)
			{
				v[i + (j)*m] = (u[i + 2 * j*m] + u[i + (2 * j + 1)*m]) / s;
				v[i + (k + j)*m] = (u[i + 2 * j*m] - u[i + (2 * j + 1)*m]) / s;
			}
		}

		for (j = 0; j < 2 * k; j++)
		{
			for (i = 0; i < m; i++)
			{
				u[i + j*m] = v[i + j*m];
			}
		}
	}
	delete[] v;
}

template<class T> 
void haar_2d_inverse (std::vector<T> &u, int m, int n) {
	int i;
	int j;
	int k;
	double s;
	double *v;

	s = sqrt(2.0);
	v = new double[m*n];

	for (j = 0; j < n; j++)
	{
		for (i = 0; i < m; i++)
		{
			v[i + j*m] = u[i + j*m];
		}
	}
	k = 1;

	while (k * 2 <= n)
	{
		for (j = 0; j < k; j++)
		{
			for (i = 0; i < m; i++)
			{
				v[i + (2 * j)*m] = (u[i + j*m] + u[i + (k + j)*m]) / s;
				v[i + (2 * j + 1)*m] = (u[i + j*m] - u[i + (k + j)*m]) / s;
			}
		}

		for (j = 0; j < 2 * k; j++)
		{
			for (i = 0; i < m; i++)
			{
				u[i + j*m] = v[i + j*m];
			}
		}
		k = k * 2;
	}
	k = 1;

	while (k * 2 <= m)
	{
		for (j = 0; j < n; j++)
		{
			for (i = 0; i < k; i++)
			{
				v[2 * i + j*m] = (u[i + j*m] + u[k + i + j*m]) / s;
				v[2 * i + 1 + j*m] = (u[i + j*m] - u[k + i + j*m]) / s;
			}
		}

		for (j = 0; j < n; j++)
		{
			for (i = 0; i < 2 * k; i++)
			{
				u[i + j*m] = v[i + j*m];
			}
		}
		k = k * 2;
	}
	delete[] v;

	return;
}

template<class TInputImage1, class TInputImage2, class TOutputImage> 
void WaveletImageFilter<TInputImage1, TInputImage2, TOutputImage>::ThreadedGenerateData ( const OutputImageRegionType &outputRegionForThread, itk::ThreadIdType threadId)
{
	Input1ImagePointer inputPtr1 = dynamic_cast<const TInputImage1*>(ProcessObject::GetInput(0));
	Input2ImagePointer inputPtr2 = dynamic_cast<const TInputImage2*>(ProcessObject::GetInput(1));

	ImageRegionConstIterator<TInputImage1> inputIt1 (inputPtr1, outputRegionForThread); 
	ImageRegionConstIterator<TInputImage2> inputIt2 (inputPtr2, outputRegionForThread); 

	this->AllocateOutputs();

	OutputImagePointer outputPtr = dynamic_cast< TOutputImage* > (this->GetOutput(0));
	outputPtr->SetRegions(inputPtr1->GetLargestPossibleRegion() );
	outputPtr->Allocate();

	ImageRegionIterator<TOutputImage> outputIt(outputPtr, outputRegionForThread); //outputRegionForThread); outputPtr->GetLargestPossibleRegion()
	ProgressReporter progress (this, threadId, outputRegionForThread.GetNumberOfPixels());
	
	inputIt1.GoToBegin();
	//inputIt2.GoToBegin();
	outputIt.GoToBegin();
	
	if (this->run() != SDK_SUCCESS)
		exit(0); //return SDK_FAILURE;

	if (this->verifyResults() != SDK_SUCCESS)
		exit(0); //return SDK_FAILURE;

	if (this->cleanup() != SDK_SUCCESS)
		exit(0); //return SDK_FAILURE;

	
	printStats();

	exit(1);
	//return SDK_SUCCESS; 

	/*
	int k = 0; 
	while( !outputIt.IsAtEnd() ) {

		outputIt.Set(100); 

		outputIt++; 
		inputIt1++; 

		k++; 
	}

	this->Modified();
	
		
	typename TInputImage1::SizeType size1; 
	size1 = inputPtr1->GetLargestPossibleRegion().GetSize();

	typename TInputImage2::SizeType size2; 
	size2 = inputPtr2->GetLargestPossibleRegion().GetSize();

	int n_row = 0; 
	int n_col = 0; 
	int n_slice = 0; 

	n_row = size1[0]; 
	n_col = size1[1]; 
	n_slice = size1[2]; 
	
	Input1RealType i1; 
	Input1RealType i2; 

	std::vector<float> Img1Coeffs; 
	std::vector<float> Img2Coeffs; 

	while(!inputIt1.IsAtEnd())
	{		
		i1 = inputIt1.Get();
		Img1Coeffs.push_back(i1);
		++inputIt1; 
				
		//progress.CompletedPixel(); 
	}

	while(!inputIt2.IsAtEnd())
	{		
		i2 = inputIt2.Get();
		Img1Coeffs.push_back(i2);
		++inputIt2; 
				
		//progress.CompletedPixel(); 
	}
	
	// calculate wavelet coefficients for image 1
	haar_2d(Img1Coeffs, n_row, n_col);
	haar_2d_inverse(Img1Coeffs, n_row, n_col);

	std::vector<data> m_data1; 
	for(size_t i = 0; i < Img1Coeffs.size(); ++i)
	{
		m_data1.push_back(data(i, Img1Coeffs[i] ));
	}
	
	// calculate wavelet coefficients for image 2
	haar_2d(Img2Coeffs, n_row, n_col);
	haar_2d_inverse(Img2Coeffs, n_row, n_col);

	std::vector<data> m_data2; 
	for(size_t i = 0; i < Img2Coeffs.size(); ++i)
	{
		m_data2.push_back(data(i, Img2Coeffs[i] ));
	}

	std::sort(m_data1.begin(), m_data1.end() );
	std::sort(m_data2.begin(), m_data2.end() ); 

	int k = 0; 
	while(!inputIt1.IsAtEnd()) {
		outputIt.Set(inputIt1.Get()); 
		outputIt++; 
		inputIt1++; 

		std::cout << inputIt1.Get();
		k++; 
	} 
	
	//int i = 0; 
	//while(!inputIt1.IsAtEnd())
	{
		//outputIt.Set(m_data1[i].value);	
		//++outputIt;
		//++i; 
	}
	writer->SetInput(outputIt.Get());
	

	progress.CompletedPixel(); 		
		
		*/
}	//ThreadedGenerateData

}	//end namespace itk

#endif

