// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>          // for DIM
#include <iAImageData.h>
#include <iAFilterDefault.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkMorphologicalWatershedImageFilter.h>
#include <itkWatershedImageFilter.h>
#pragma GCC diagnostic pop

IAFILTER_DEFAULT_CLASS(iAWatershed);
IAFILTER_DEFAULT_CLASS(iAMorphologicalWatershed);

// Watershed segmentation

template<class T>
void watershed(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM >   InputImageType;
	auto wsFilter = itk::WatershedImageFilter<InputImageType>::New();
	wsFilter->SetLevel ( parameters["Level"].toDouble() );
	wsFilter->SetThreshold ( parameters["Threshold"].toDouble() );
	wsFilter->SetInput( dynamic_cast< InputImageType * >( filter->imageInput(0)->itkImage() ) );
	filter->progress()->observe( wsFilter );
	wsFilter->Update();
	// return is unsigned long long, but vtk can't handle that, so convert to ulong:
	filter->addOutput(std::make_shared<iAImageData>(castImageTo<unsigned long>(wsFilter->GetOutput())) );
}

iAWatershed::iAWatershed() :
	iAFilter("Watershed", "Segmentation/Based on Watershed",
		"Computes a watershed segmentation the input image.<br/>"
		"As input image use for example a gradient magnitude image.<br/>"
		"Both parameters <em>Threshold</em> and <em>Level</em> are percentage "
		"points of the maximum height value in the input (they must be in the "
		"interval [0..1]).<br/>"
		"The <em>Level</em> parameter controls the depth of metaphorical flooding of the image. "
		"That is, it sets the maximum saliency value of interest in the result. Raising "
		"and lowering the Level influences the number of segments in the basic segmentation "
		"that are merged to produce the final output. A level of 1.0 is analogous to "
		"flooding the image up to a depth that is 100 percent of the maximum value in the "
		"image. A level of 0.0 produces the basic segmentation, which will typically be "
		"very oversegmented. Level values of interest are typically low (i.e. less than "
		"about 0.40 or 40% ), since higher values quickly start to undersegment the image."
		"The Level parameter can be used to create a hierarchy of output images in "
		"constant time once an initial segmentation is done.A typical scenario might go "
		"like this: For the initial execution of the filter, set the Level to the maximum "
		"saliency value that you anticipate might be of interest.<br/>"
		"<em>Threshold</em> is used to set the absolute minimum height value used during processing. "
		"Raising this threshold percentage effectively decreases the number of local minima "
		"in the input, resulting in an initial segmentation with fewer regions. The "
		"assumption is that the shallow regions that thresholding removes are of of less "
		"interest.<br/>"
		"For more information, see the article "
		"<a href=\"https://www.insight-journal.org/browse/publication/92/\">"
		"The watershed transform in ITK - discussion and new developments</a> "
		"in the ITK journal, as well as the information on"
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1WatershedImageFilter.html\">"
		"Watershed filter in the ITK documentation</a>.")
{
	addParameter("Level", iAValueType::Continuous, 0);
	addParameter("Threshold", iAValueType::Continuous, 0);
}

void iAWatershed::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(watershed, inputScalarType(), this, parameters);
}


// Morphological Watershed

template<class T>
void morph_watershed(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< unsigned long, DIM > OutputImageType;
	auto mWSFilter = itk::MorphologicalWatershedImageFilter<InputImageType, OutputImageType>::New();
	mWSFilter->SetMarkWatershedLine(parameters["Mark WS Lines"].toBool());
	mWSFilter->SetFullyConnected(parameters["Fully Connected"].toBool());
	mWSFilter->SetLevel( parameters["Level"].toDouble() );
	mWSFilter->SetInput( dynamic_cast< InputImageType * >( filter->imageInput(0)->itkImage() ) );
	filter->progress()->observe( mWSFilter );
	mWSFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(mWSFilter->GetOutput()) );
}

iAMorphologicalWatershed::iAMorphologicalWatershed() :
	iAFilter("Morphological Watershed", "Segmentation/Based on Watershed",
		"Calculates the Morphological Watershed Transformation.<br/>"
		"As input image use for example a gradient magnitude image.<br/>"
		"<em>Mark WS Line</em> labels watershed lines with 0, background with 1.<br/>"
		"For further information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MorphologicalWatershedImageFilter.html\">"
		"Morphological Watershed filter</a> in the ITK documentation.</p>")
{
	addParameter("Level", iAValueType::Continuous, 0);
	addParameter("Mark WS Lines", iAValueType::Boolean, false);
	addParameter("Fully Connected", iAValueType::Boolean, false);
}

void iAMorphologicalWatershed::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(morph_watershed, inputScalarType(), this, parameters);
}
