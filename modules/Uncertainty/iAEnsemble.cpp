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
#include "iAEnsemble.h"

#include "iAEnsembleDescriptorFile.h"
#include "iASamplingResults.h"
#include "iAMember.h"

#include "iAConsole.h"
#include "iAToolsITK.h"

#include <itkAddImageFilter.h>
#include <itkMultiplyImageFilter.h>

#include <QFileInfo>

QSharedPointer<iAEnsemble> iAEnsemble::create()
{
	return QSharedPointer<iAEnsemble>(new iAEnsemble);
}

bool iAEnsemble::load(iAEnsembleDescriptorFile const & ensembleFile)
{
	// load sampling data:
	QMap<int, QString> const & samplings = ensembleFile.GetSamplings();
	for (int key : samplings.keys())
	{
		if (!loadSampling(samplings[key], ensembleFile.GetLabelCount(), key))
		{
			DEBUG_LOG(QString("Ensemble: Could not load sampling '%1'!").arg(samplings[key]));
			return false;
		}
	}
	createUncertaintyImages(ensembleFile.GetLabelCount());
	//QFileInfo(fileName);
	// load all ensemble members into member view
	// update spatial view to show representative of all
	// enable probability probing in chart view?
	return true;
}


typedef itk::Image<double, 3> DoubleImage;
typedef itk::Image<int, 3> IntImage;
typedef IntImage::Pointer IntImagePointer;


void iAEnsemble::createUncertaintyImages(int labelCount)
{
	// also load slice images here?
	if (m_samplings.size() == 0 || m_samplings[0]->GetMembers().size() == 0)
	{
		DEBUG_LOG("No samplings or no members found!");
		return;
	}
	m_samplings[0]->Get(0)->GetLabelledImage();
	size_t count = 0;

	// also calculate neighbourhood uncertainty here?
	m_labelDistr.clear();
	m_probDistr.clear();
	typedef itk::AddImageFilter<itk::Image<double, 3> > AddImgFilterType;
	for (QSharedPointer<iASamplingResults> sampling : m_samplings)
	{
		for (QSharedPointer<iAMember> member : sampling->GetMembers())
		{
			IntImagePointer labelImg = dynamic_cast<IntImage*>(member->GetLabelledImage().GetPointer());
			QVector<iAITKIO::ImagePointer> probImgs = member->GetProbabilityImgs(labelCount);
			if (probImgs.size() != labelCount)
			{
				DEBUG_LOG("Not enough probability images available!");
				return;
			}
			if (m_labelDistr.empty())
			{	// initialized empty sums:
				for (int i = 0; i < labelCount; ++i)
				{	// AllocateImage automatically initializes to 0
					auto labelSumI = CreateImage<IntImage>(labelImg);
					m_labelDistr.push_back(labelSumI);
				}
			}
			bool allFresh = m_probDistr.empty();
			for (int l = 0; l < labelCount; ++l)
			{
				// create probability histogram here?
				if (allFresh)
				{
					m_probDistr.push_back(probImgs[l]);
				}
				else
				{
					AddImgFilterType::Pointer addImgFilter = AddImgFilterType::New();
					addImgFilter->InPlaceOn();
					addImgFilter->SetInput(dynamic_cast<itk::Image<double, 3>*>(m_probDistr[l].GetPointer()));
					addImgFilter->SetInput2(dynamic_cast<itk::Image<double, 3>*>(probImgs[l].GetPointer()));
					addImgFilter->Update();
				}
			}
			itk::Size<3> size = labelImg->GetLargestPossibleRegion().GetSize();
			itk::Index<3> idx;
			for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
			{
				for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
				{
					for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
					{
						int label = labelImg->GetPixel(idx);
						// optimize speed via iterators / direct access?
						m_labelDistr[label]->SetPixel(idx, m_labelDistr[label]->GetPixel(idx) + 1);
					}
				}
			}
			++count;
		}
	}

	// divide by count to get distributions:
	double factor = 1.0 / count;
	typedef itk::MultiplyImageFilter<IntImage, DoubleImage, DoubleImage> IntToDoubleMultiplyImg;
	typedef itk::MultiplyImageFilter<DoubleImage, DoubleImage, DoubleImage> DoubleMultiplyImg;
	for (int l = 0; l < labelCount; ++l)
	{
		auto multiplyLabelFilter = IntToDoubleMultiplyImg::New();
		multiplyLabelFilter->SetInput(m_labelDistr[l]);
		multiplyLabelFilter->InPlaceOn();
		multiplyLabelFilter->SetConstant2(factor);
		multiplyLabelFilter->Update();

		auto multiplyProbFilter = DoubleMultiplyImg::New();
		multiplyProbFilter->SetInput(m_probDistr[l]);
		multiplyProbFilter->InPlaceOn();
		multiplyProbFilter->SetConstant2(factor);
		multiplyProbFilter->Update();
	}
}

bool iAEnsemble::loadSampling(QString const & fileName, int labelCount, int id)
{
	//m_simpleLabelInfo->SetLabelCount(labelCount);
	if (fileName.isEmpty())
	{
		DEBUG_LOG("No filename given, not loading.");
		return false;
	}
	QSharedPointer<iASamplingResults> samplingResults = iASamplingResults::Load(fileName, id);
	if (!samplingResults)
	{
		DEBUG_LOG("Loading Sampling failed.");
		return false;
	}
	m_samplings.push_back(samplingResults);
	return true;
}

iAEnsemble::iAEnsemble()
{
}
