// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASegmentationMetrics.h"

#include <iADataSet.h>
#include <iALog.h>
#include <iATypedCallHelper.h>

#include <itkLabelOverlapMeasuresImageFilter.h>

template <typename ImagePixelType>
void CalculateSegmentationMetrics(iAFilter* filter)
{
	typedef itk::Image<ImagePixelType, 3> ImageType;
	typedef typename ImageType::Pointer ImagePointer;
	typedef itk::LabelOverlapMeasuresImageFilter<ImageType > DiceFilterType;
	auto diceFilter = DiceFilterType::New();
	ImagePointer groundTruthPtr = dynamic_cast<ImageType*>(filter->imageInput(0)->itkImage());
	ImagePointer segmentedPtr = dynamic_cast<ImageType*>(filter->imageInput(1)->itkImage());
	if (!groundTruthPtr || !segmentedPtr)
	{
		LOG(lvlError, "Input images do not have the same type, but are required to!");
		return;
	}
	diceFilter->SetSourceImage(groundTruthPtr);
	diceFilter->SetTargetImage(segmentedPtr);
	diceFilter->Update();

	filter->addOutputValue("Total Overlap", diceFilter->GetTotalOverlap());
	filter->addOutputValue("Union Overlap (Jaccard)", diceFilter->GetUnionOverlap());
	filter->addOutputValue("Mean Overlap (Dice)", diceFilter->GetMeanOverlap());
	filter->addOutputValue("Volume Similarity", diceFilter->GetVolumeSimilarity());
	//filter->addOutputValue("False negatives", diceFilter->GetFalseNegativeError());
	//filter->addOutputValue("False positives", diceFilter->GetFalsePositiveError());

	typename DiceFilterType::MapType labelMap = diceFilter->GetLabelSetMeasures();
	typename DiceFilterType::MapType::const_iterator it;
	for (it = labelMap.begin(); it != labelMap.end(); ++it)
	{
		if ((*it).first == 0)
		{
			continue;
		}
		int label = (*it).first;
		filter->addOutputValue(QString("Label %1 Target Overlap").arg(label), diceFilter->GetTargetOverlap(label));
		filter->addOutputValue(QString("Label %1 Union Overlap").arg(label), diceFilter->GetUnionOverlap(label));
		filter->addOutputValue(QString("Label %1 Mean Overlap").arg(label), diceFilter->GetMeanOverlap(label));
		filter->addOutputValue(QString("Label %1 Volume Similarity").arg(label), diceFilter->GetVolumeSimilarity(label));
		//filter->addOutputValue(QString("Label %1 False negatives").arg(label), diceFilter->GetFalseNegativeError(label));
		//filter->addOutputValue(QString("Label %1 False positives").arg(label), diceFilter->GetFalsePositiveError(label));
	}
}
iASegmentationMetrics::iASegmentationMetrics() :
	iAFilter("Segmentation Quality", "Metrics",
		"Computes metrics for the quality of a segmentation as compared to a reference image.<br/>"
		"The currently selected (=first) image is used as the image to be judged, "
		"the additional input (=second) is used as reference image.<br/>"
		"For more information, see the <a href="
		"\"https://itk.org/Doxygen/html/classitk_1_1LabelOverlapMeasuresImageFilter.html\">"
		"Label Overlap Measures Filter</a> in the ITK documentation.", 2, 0)
{
	addOutputValue("Total Overlap");
	addOutputValue("Union Overlap (Jaccard)");
	addOutputValue("Mean Overlap (Dice)");
	addOutputValue("Volume Similarity");
}

void iASegmentationMetrics::performWork(QVariantMap const & /*parameters*/)
{
	switch (inputScalarType())
	{	// only int types, so ITK_TYPED_CALL won't work
	case iAITKIO::ScalarType::UCHAR: CalculateSegmentationMetrics<unsigned char> (this); break;
	case iAITKIO::ScalarType::CHAR:  CalculateSegmentationMetrics<char>          (this); break;
	case iAITKIO::ScalarType::SHORT: CalculateSegmentationMetrics<short>         (this); break;
	case iAITKIO::ScalarType::USHORT:CalculateSegmentationMetrics<unsigned short>(this); break;
	case iAITKIO::ScalarType::INT:   CalculateSegmentationMetrics<int>           (this); break;
	case iAITKIO::ScalarType::UINT:  CalculateSegmentationMetrics<unsigned int>  (this); break;
	case iAITKIO::ScalarType::LONG:  CalculateSegmentationMetrics<long>          (this); break;
	case iAITKIO::ScalarType::ULONG: CalculateSegmentationMetrics<unsigned long> (this); break;
	default:
		throw itk::ExceptionObject(__FILE__, __LINE__,
			"Segmentation Metrics: Only Integer image types are allowed as input!");
		break;
	}
}
