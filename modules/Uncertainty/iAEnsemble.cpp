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
#include "iAMember.h"
#include "iASamplingResults.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iAPerformanceHelper.h"
#include "iAToolsITK.h"

#include <QDir>
#include <QFileInfo>

QSharedPointer<iAEnsemble> iAEnsemble::create()
{
	return QSharedPointer<iAEnsemble>(new iAEnsemble);
}

bool iAEnsemble::load(QString const & ensembleFileName, iAEnsembleDescriptorFile const & ensembleFile)
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
	createUncertaintyImages(ensembleFile.GetLabelCount(), QFileInfo(ensembleFileName).absolutePath() + "/cache");
	//QFileInfo(fileName);
	// load all ensemble members into member view
	// update spatial view to show representative of all
	// enable probability probing in chart view?
	return true;
}

typedef itk::ImageRegionIterator<DoubleImage> DoubleImageIterator;
typedef itk::ImageRegionConstIterator<DoubleImage> DoubleImageConstIter;

void iAEnsemble::createUncertaintyImages(int labelCount, QString const & cachePath)
{
	QDir qdir;
	if (!qdir.mkpath(cachePath))
	{
		DEBUG_LOG(QString("Can't create cache directory %1!").arg(cachePath));
		return;
	}
	try
	{
		// also load slice images here?
		if (m_samplings.size() == 0 || m_samplings[0]->GetMembers().size() == 0)
		{
			DEBUG_LOG("No samplings or no members found!");
			return;
		}
		itk::Index<3> idx;
		itk::Size<3> size;
		itk::Vector<double, 3> spacing;

		// also calculate neighbourhood uncertainty here?
		size_t count = 0;
		for (QSharedPointer<iASamplingResults> sampling : m_samplings)
		{
			count += sampling->GetMembers().size();
		}
		double factor = 1.0 / count;
		if (QFileInfo::exists(cachePath + "/labelDistributionEntropy.mhd"))
		{
			iAITKIO::ScalarPixelType pixelType;
			iAITKIO::ImagePointer img = iAITKIO::readFile(cachePath + "/labelDistributionEntropy.mhd", pixelType, false);
			m_labelDistrEntropy = dynamic_cast<DoubleImage*>(img.GetPointer());
			if (pixelType != itk::ImageIOBase::DOUBLE || !m_labelDistrEntropy)
			{
				DEBUG_LOG("Error loading label distribution entropy image!");
				return;
			}
			size = m_labelDistrEntropy->GetLargestPossibleRegion().GetSize();
			spacing = m_labelDistrEntropy->GetSpacing();
		}
		else
		{
			iAPerformanceHelper labelDistrMeasure;
			labelDistrMeasure.start("Label Distribution Loop");
			m_labelDistr.clear();
			for (QSharedPointer<iASamplingResults> sampling : m_samplings)
			{
				for (QSharedPointer<iAMember> member : sampling->GetMembers())
				{
					typename IntImage::Pointer labelImg = dynamic_cast<IntImage*>(member->GetLabelledImage().GetPointer());
					if (m_labelDistr.empty())
					{	// initialize empty sums:
						for (int i = 0; i < labelCount; ++i)
						{	// AllocateImage automatically initializes to 0
							auto labelSumI = CreateImage<IntImage>(labelImg);
							m_labelDistr.push_back(labelSumI);
						}
						size = labelImg->GetLargestPossibleRegion().GetSize();
					}
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
				}
			}
			labelDistrMeasure.stop();

			iAPerformanceHelper labelDistrSumEntropyLoopMeasure;
			labelDistrSumEntropyLoopMeasure.start("Label Distribution Entropy Loop");
			spacing = m_labelDistr[0]->GetSpacing();
			m_labelDistrEntropy = CreateImage<DoubleImage>(size, spacing);
			// TOOD: itk entropy filter!
			for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
			{
				for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
				{
					for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
					{
						// calculate entropy
						double entropy = 0;
						for (int l = 0; l < labelCount; ++l)
						{
							// optimize speed via iterators / direct access?
							double prob = static_cast<double>(m_labelDistr[l]->GetPixel(idx)) / count;
							if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
							{
								entropy += (prob * std::log(prob));
							}
						}
						entropy = -entropy;	// * normalizeFactor	//entropy = clamp(0.0, limit, entropy);
						m_labelDistrEntropy->SetPixel(idx, entropy);
					}
				}
			}
			labelDistrSumEntropyLoopMeasure.stop();
			iAITKIO::writeFile(cachePath + "/labelDistributionEntropy.mhd", m_labelDistrEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}

		if (QFileInfo::exists(cachePath + "/avgAlgProbSumEntropy.mhd"))
		{
			iAITKIO::ScalarPixelType pixelType;
			iAITKIO::ImagePointer img = iAITKIO::readFile(cachePath + "/avgAlgProbSumEntropy.mhd", pixelType, false);
			m_probSumEntropy = dynamic_cast<DoubleImage*>(img.GetPointer());
			if (pixelType != itk::ImageIOBase::DOUBLE || !m_probSumEntropy)
			{
				DEBUG_LOG("Error loading average algorithm entropy (from probability sums) image!");
			}
		}
		else
		{
			iAPerformanceHelper probSumLoopMeasure;
			probSumLoopMeasure.start("Probability Sum Loop");
			m_probDistr.clear();
			for (QSharedPointer<iASamplingResults> sampling : m_samplings)
			{
				for (QSharedPointer<iAMember> member : sampling->GetMembers())
				{
					QVector<iAITKIO::ImagePointer> probImgs = member->GetProbabilityImgs(labelCount);
					if (probImgs.size() != labelCount)
					{
						DEBUG_LOG("Not enough probability images available!");
						return;
					}
					bool allFresh = m_probDistr.empty();
					for (int l = 0; l < labelCount; ++l)
					{
						// create probability histogram here?
						if (allFresh)
						{
							m_probDistr.push_back(dynamic_cast<DoubleImage*>(probImgs[l].GetPointer()));
						}
						else
						{
							auto probImg = dynamic_cast<DoubleImage*>(probImgs[l].GetPointer());
							DoubleImageConstIter inImgIt(probImg, probImg->GetLargestPossibleRegion());
							DoubleImageIterator outImgIt(m_probDistr[l], m_probDistr[l]->GetLargestPossibleRegion());
							while (!inImgIt.IsAtEnd() && !outImgIt.IsAtEnd())
							{
								double inImgVal = inImgIt.Get();
								double outImgVal = outImgIt.Get();
								outImgIt.Set(inImgVal + outImgVal);
								++inImgIt;
								++outImgIt;
							}
						}
					}
				}
			}
			probSumLoopMeasure.stop();
			iAPerformanceHelper probSumEntropyLoopMeasure;
			probSumEntropyLoopMeasure.start("Prob Sum Entropy Loop");
			m_probSumEntropy = CreateImage<DoubleImage>(size, spacing);
			for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
			{
				for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
				{
					for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
					{
						// calculate entropy
						double entropy = 0;
						for (int l = 0; l < labelCount; ++l)
						{
							// optimize speed via iterators / direct access?
							double prob = m_probDistr[l]->GetPixel(idx) / count;
							if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
							{
								entropy += (prob * std::log(prob));
							}
						}
						entropy = -entropy;	// * normalizeFactor	//entropy = clamp(0.0, limit, entropy);
						m_probSumEntropy->SetPixel(idx, entropy);
					}
				}
			}
			probSumEntropyLoopMeasure.stop();
			iAITKIO::writeFile(cachePath + "/avgAlgProbSumEntropy.mhd", m_probSumEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}

		if (QFileInfo::exists(cachePath + "/avgAlgEntropyAvgEntropy.mhd"))
		{
			iAITKIO::ScalarPixelType pixelType;
			iAITKIO::ImagePointer img = iAITKIO::readFile(cachePath + "/avgAlgEntropyAvgEntropy.mhd", pixelType, false);
			m_entropyAvgEntropy = dynamic_cast<DoubleImage*>(img.GetPointer());
			if (pixelType != itk::ImageIOBase::DOUBLE || !m_entropyAvgEntropy)
			{
				DEBUG_LOG("Error loading average algorithm entropy (from algorithm entropy average) image!");
			}
		}
		else
		{
			iAPerformanceHelper entropySumLoopMeasure;
			entropySumLoopMeasure.start("Entropy Sum Loop");
			m_entropyAvgEntropy = CreateImage<DoubleImage>(size, spacing);
			for (QSharedPointer<iASamplingResults> sampling : m_samplings)
			{
				for (QSharedPointer<iAMember> member : sampling->GetMembers())
				{
					QVector<iAITKIO::ImagePointer> probImgs = member->GetProbabilityImgs(labelCount);
					typename DoubleImage::Pointer * probImgsArray = new DoubleImage::Pointer[labelCount];
					for (int l = 0; l < labelCount; ++l)
					{
						probImgsArray[l] = dynamic_cast<DoubleImage*>(probImgs[l].GetPointer());
					}
					for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
					{
						for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
						{
							for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
							{
								// calculate entropy
								double entropy = 0;
								for (int l = 0; l < labelCount; ++l)
								{
									double prob = probImgsArray[l]->GetPixel(idx);
									if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
									{
										entropy += (prob * std::log(prob));
									}
								}
								entropy = -entropy;	// * normalizeFactor
													//entropy = clamp(0.0, limit, entropy);
								m_entropyAvgEntropy->SetPixel(idx, m_entropyAvgEntropy->GetPixel(idx) + entropy);
							}
						}
					}
				}
			}
			entropySumLoopMeasure.stop();
			iAPerformanceHelper entropySumDivLoopMeasure;
			entropySumDivLoopMeasure.start("Entropy Sum Division");
			DoubleImageIterator outImgIt(m_entropyAvgEntropy, m_entropyAvgEntropy->GetLargestPossibleRegion());
			while (!outImgIt.IsAtEnd())
			{
				double outImgVal = outImgIt.Get();
				outImgIt.Set(outImgVal * factor);
				++outImgIt;
			}
			entropySumDivLoopMeasure.stop();
			iAITKIO::writeFile(cachePath + "/avgAlgEntropyAvgEntropy.mhd", m_entropyAvgEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}

		iAPerformanceHelper imgConversionMeasure;
		imgConversionMeasure.start("Image Conversion");
		iAConnector con1;
		con1.SetImage(m_labelDistrEntropy);
		m_labelDistributionUncertainty = con1.GetVTKImage();

		iAConnector con2;
		con2.SetImage(m_entropyAvgEntropy);
		m_avgAlgEntropySumUncertainty = con2.GetVTKImage();

		iAConnector con3;
		con3.SetImage(m_probSumEntropy);
		m_avgAlgProbEntropyUncertainty = con3.GetVTKImage();
		imgConversionMeasure.stop();
	}
	catch (itk::ExceptionObject & excp)
	{
		DEBUG_LOG(QString("ITK ERROR: %1").arg(excp.what()));
	}
}


vtkImagePointer iAEnsemble::GetLabelDistribution()
{
	return m_labelDistributionUncertainty;
}

vtkImagePointer iAEnsemble::GetAvgAlgEntropyFromSum()
{
	return m_avgAlgEntropySumUncertainty;
}

vtkImagePointer iAEnsemble::GetAvgAlgEntropyFromProbSum()
{
	return m_avgAlgProbEntropyUncertainty;
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
