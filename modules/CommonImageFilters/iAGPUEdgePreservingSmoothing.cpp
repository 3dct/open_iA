/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#ifndef ITKNOGPU
#include "iAGPUEdgePreservingSmoothing.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#define CL_TARGET_OPENCL_VERSION 220
#include <itkGPUImage.h>
#include <itkGPUKernelManager.h>
#include <itkGPUContextManager.h>
#include <itkGPUImageToImageFilter.h>
#include <itkGPUGradientAnisotropicDiffusionImageFilter.h>

template<class T>
void GPU_gradient_anisotropic_diffusion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	// register object factory for GPU image and filter
	itk::ObjectFactoryBase::RegisterFactory(itk::GPUImageFactory::New());
	itk::ObjectFactoryBase::RegisterFactory(itk::GPUGradientAnisotropicDiffusionImageFilterFactory::New());

	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;
	typedef itk::GPUGradientAnisotropicDiffusionImageFilter< InputImageType, RealImageType > GGADIFType;

	auto gadFilter = GGADIFType::New();
	gadFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	gadFilter->SetTimeStep(params["Time Step"].toDouble());
	gadFilter->SetConductanceParameter(params["Conductance"].toDouble());
	gadFilter->SetInput(dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()));
	filter->progress()->observe(gadFilter);
	gadFilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(gadFilter->GetOutput()));
	else
		filter->addOutput(gadFilter->GetOutput());
}

void iAGPUEdgePreservingSmoothing::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(GPU_gradient_anisotropic_diffusion, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAGPUEdgePreservingSmoothing)

iAGPUEdgePreservingSmoothing::iAGPUEdgePreservingSmoothing() :
	iAFilter("Gradient Anisotropic Diffusion (GPU)", "Smoothing/Edge preserving smoothing",
		"Performs GPU-accelerated gradient anisotropic diffusion.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GPUGradientAnisotropicDiffusionImageFilter.html\">"
		"GPU Gradient Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", Discrete, 100, 1);
	addParameter("Time step", Continuous, 0.0625);
	addParameter("Conductance", Continuous, 1);
	addParameter("Convert back to input type", Boolean, false);
}
#endif