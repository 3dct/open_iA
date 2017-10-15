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

#include <vtkImageData.h>

#include <QLocale>

iAGPUEdgePreservingSmoothing::iAGPUEdgePreservingSmoothing(QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent)
	: iAAlgorithm(fn, i, p, logger, parent)
{}


/**
* GPU Gradient anisotropic diffusion template initializes itkGPUGradientAnisotropicDiffusionImageFilter .
* \param	i		NumberOfIterations.
* \param	t		TimeStep.
* \param	c		ConductanceParameter.
* \param	p		Filter progress information.
* \param	image	Input image.
* \param	T		Template datatype.
* \return	int		Status code.
*/
template<class T>
int GPU_gradient_anisotropic_diffusion_template(unsigned int i, double t, double c, iAProgress* p, iAConnector* image)
{
	// register object factory for GPU image and filter

	itk::ObjectFactoryBase::RegisterFactory(itk::GPUImageFactory::New());
	itk::ObjectFactoryBase::RegisterFactory(itk::GPUGradientAnisotropicDiffusionImageFilterFactory::New());

	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;

	typedef itk::GPUGradientAnisotropicDiffusionImageFilter< InputImageType, RealImageType > GGADIFType;
	typename GGADIFType::Pointer filter = GGADIFType::New();

	filter->SetNumberOfIterations(i);
	filter->SetTimeStep(t);
	filter->SetConductanceParameter(c);
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));

	p->Observe(filter);

	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}


void iAGPUEdgePreservingSmoothing::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL(GPU_gradient_anisotropic_diffusion_template, itkType,
		iterations, timestep, conductance, getItkProgress(), getConnector());
}