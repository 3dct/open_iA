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
#include "iAWatershedSegmentation.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkMorphologicalWatershedImageFilter.h>
#include <itkWatershedImageFilter.h>


// Watershed segmentation 

template<class T> 
void watershed(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::WatershedImageFilter < InputImageType > WIFType;
	auto wsFilter = WIFType::New();
	wsFilter->SetLevel ( parameters["Level"].toDouble() );
	wsFilter->SetThreshold ( parameters["Threshold"].toDouble() );
	wsFilter->SetInput( dynamic_cast< InputImageType * >( filter->input()[0]->itkImage() ) );
	filter->progress()->observe( wsFilter );
	wsFilter->Update();
	filter->addOutput( castImageTo<unsigned long>(wsFilter->GetOutput()) );
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
	addParameter("Level", Continuous, 0);
	addParameter("Threshold", Continuous, 0);
}

void iAWatershed::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(watershed, inputPixelType(), this, parameters);
}


// Morphological Watershed

template<class T>
void morph_watershed(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< unsigned long, DIM > OutputImageType;
	typedef itk::MorphologicalWatershedImageFilter<InputImageType, OutputImageType> MWIFType;
	auto mWSFilter = MWIFType::New();
	mWSFilter->SetMarkWatershedLine(parameters["Mark WS Lines"].toBool());
	mWSFilter->SetFullyConnected(parameters["Fully Connected"].toBool());
	mWSFilter->SetLevel( parameters["Level"].toDouble() );
	mWSFilter->SetInput( dynamic_cast< InputImageType * >( filter->input()[0]->itkImage() ) );
	filter->progress()->observe( mWSFilter );
	mWSFilter->Update();
	filter->addOutput( castImageTo<unsigned long>(mWSFilter->GetOutput()) );
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
	addParameter("Level", Continuous, 0);
	addParameter("Mark WS Lines", Boolean, false);
	addParameter("Fully Connected", Boolean, false);
}

void iAMorphologicalWatershed::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(morph_watershed, inputPixelType(), this, parameters);
}
