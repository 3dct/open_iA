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
#include "iASegmentationMetrics.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iATypedCallHelper.h"

#include <itkLabelOverlapMeasuresImageFilter.h>

template <typename ImagePixelType>
void CalculateSegmentationMetrics(QVector<iAConnector*> & images, iAFilter* iafilter)
{
	typedef itk::Image<ImagePixelType, 3> ImageType;
	typedef typename ImageType::Pointer ImagePointer;
	typedef itk::LabelOverlapMeasuresImageFilter<ImageType > FilterType;
	typename FilterType::Pointer filter = FilterType::New();
	ImagePointer groundTruthPtr = dynamic_cast<ImageType*>(images[0]->GetITKImage());
	ImagePointer segmentedPtr = dynamic_cast<ImageType*>(images[1]->GetITKImage());
	if (!groundTruthPtr || !segmentedPtr)
	{
		DEBUG_LOG("Input images do not have the same type, but are required to!");
		return;
	}
	filter->SetSourceImage(groundTruthPtr);
	filter->SetTargetImage(segmentedPtr);
	filter->Update();

	iafilter->AddOutputValue("Total Overlap", filter->GetTotalOverlap());
	iafilter->AddOutputValue("Union Overlap (Jaccard)", filter->GetUnionOverlap());
	iafilter->AddOutputValue("Mean Overlap (Dice)", filter->GetMeanOverlap());
	iafilter->AddOutputValue("Volume Similarity", filter->GetVolumeSimilarity());
	//iafilter->AddOutputValue("False negatives", filter->GetFalseNegativeError());
	//iafilter->AddOutputValue("False positives", filter->GetFalsePositiveError());

	typename FilterType::MapType labelMap = filter->GetLabelSetMeasures();
	typename FilterType::MapType::const_iterator it;
	for (it = labelMap.begin(); it != labelMap.end(); ++it)
	{
		if ((*it).first == 0)
		{
			continue;
		}
		int label = (*it).first;
		iafilter->AddOutputValue(QString("Label %1 Target Overlap").arg(label), filter->GetTargetOverlap(label));
		iafilter->AddOutputValue(QString("Label %1 Union Overlap").arg(label), filter->GetUnionOverlap(label));
		iafilter->AddOutputValue(QString("Label %1 Mean Overlap").arg(label), filter->GetMeanOverlap(label));
		iafilter->AddOutputValue(QString("Label %1 Volume Similarity").arg(label), filter->GetVolumeSimilarity(label));
		//iafilter->AddOutputValue(QString("Label %1 False negatives").arg(label), filter->GetFalseNegativeError(label));
		//iafilter->AddOutputValue(QString("Label %1 False positives").arg(label), filter->GetFalsePositiveError(label));
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

void iASegmentationMetrics::PerformWork(QMap<QString, QVariant> const & parameters)
{
	switch (m_con->GetITKScalarPixelType())
	{	// only int types, so ITK_TYPED_CALL won't work
	case itk::ImageIOBase::UCHAR: CalculateSegmentationMetrics<unsigned char> (m_cons, this); break;
	case itk::ImageIOBase::CHAR:  CalculateSegmentationMetrics<char>          (m_cons, this); break;
	case itk::ImageIOBase::SHORT: CalculateSegmentationMetrics<short>         (m_cons, this); break;
	case itk::ImageIOBase::USHORT:CalculateSegmentationMetrics<unsigned short>(m_cons, this); break;
	case itk::ImageIOBase::INT:   CalculateSegmentationMetrics<int>           (m_cons, this); break;
	case itk::ImageIOBase::UINT:  CalculateSegmentationMetrics<unsigned int>  (m_cons, this); break;
	case itk::ImageIOBase::LONG:  CalculateSegmentationMetrics<long>          (m_cons, this); break;
	case itk::ImageIOBase::ULONG: CalculateSegmentationMetrics<unsigned long> (m_cons, this); break;
	default:
		throw itk::ExceptionObject(__FILE__, __LINE__,
			"Segmentation Metrics: Only Integer image types are allowed as input!");
		break;
	}
}
