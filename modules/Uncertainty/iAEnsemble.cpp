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
	
	QVector<iAITKIO::ImagePointer> labelSums;
	QVector<iAITKIO::ImagePointer> probSums;

	typedef itk::AddImageFilter<itk::Image<double, 3> > AddImgFilterType;
	for (QSharedPointer<iASamplingResults> sampling : m_samplings)
	{
		for (QSharedPointer<iAMember> member : sampling->GetMembers())
		{
			iAITKIO::ImagePointer labelImg = member->GetLabelledImage();
			QVector<iAITKIO::ImagePointer> probImgs = member->GetProbabilityImgs(labelCount);
			if (probImgs.size() != labelCount)
			{
				DEBUG_LOG("Not enough probability images available!");
				return;
			}
			if (labelSums.empty())
			{	// initialized empty sums:
				for (int i = 0; i < labelCount; ++i)
				{	// AllocateImage automatically initializes to 0
					auto labelSumI = AllocateImage(labelImg);
					auto probsSumI = AllocateImage(probImgs[0]);
					labelSums.push_back(labelSumI);
					probSums.push_back(probsSumI);
				}
			}
			// AddImageFilter maybe overkill?
			for (int l = 0; l < labelCount; ++l)
			{
				AddImgFilterType::Pointer addImgFilter = AddImgFilterType::New();
				addImgFilter->SetInput1(dynamic_cast<itk::Image<double, 3>*>(probSums[l].GetPointer()));
				addImgFilter->SetInput2(dynamic_cast<itk::Image<double, 3>*>(probImgs[l].GetPointer()));
				addImgFilter->Update();
				probSums[l] = addImgFilter->GetOutput();
			}
			++count;
		}
	}

	// divide
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
