/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAGPUEdgePreservingSmoothing.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkGPUImage.h>
#include <itkGPUKernelManager.h>
#include <itkGPUContextManager.h>
#include <itkGPUImageToImageFilter.h>
#include <itkGPUGradientAnisotropicDiffusionImageFilter.h>


template<class T>
void GPU_gradient_anisotropic_diffusion_template(QMap<QString, QVariant> const & parameters,
	iAProgress* p, iAConnector* image)
{
	// register object factory for GPU image and filter
	itk::ObjectFactoryBase::RegisterFactory(itk::GPUImageFactory::New());
	itk::ObjectFactoryBase::RegisterFactory(itk::GPUGradientAnisotropicDiffusionImageFilterFactory::New());

	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;
	typedef itk::GPUGradientAnisotropicDiffusionImageFilter< InputImageType, RealImageType > GGADIFType;

	auto filter = GGADIFType::New();
	filter->SetNumberOfIterations(parameters["Number of Iterations"].toUInt());
	filter->SetTimeStep(parameters["Time Step"].toDouble());
	filter->SetConductanceParameter(parameters["Conductance"].toDouble());
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

void iAGPUEdgePreservingSmoothing::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(GPU_gradient_anisotropic_diffusion_template, m_con->GetITKScalarPixelType(),
		parameters, m_progress, m_con);
}

IAFILTER_CREATE(iAGPUEdgePreservingSmoothing)

iAGPUEdgePreservingSmoothing::iAGPUEdgePreservingSmoothing() :
	iAFilter("Gradient Anisotropic Diffusion (GPU)", "Smoothing/Edge preserving smoothing",
		"Performs GPU-accelerated gradient anisotropic diffusion.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GPUGradientAnisotropicDiffusionImageFilter.html\">"
		"GPU Gradient Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	AddParameter("Number of Iterations", Discrete, 100, 1);
	AddParameter("Time Step", Continuous, 0.0625);
	AddParameter("Conductance", Continuous, 1);
}