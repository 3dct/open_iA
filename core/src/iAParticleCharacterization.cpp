/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAParticleCharacterization.h"

#include "iAConnector.h"
#include "iAProgress.h"

#include <itkAdaptiveHistogramEqualizationImageFilter.h>
#include <itkDerivativeImageFilter.h>
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkLaplacianImageFilter.h>
#include <itkLaplacianSegmentationLevelSetFunction.h>
#include <itkZeroCrossingImageFilter.h>

/*HistEqual*/

#include <QLocale>

/**
* template computeHessian
* 
* This template is used for calculating the hessian matrix
* \param	sigma			Sigma
* \param	hessianComputed Is the hessian matrix already computed.
* \param	p				Filter progress information. 
* \param	image			Input image. 
* \param	T				Input type 
* \return	int Status-Code. 
*/
template<class T> int particalCharacterization_template( int param1, iAProgress* p, iAConnector* image, T)
{
	/** Der Code sollt hier beginnen **/
	/** Das hier ist nur Platzhalter. Berechnung der Ableitung. **/


	typedef itk::Image< T, 3 >   InputImageType;

	typename InputImageType::Pointer castImage;
	castImage = dynamic_cast< InputImageType * >( image->GetITKImage() );

	//typedef itk::Image< float, 3 >   RealImageType;

	//typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
	//typename CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
	//toReal->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );

	//typedef itk::DerivativeImageFilter< RealImageType, RealImageType > DIFType;
	//DIFType::Pointer filter = DIFType::New();

	//filter->SetOrder( param1 );
	//filter->SetDirection( param1 );
	//filter->SetInput( toReal->GetOutput() );
	////filter->SetInput( image->GetITKImage()  );
	////filter->SetInput( image->GetITKImage() );

	//p->Observe( filter );

	//filter->Update(); 

	//image->SetImage(filter->GetOutput());
	//image->Modified();

	//filter->ReleaseDataFlagOn();

		/*
		The parameter alpha controls how much the filter acts like the classical histogram
		equalization method (alpha=0) to how much the filter acts like an unsharp mask (alpha=1).
		
		The parameter beta controls how much the filter acts like an unsharp mask (beta=0) to
		much the filter acts like pass through (beta=1, with alpha=1). 
		*/
	  typedef  itk::AdaptiveHistogramEqualizationImageFilter< InputImageType > AdaptiveHistogramEqualizationImageFilterType;
	  typename AdaptiveHistogramEqualizationImageFilterType::Pointer adaptiveHistogramEqualizationImageFilter = AdaptiveHistogramEqualizationImageFilterType::New();
	  adaptiveHistogramEqualizationImageFilter->SetInput( castImage );
	  adaptiveHistogramEqualizationImageFilter->SetAlpha(1);
	  adaptiveHistogramEqualizationImageFilter->SetBeta(0);
	  adaptiveHistogramEqualizationImageFilter->SetRadius(1);
	 
	  p->Observe( adaptiveHistogramEqualizationImageFilter );

	  adaptiveHistogramEqualizationImageFilter->Update();
	  
	  image->SetImage(adaptiveHistogramEqualizationImageFilter->GetOutput());
	  image->Modified();

	  adaptiveHistogramEqualizationImageFilter->ReleaseDataFlagOn();

 	return EXIT_SUCCESS;
}

iAParticleCharacterization::iAParticleCharacterization( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent)
	: iAFilter( fn, fid, i, p, logger, parent )
{
	
}

iAParticleCharacterization::~iAParticleCharacterization()
{
}

void iAParticleCharacterization::run()
{
	switch (getFilterID())
	{
	case COMPUTEPARTICLETEST:
		compute_particletest(); break;
	case UNKNOWN_FILTER: 
	default:
		addMsg(tr("  unknown filter type"));
	}
}

void iAParticleCharacterization::compute_particletest( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		switch (getVtkImageData()->GetScalarType()) // This filter handles all types
		{
		case VTK_UNSIGNED_CHAR:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<unsigned char>(0));addMsg("--> 1"); break;
		case VTK_CHAR:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<char>(0));addMsg("--> 2"); break;
		case VTK_UNSIGNED_SHORT:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<unsigned short>(0));addMsg("--> 3"); break;
		case VTK_SHORT:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<short>(0));addMsg("--> 4"); break;
		case VTK_UNSIGNED_INT:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<unsigned int>(0));addMsg("--> 5");  break;
		case VTK_INT:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<int>(0));addMsg("--> 6"); break;
		case VTK_UNSIGNED_LONG:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<unsigned long>(0));addMsg("--> 7"); break;
		case VTK_LONG:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<long>(0));addMsg("--> 8"); break;
		case VTK_FLOAT:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<float>(0));addMsg("--> 9"); break;
		case VTK_DOUBLE:
			particalCharacterization_template( param1, getItkProgress(), getConnector(), static_cast<double>(0));addMsg("--> 10"); break;
		default:
			addMsg(tr("  unknown component type"));
			return;
		}
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms. For learning only.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3. For learning only.").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}
