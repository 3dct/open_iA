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
#include "iAWatershedSegmentation.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkMorphologicalWatershedImageFilter.h>
#include <itkScalarToRGBPixelFunctor.h>
#include <itkUnaryFunctorImageFilter.h>
#include <itkWatershedImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>


// Watershed segmentation 

template<class T> 
void watershed_template( double l, double t, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::WatershedImageFilter < InputImageType > WIFType;
	auto filter = WIFType::New();
	filter->SetLevel ( l );
	filter->SetThreshold ( t);
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	p->Observe( filter );
	filter->Update();
	typedef itk::Image< typename WIFType::OutputImagePixelType, DIM > IntImageType;
	typedef itk::Image<	unsigned long, DIM>  LongImageType;
	typedef itk::CastImageFilter< IntImageType, LongImageType > CastFilterType;
	auto longcaster = CastFilterType::New();
	longcaster->SetInput(0, filter->GetOutput() );
	image->SetImage( longcaster->GetOutput() );
	image->Modified();
}

IAFILTER_CREATE(iAWatershed)

iAWatershed::iAWatershed() :
	iAFilter("Watershed", "Segmentation/Based on Watershed",
		"Computes a watershed segmentation the input image.<br/>"
		"As input image use for example a gradient magnitude image.<br/>"
		"Both parameters <em>Threshold</em> and <em>Level</em> are percentage "
		"points of the maximum height value in the input (they must be in the "
		"interval [0..1]).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1WatershedImageFilter.html\">"
		"Watershed filter</a> in the ITK documentation.")
{
	AddParameter("Level", Continuous, 0);
	AddParameter("Threshold", Continuous, 0);
}

void iAWatershed::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(watershed_template, itkType,
		parameters["Level"].toDouble(),
		parameters["Threshold"].toDouble(),
		m_progress, m_con);
}


// Morphological Watershed

template<class T>
void morph_watershed_template( double mwsLevel, bool mwsMarkWSLines, bool mwsFullyConnected, iAProgress* p,
							  iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< unsigned long, DIM > OutputImageType;
	typedef itk::MorphologicalWatershedImageFilter<InputImageType, OutputImageType> MWIFType;
	auto mWSFilter = MWIFType::New();
	mwsMarkWSLines ? mWSFilter->MarkWatershedLineOn() : mWSFilter->MarkWatershedLineOff();
	mwsFullyConnected ? mWSFilter->FullyConnectedOn() : mWSFilter->FullyConnectedOff();
	mWSFilter->SetLevel( mwsLevel );
	mWSFilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	p->Observe( mWSFilter );
	mWSFilter->Update();
	typedef itk::Image< typename MWIFType::OutputImagePixelType, DIM > IntImageType;
	typedef itk::Image<	unsigned long, DIM>  LongImageType;
	typedef itk::CastImageFilter< IntImageType, LongImageType > CastFilterType;
	auto longcaster = CastFilterType::New();
	longcaster->SetInput( 0, mWSFilter->GetOutput() );
	image->SetImage( longcaster->GetOutput() );
	image->Modified();
}

IAFILTER_CREATE(iAMorphologicalWatershed)

iAMorphologicalWatershed::iAMorphologicalWatershed() :
	iAFilter("Morphological Watershed", "Segmentation/Based on Watershed",
		"Calculates the Morphological Watershed Transformation.<br/>"
		"As input image use for example a gradient magnitude image.<br/>"
		"<em>Mark WS Line</em> labels watershed lines with 0, background with 1.<br/>"
		"For further information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MorphologicalWatershedImageFilter.html\">"
		"Morphological Watershed filter</a> in the ITK documentation.</p>")
{
	AddParameter("Level", Continuous, 0);
	AddParameter("Mark WS Lines", Boolean, false);
	AddParameter("Fully Connected", Boolean, false);
}

void iAMorphologicalWatershed::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(morph_watershed_template, itkType,
		parameters["Level"].toDouble(),
		parameters["Mark WS Lines"].toBool(),
		parameters["Fully Connected"].toBool(),
		m_progress, m_con);
}
