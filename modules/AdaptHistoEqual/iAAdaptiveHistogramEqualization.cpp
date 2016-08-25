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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iAAdaptiveHistogramEqualization.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkAdaptiveHistogramEqualizationImageFilter.h>
#include <itkImageFileWriter.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* template computes Adaptive Histogram Equalization
* 
* This template is used for calculating the Adaptive Histogram Equalization
* \param	Alpha			Alpha controls how much the filter acts like the classical histogram equalization
* \param	Beta			Beta controls how much the filter acts like an unsharp mask
* \param	p				Filter progress information. 
* \param	image			Input image. 
* \param	T				Input type 
* \return	int Status-Code. 
*/
template<class T> int iAAdaptiveHistogramEqualization_template( double aheAlpha, double aheBeta, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	
	typename InputImageType::Pointer castImage;
	castImage = dynamic_cast< InputImageType * >( image->GetITKImage() );

	typedef  itk::AdaptiveHistogramEqualizationImageFilter< InputImageType > AdaptiveHistogramEqualizationImageFilterType;
	typename AdaptiveHistogramEqualizationImageFilterType::Pointer adaptiveHistogramEqualizationImageFilter = AdaptiveHistogramEqualizationImageFilterType::New();
	adaptiveHistogramEqualizationImageFilter->SetInput( castImage );
	adaptiveHistogramEqualizationImageFilter->SetAlpha( aheAlpha );
	adaptiveHistogramEqualizationImageFilter->SetBeta( aheBeta );
	adaptiveHistogramEqualizationImageFilter->SetRadius( 1 );
	 
	p->Observe( adaptiveHistogramEqualizationImageFilter );

	adaptiveHistogramEqualizationImageFilter->Update();
	  
	image->SetImage(adaptiveHistogramEqualizationImageFilter->GetOutput());
	image->Modified();

	adaptiveHistogramEqualizationImageFilter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* constructor. 
* \param	fn		Filter name. 
* \param	fid		Filter ID number. 
* \param	i		Input image data. 
* \param	p		Input vtkpolydata. 
* \param	w		Input widget list. 
* \param	parent	Parent object. 
*/

iAAdaptiveHistogramEqualization::iAAdaptiveHistogramEqualization( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent)
	: iAFilter( fn, fid, i, p, logger, parent )
{

}

/**
* destructor. 
*/

iAAdaptiveHistogramEqualization::~iAAdaptiveHistogramEqualization()
{
}

/**
* Execute the filter thread. 
*/

void iAAdaptiveHistogramEqualization::run()
{
	switch (getFilterID())
	{
	case ADAPTIVE_HISTOGRAM_EQUALIZATION:
		compute_iAAdaptiveHistogramEqualization(); break;
	case UNKNOWN_FILTER: 
	default:
		addMsg(tr("  unknown filter type"));
	}
}

/**
* Computes Adaptive Histogram Equalization of this object. 
*/

void iAAdaptiveHistogramEqualization::compute_iAAdaptiveHistogramEqualization( )
{
	addMsg(tr("%1  %2 started (Alpha: %3 Beta: %4)").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()).arg(this->aheAlpha).arg(this->aheBeta));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();
	
	try
	{
		VTK_TYPED_CALL(iAAdaptiveHistogramEqualization_template, getVtkImageData()->GetScalarType(),
			aheAlpha, aheBeta, getItkProgress(), getConnector());
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
