/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iASegmentationMetrics.h"

#include <iAConnector.h>
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
	ImagePointer groundTruthPtr = dynamic_cast<ImageType*>(filter->input()[0]->itkImage());
	ImagePointer segmentedPtr = dynamic_cast<ImageType*>(filter->input()[1]->itkImage());
	if (!groundTruthPtr || !segmentedPtr)
	{
		LOG(lvlInfo, "Input images do not have the same type, but are required to!");
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
IAFILTER_CREATE(iASegmentationMetrics)

iASegmentationMetrics::iASegmentationMetrics() :
	iAFilter("Segmentation Quality", "Metrics",
		"Computes metrics for the quality of a segmentation as compared to a reference image.<br/>"
		"The currently selected (=first) image is used as the image to be judged, "
		"the additional input (=second) is used as reference image.<br/>"
		"For more information, see the <a href="
		"\"https://itk.org/Doxygen/html/classitk_1_1LabelOverlapMeasuresImageFilter.html\">"
		"Label Overlap Measures Filter</a> in the ITK documentation.", 2, 0)
{}

void iASegmentationMetrics::performWork(QMap<QString, QVariant> const & /*parameters*/)
{
	switch (inputPixelType())
	{	// only int types, so ITK_TYPED_CALL won't work
	case itk::ImageIOBase::UCHAR: CalculateSegmentationMetrics<unsigned char> (this); break;
	case itk::ImageIOBase::CHAR:  CalculateSegmentationMetrics<char>          (this); break;
	case itk::ImageIOBase::SHORT: CalculateSegmentationMetrics<short>         (this); break;
	case itk::ImageIOBase::USHORT:CalculateSegmentationMetrics<unsigned short>(this); break;
	case itk::ImageIOBase::INT:   CalculateSegmentationMetrics<int>           (this); break;
	case itk::ImageIOBase::UINT:  CalculateSegmentationMetrics<unsigned int>  (this); break;
	case itk::ImageIOBase::LONG:  CalculateSegmentationMetrics<long>          (this); break;
	case itk::ImageIOBase::ULONG: CalculateSegmentationMetrics<unsigned long> (this); break;
	default:
		throw itk::ExceptionObject(__FILE__, __LINE__,
			"Segmentation Metrics: Only Integer image types are allowed as input!");
		break;
	}
}
