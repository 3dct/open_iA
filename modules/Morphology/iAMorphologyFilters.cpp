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
#include "pch.h"
#include "iAMorphologyFilters.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkBinaryBallStructuringElement.h>
#include <itkFFTNormalizedCorrelationImageFilter.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkHessian3DToVesselnessMeasureImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>
#include <itkNormalizedCorrelationImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* Dilation filter template for different data types .
* \param	c		radius. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype. 
* \return	int		Status code 
*/
template<class T> 
int DilationFilter_template(int c, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< long, 3 >   OutputImageType;

	typedef itk::BinaryBallStructuringElement<typename InputImageType::PixelType,3> StructuringElementType;
	StructuringElementType structuringElement;
	structuringElement.SetRadius(c);
	structuringElement.CreateStructuringElement();

	typedef itk::GrayscaleDilateImageFilter <InputImageType, InputImageType, StructuringElementType>
		GrayscaleDilateImageFilterType;


	typename GrayscaleDilateImageFilterType::Pointer dilateFilter = GrayscaleDilateImageFilterType::New();
	dilateFilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	dilateFilter->SetKernel(structuringElement);

	p->Observe( dilateFilter );

	dilateFilter->Update(); 
	image->SetImage(dilateFilter->GetOutput());
	image->Modified();

	dilateFilter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* Erosion filter template for different data types .
* \param	c		radius. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype. 
* \return	int		Status code 
*/
template<class T> 
int ErosionFilter_template(int c, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< long, 3 >   OutputImageType;

	typedef itk::BinaryBallStructuringElement<typename InputImageType::PixelType,3> StructuringElementType;
	StructuringElementType structuringElement;
	structuringElement.SetRadius(c);
	structuringElement.CreateStructuringElement();

	typedef itk::GrayscaleErodeImageFilter <InputImageType, InputImageType, StructuringElementType>
		GrayscaleErodeImageFilterType;


	typename GrayscaleErodeImageFilterType::Pointer erodeFilter = GrayscaleErodeImageFilterType::New();
	erodeFilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	erodeFilter->SetKernel(structuringElement);

	p->Observe( erodeFilter );

	erodeFilter->Update();
	image->SetImage(erodeFilter->GetOutput());
	image->Modified();

	erodeFilter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* Enhancement filter template for different data types .
* \param	s		sigma. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype. 
* \return	int		Status code 
*/
template<class T> int EnhancementFilter_template(int s, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;

	typedef itk::Hessian3DToVesselnessMeasureImageFilter<typename InputImageType::PixelType> EnhancementFilter; 

	// ITK Hessian
	//hesse image
	typedef itk::HessianRecursiveGaussianImageFilter<InputImageType> HRGIFType;
	itk::SmartPointer< HRGIFType >  hessfilter = HRGIFType::New();
	hessfilter->SetInput(dynamic_cast< InputImageType * >( image->GetITKImage() ));
	hessfilter->SetSigma(s);
	hessfilter->Update();

	typename EnhancementFilter::Pointer vesselness = EnhancementFilter::New();
	vesselness->SetInput( hessfilter->GetOutput() );
	vesselness->Update();

	image->SetImage(vesselness->GetOutput());
	image->Modified();

	return EXIT_SUCCESS;
}

iAMorphologyFilters::iAMorphologyFilters( QString fn, iAMorphologyOperationType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, i, p, logger, parent ), m_type(fid)
{}


void iAMorphologyFilters::run()
{
	switch (m_type)
	{
	case DILATION_FILTER: 
		DilationFilter(); break;
	case EROSION_FILTER: 
		ErosionFilter(); break;
	case VESSEL_ENHANCEMENT_FILTER: 
		VesselEnhancementFilter(); break;
	default:
		addMsg(tr("unknown filter type"));
	}
}

void iAMorphologyFilters::DilationFilter( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(DilationFilter_template, itkType,
			r, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}

void iAMorphologyFilters::ErosionFilter( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(ErosionFilter_template, itkType,
			r, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}

void iAMorphologyFilters::VesselEnhancementFilter( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(EnhancementFilter_template, itkType,
			r, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}
