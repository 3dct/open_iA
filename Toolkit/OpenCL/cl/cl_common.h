#ifndef CL_COMMON_H
#define CL_COMMON_H

#include <itkMacro.h>
#include <QtGui>
#include <QMessageBox>
#include "iAmat4.h"

// When the AMD OpenCL 1.2 installed, we need this line to do the work
// if it does not work on your PC, just uncomment the following line 
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include "cl.hpp"

#ifndef __APPLE__
#ifndef __MACOSX
#include "CL/cl.h"
#endif
#endif

inline char const * descriptionOfError(cl_int err)
{
	switch (err) 
	{
	case CL_SUCCESS:                            return "Success!";
	case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
	case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
	case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
	case CL_OUT_OF_RESOURCES:                   return "Out of resources";
	case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
	case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
	case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
	case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
	case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
	case CL_MAP_FAILURE:                        return "Map failure";
	case CL_INVALID_VALUE:                      return "Invalid value";
	case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
	case CL_INVALID_PLATFORM:                   return "Invalid platform";
	case CL_INVALID_DEVICE:                     return "Invalid device";
	case CL_INVALID_CONTEXT:                    return "Invalid context";
	case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
	case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
	case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
	case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
	case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
	case CL_INVALID_SAMPLER:                    return "Invalid sampler";
	case CL_INVALID_BINARY:                     return "Invalid binary";
	case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
	case CL_INVALID_PROGRAM:                    return "Invalid program";
	case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
	case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
	case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
	case CL_INVALID_KERNEL:                     return "Invalid kernel";
	case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
	case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
	case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
	case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
	case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
	case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
	case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
	case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
	case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
	case CL_INVALID_EVENT:                      return "Invalid event";
	case CL_INVALID_OPERATION:                  return "Invalid operation";
	case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
	case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
	case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
	default: return "Unknown";
	}
}

#define itk_clSafeCall(err)\
	if(CL_SUCCESS != err)\
{\
	char buffer[200];\
	sprintf (buffer, "An error occurred in OpenCL function call! Error: \"%s\"", descriptionOfError(err));\
	throw itk::ExceptionObject(__FILE__, __LINE__, buffer);\
}

#define itk_clThrowBuildLog(log)\
{\
	char buffer[99999];\
	sprintf (buffer, "An error occurred during OpenCL build. Log:\n \"%s\"", log.data());\
	throw itk::ExceptionObject(__FILE__, __LINE__, buffer);\
}
//TODO: remove inline?
inline QString readFile(QString filename) 
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) 
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "Cannot open OpenCL source file!");
		return 0;
	}

	QByteArray total;
	QByteArray line;
	while (!file.atEnd()) 
	{
		line = file.readLine(1024);
		total.append(line);
	}

	return QString(total);
}

inline int generateEmbeddableSource(QString src_filename, QString output_filename)
{
	QString toEmbed = "";
	QFile file(src_filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw itk::ExceptionObject(__FILE__, __LINE__, "Cannot open OpenCL source file!");
		return 0;
	}

	QByteArray line;
	while (!file.atEnd()) 
	{
		line = file.readLine(1024);
		toEmbed.append(QString("\"") + QString(line).remove("\n") + QString(" \\n\"\n"));
	}

	QFile out(output_filename);
	if( !out.open(QIODevice::WriteOnly | QIODevice::Text) )
		return 0;

	QTextStream outstream(&out);
	outstream << toEmbed;
	out.close();
	return 1;
}

inline cl_float4 Vec3_to_cl_float4(const iAVec3 & v)
{
	cl_float4 res = {v.x, v.y, v.z, 0.0f};
	return res;
}

/**
* Get best fitting 1D local size given the global size and an OpenCL kernel.
* Global size should be dividable by local size. And local size should not 
* be more than the maximum number of threads that kernel can handle.
*/
inline size_t GetLocalForKernel(const cl::Kernel kernel, const size_t global, const cl::Device device)
{
	size_t local; 
	itk_clSafeCall(  kernel.getWorkGroupInfo(device, CL_KERNEL_WORK_GROUP_SIZE, &local)  );
	while( 0 != global%local ) local--;
	return local;
}

inline void cl_init(cl::Device			& device_out, 
					cl::Context			& context_out, 
					cl::CommandQueue	& queue_out,
					cl_device_type		devType = CL_DEVICE_TYPE_GPU)
{

	std::vector<cl::Platform>	platforms;
	itk_clSafeCall(  cl::Platform::get(&platforms)  );

	std::vector<cl::Device>	devices;
	bool noDevices = true;
	cl_uint maxCU = 0, maxClock = 0;
	for(std::vector<cl::Platform>::const_iterator p = platforms.begin(); p != platforms.end(); ++p) 
	{
		try
		{
			p->getDevices(devType, &devices);
		}
		catch (...)
		{
			continue;
		}
		if( !devices.empty() ) 
		{
			noDevices = false;
			for(std::vector<cl::Device>::const_iterator d = devices.begin(); d != devices.end(); ++d) 
			{
				cl_uint CU = 0, Clock = 0;
                char buf[128];
                itk_clSafeCall( clGetDeviceInfo((*d)(), CL_DEVICE_VENDOR, 128, buf, NULL) );
				itk_clSafeCall( clGetDeviceInfo((*d)(), CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &CU,	 NULL) );
				itk_clSafeCall( clGetDeviceInfo((*d)(), CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &Clock, NULL) );
                QString deviceVendor( buf );
                if( devType == CL_DEVICE_TYPE_GPU && deviceVendor.contains("Intel", Qt::CaseInsensitive) ) //skip intel's gpu
                    continue;
				if(CU*Clock > maxCU*maxClock)
				{
					maxCU = CU; maxClock = Clock;

					//Initialize context, queue, and device 
					cl_context_properties	properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)(*p)(), 0};
					device_out = (*d)();
					context_out = cl::Context(devices, properties);
					queue_out = cl::CommandQueue(context_out, device_out);// create cl command queues
				}
			}			
		}
	}
	if(noDevices)
	{
		char buffer[1024];
		sprintf (buffer, "An error occurred in OpenCL initialization! Error: \"%s\"", "no cl devices were found");
		throw itk::ExceptionObject(__FILE__, __LINE__, buffer);
	}
}

#endif //CL_COMMON_H
