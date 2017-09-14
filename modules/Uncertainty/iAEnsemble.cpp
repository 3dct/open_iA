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
#include "iAMathUtility.h"
#include "iAPerformanceHelper.h"
#include "iAToolsITK.h"

#include <itkConstNeighborhoodIterator.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>

QSharedPointer<iAEnsemble> iAEnsemble::Create(int entropyBinCount)
{
	return QSharedPointer<iAEnsemble>(new iAEnsemble(entropyBinCount));
}

bool iAEnsemble::Load(QString const & ensembleFileName, iAEnsembleDescriptorFile const & ensembleFile)
{
	iAPerformanceHelper ensembleLoad;
	ensembleLoad.start("Ensemble loading");
	QMap<int, QString> const & samplings = ensembleFile.GetSamplings();
	for (int key : samplings.keys())
	{
		if (!LoadSampling(samplings[key], ensembleFile.GetLabelCount(), key))
		{
			DEBUG_LOG(QString("Ensemble: Could not load sampling '%1'!").arg(samplings[key]));
			return false;
		}
	}
	m_labelCount = ensembleFile.GetLabelCount();
	CreateUncertaintyImages(ensembleFile.GetLabelCount(), QFileInfo(ensembleFileName).absolutePath() + "/cache");
	ensembleLoad.stop();
	return true;
}


namespace
{
	/**
	 * Calculate an entropy image out of a given collection of images.
	 *
	 * For each voxel, the given collection is interpreted as separate probability distribution
	 * @param distribution the collection of images considered as probability distribution
	 * @param normalize whether to normalize the entropy to the range [0..1]
	 *        the maximum entropy appears when there is equal distribution among all alternatives
	 *        (i.e. if distribution has N elements, then the entropy is maximum for a voxel if all
	 *        values of the distribution for that voxel have the value 1/N)
	 * @param probFactor factor to apply to the elements of distribution before calculating the entropy
	 *        from it. This is useful if the given distribution does not contain probabilities, but
	 *        e.g. a "histogram", that is, a number of occurences of that particular value among the
	 *        set for which the entropy should be calculated. In that case, specify 1/(size of set)
	 *        as factor
	 * @return the entropy for each voxel in form of an image with pixel type double
	 *        in case that the given distribution was empty, an empty smart ptr will be returned
	 *        (equivalent of null).
	 */
	template <typename TImage>
	DoubleImage::Pointer CalculateEntropyImage(QVector<typename TImage::Pointer> distribution, bool normalize = true, double probFactor = 1.0)
	{
		if (distribution.empty())
		{
			return DoubleImage::Pointer();
		}
		auto size = distribution[0]->GetLargestPossibleRegion().GetSize();
		auto spacing = distribution[0]->GetSpacing();
		double limit = std::log(distribution.size());  // max entropy: - N* (1/N * log(1/N)) = log(N)
		double normalizeFactor = normalize ? 1.0 / limit : 1.0;
		auto result = CreateImage<DoubleImage>(size, spacing);
		itk::Index<3> idx; // optimize speed via iterators / direct access?
		for (idx[0] = 0; idx[0] < size[0]; ++idx[0])
		{
			for (idx[1] = 0; idx[1] < size[1]; ++idx[1])
			{
				for (idx[2] = 0; idx[2] < size[2]; ++idx[2])
				{
					double entropy = 0;
					for (int l = 0; l < distribution.size(); ++l)
					{
						double prob = distribution[l]->GetPixel(idx) * probFactor;
						if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
						{
							entropy += (prob * std::log(prob));
						}
					}
					entropy = clamp(0.0, limit, -entropy * normalizeFactor);
					result->SetPixel(idx, entropy);
				}
			}
		}
		return result;
	}

	typedef itk::ImageRegionIterator<DoubleImage> DoubleImageIterator;
	typedef itk::ImageRegionConstIterator<DoubleImage> DoubleImageConstIter;

	void MultiplyImageInPlace(DoubleImage::Pointer img, double factor)
	{
		DoubleImageIterator outImgIt(img, img->GetLargestPossibleRegion());
		while (!outImgIt.IsAtEnd())
		{
			double outImgVal = outImgIt.Get();
			outImgIt.Set(outImgVal * factor);
			++outImgIt;
		}
	}

	void AddImageInPlace(DoubleImage::Pointer inOutimg, DoubleImage::Pointer inImg2)
	{
		DoubleImageIterator outImgIt(inOutimg, inOutimg->GetLargestPossibleRegion());
		DoubleImageConstIter inImgIt(inImg2, inImg2->GetLargestPossibleRegion());
		while (!inImgIt.IsAtEnd() && !outImgIt.IsAtEnd())
		{
			double inImgVal = inImgIt.Get();
			double outImgVal = outImgIt.Get();
			outImgIt.Set(inImgVal + outImgVal);
			++inImgIt;
			++outImgIt;
		}
		// assert(inImgIt.IsAtEnd() == outImgIt.IsAtEnd()); // images of same resolution
	}

	template <typename TImage>
	bool LoadCachedImage(typename TImage::Pointer & imgPointer, QString const & fileName, QString const & label)
	{
		if (!QFileInfo::exists(fileName))
		{
			return false;
		}
		iAITKIO::ScalarPixelType pixelType;
		iAITKIO::ImagePointer img = iAITKIO::readFile(fileName, pixelType, false);
		imgPointer = dynamic_cast<TImage*>(img.GetPointer()); // check pixelType?
		if (!imgPointer)
		{
			DEBUG_LOG(QString("Error loading %1!").arg(label));
			return false;
		}
		return true;
	}

	template <typename TImage>
	bool LoadCachedImageSeries(QVector<typename TImage::Pointer> & imgPointers, QString const & baseName, int startIdx, int count, QString const & label)
	{
		imgPointers.clear();
		imgPointers.resize(count);
		for (int i = startIdx; i < count; ++i)
		{
			if (!LoadCachedImage<TImage>(imgPointers[i], QString("%1%2.mhd").arg(baseName).arg(i), label))
			{
				return false;
			}
		}
		return true;
	}

	template <typename TImage>
	vtkSmartPointer<vtkImageData> ConvertITK2VTK(typename TImage::Pointer itkImg)
	{
		iAConnector con;
		con.SetImage(itkImg);
		return con.GetVTKImage();
	}

	bool LoadHistogram(QString const & fileName, double* & destination, int & binCount)
	{
		if (!QFileInfo::exists(fileName))
		{
			return false;
		}
		QVector<QString> lines;
		QFile in(fileName);
		if (!in.open(QIODevice::ReadOnly | QIODevice::Text) ||
			!in.isOpen())
		{
			DEBUG_LOG(QString("Couldn't open %1 for reading!").arg(fileName));
			return false;
		}
		QTextStream inStream(&in);
		while (!inStream.atEnd()) {
			QString line = inStream.readLine();
			lines.push_back(line);
		}
		in.close();
		if (lines.size() != binCount)
		{
			DEBUG_LOG("Different histogram bin count than anticipated, readjusting buffer size.");
			delete[] destination;
			binCount = lines.size();
			destination = new double[binCount];
		}
		size_t cur = 0;
		bool ok;
		for (QString line : lines)
		{
			destination[cur] = line.toDouble(&ok);
			if (!ok)
			{
				DEBUG_LOG(QString("Error while trying to convert %1 to number in histogram reading!").arg(line));
				return false;
			}
			++cur;
		}
		return true;
	}

	bool StoreHistogram(QString const & fileName, double const * histogramData, int binCount)
	{
		QFile out(fileName);
		if (!out.open(QIODevice::WriteOnly | QIODevice::Text) ||
			!out.isOpen())
		{
			DEBUG_LOG(QString("Couldn't open %1 for output!").arg(fileName));
			return false;
		}
		QTextStream outStream(&out);
		for (int i = 0; i < binCount; ++i)
		{
			outStream << QString("%1\n").arg(histogramData[i]);
		}
		out.close();
		return true;
	}

	bool StoreValues(QString const & fileName, std::vector<double> const & values)
	{
		QFile out(fileName);
		if (!out.open(QIODevice::WriteOnly | QIODevice::Text) ||
			!out.isOpen())
		{
			DEBUG_LOG(QString("Couldn't open %1 for output!").arg(fileName));
			return false;
		}
		QTextStream outStream(&out);
		for (int i = 0; i < values.size(); ++i)
		{
			outStream << QString("%1\n").arg(values[i]);
		}
		out.close();
		return true;
	}

	bool LoadValues(QString const & fileName, std::vector<double> & values)
	{
		if (!QFileInfo::exists(fileName))
		{
			return false;
		}
		QVector<QString> lines;
		QFile in(fileName);
		if (!in.open(QIODevice::ReadOnly | QIODevice::Text) ||
			!in.isOpen())
		{
			DEBUG_LOG(QString("Couldn't open %1 for reading!").arg(fileName));
			return false;
		}
		QTextStream inStream(&in);
		values.clear();
		bool ok;
		while (!inStream.atEnd()) {
			QString line = inStream.readLine();
			double val = line.toDouble(&ok);
			if (!ok)
			{
				DEBUG_LOG(QString("Error while trying to convert %1 to number in histogram reading!").arg(line));
				return false;
			}
			values.push_back(val);
		}
		in.close();
		return true;
	}
}

DoubleImage::Pointer NeighbourhoodEntropyImage(IntImage::Pointer intImage, int labelCount, size_t patchSize, itk::Size<3> size, itk::Vector<double, 3> spacing)
{
	DoubleImage::Pointer result = CreateImage<DoubleImage>(size, spacing);
	int neighbourhoodSize = std::pow(patchSize * 2 + 1, 3);
	int * labelHistogram = new int[neighbourhoodSize];
	double probFactor = 1.0 / neighbourhoodSize;
	double limit = std::log(labelCount);  // max entropy: - N* (1/N * log(1/N)) = log(N)
	double normalizeFactor = 1.0 / limit;
	bool IsInBounds;
	itk::Size<3> patch3Dsize = { patchSize, patchSize, patchSize };
	itk::ConstNeighborhoodIterator<IntImage> it(patch3Dsize, intImage, intImage->GetLargestPossibleRegion());
	itk::ImageRegionIterator<DoubleImage> outIt(result, result->GetLargestPossibleRegion());
	it.GoToBegin();
	outIt.GoToBegin();
	while (!it.IsAtEnd() && !outIt.IsAtEnd())
	{
		double sum = 0;
		std::fill(labelHistogram, labelHistogram + labelCount, 0);
		int valueCount = 0;
		for (int i = 0; i < neighbourhoodSize; ++i)
		{
			int value = it.GetPixel(i, IsInBounds);
			if (IsInBounds)
			{
				labelHistogram[value]++;
				valueCount++;
			}
		}

		double entropy = 0;
		if (valueCount == neighbourhoodSize)
		{
			for (int l = 0; l < labelCount; ++l)
			{
				double prob = labelHistogram[l] * probFactor;
				if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
				{
					entropy += (prob * std::log(prob));
				}
			}
			entropy = clamp(0.0, limit, -entropy * normalizeFactor);
		}
		else
		{
			for (int l = 0; l < labelCount; ++l)
			{
				double prob = static_cast<double>(labelHistogram[l]) / valueCount;
				if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
				{
					entropy += (prob * std::log(prob));
				}
			}
			double limit = std::log(valueCount);  // max entropy: - N* (1/N * log(1/N)) = log(N)
			double normalizeFactor = 1.0 / limit;
			entropy = clamp(0.0, limit, -entropy * normalizeFactor);
		}
		outIt.Set(entropy);
		++it;
		++outIt;
	}
	delete[] labelHistogram;
	return result;
}

void iAEnsemble::CreateUncertaintyImages(int labelCount, QString const & cachePath)
{
	QDir qdir;
	if (!qdir.mkpath(cachePath))
	{
		DEBUG_LOG(QString("Can't create cache directory %1!").arg(cachePath));
		return;
	}
	if (labelCount <= 0)
	{
		DEBUG_LOG(QString("Invalid label count: %1").arg(labelCount));
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

		if (LoadCachedImageSeries<IntImage>(m_labelDistr, cachePath+"/labelDistribution", 0, labelCount, "Label Distribution"))
		{
			size = m_labelDistr[0]->GetLargestPossibleRegion().GetSize();
			spacing = m_labelDistr[0]->GetSpacing();
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
					typename IntImage::Pointer labelImg = dynamic_cast<IntImage*>(member->LabelImage().GetPointer());
					if (m_labelDistr.empty())
					{	// initialize empty sums:
						for (int i = 0; i < labelCount; ++i)
						{	// AllocateImage automatically initializes to 0
							auto labelSumI = CreateImage<IntImage>(labelImg);
							m_labelDistr.push_back(labelSumI);
						}
						size = labelImg->GetLargestPossibleRegion().GetSize();
						spacing = labelImg->GetSpacing();
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
			for (int i = 0; i < labelCount; ++i)
			{
				iAITKIO::writeFile(cachePath + "/labelDistribution" +QString::number(i)+".mhd",
					m_labelDistr[i].GetPointer(), itk::ImageIOBase::INT, true);
			}
		}

		if (!LoadCachedImage<DoubleImage>(m_labelDistrEntropy, cachePath + "/labelDistributionEntropy.mhd", "label distribution entropy"))
		{
			iAPerformanceHelper labelDistrSumEntropyLoopMeasure;
			labelDistrSumEntropyLoopMeasure.start("Label Distribution Entropy Loop");
			m_labelDistrEntropy = CalculateEntropyImage<IntImage>(m_labelDistr, true, factor);
			labelDistrSumEntropyLoopMeasure.stop();
			iAITKIO::writeFile(cachePath + "/labelDistributionEntropy.mhd", m_labelDistrEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}

		if (!LoadCachedImage<DoubleImage>(m_probSumEntropy, cachePath + "/avgAlgProbSumEntropy.mhd", "average algorithm entropy(from probability sums)"))
		{
			iAPerformanceHelper probSumLoopMeasure;
			probSumLoopMeasure.start("Probability Sum Loop");
			m_probDistr.clear();
			for (QSharedPointer<iASamplingResults> sampling : m_samplings)
			{
				for (QSharedPointer<iAMember> member : sampling->GetMembers())
				{
					QVector<DoubleImage::Pointer> probImgs = member->GetProbabilityImgs(labelCount);
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
							m_probDistr.push_back(probImgs[l]);
						}
						else
						{
							AddImageInPlace(m_probDistr[l], probImgs[l]);
						}
					}
				}
			}
			probSumLoopMeasure.stop();
			iAPerformanceHelper probSumEntropyLoopMeasure;
			probSumEntropyLoopMeasure.start("Prob Sum Entropy Loop");
			for (int l = 0; l < m_probDistr.size(); ++l)
			{
				MultiplyImageInPlace(m_probDistr[l], factor);
			}
			m_probSumEntropy = CalculateEntropyImage<DoubleImage>(m_probDistr);
			probSumEntropyLoopMeasure.stop();
			iAITKIO::writeFile(cachePath + "/avgAlgProbSumEntropy.mhd", m_probSumEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}

		if (!LoadCachedImage<DoubleImage>(m_entropyAvgEntropy, cachePath + "/avgAlgEntropyAvgEntropy.mhd", "average algorithm entropy (from algorithm entropy average)")
			|| !LoadHistogram(cachePath+"/algorithmEntropyHistogram.csv", m_entropyHistogram, m_entropyBinCount)
			|| !LoadValues(cachePath + "/algorithmEntropyMean.csv", m_memberEntropyAvg)
			|| !LoadValues(cachePath + "/algorithmEntropyVar.csv", m_memberEntropyVar))
		{
			iAPerformanceHelper entropySumLoopMeasure;
			entropySumLoopMeasure.start("Entropy Sum Loop");
			m_entropyAvgEntropy = CreateImage<DoubleImage>(size, spacing);
			int memberIdx = 0;
			double numberOfPixels = size[0] * size[1] * size[2];
			for (QSharedPointer<iASamplingResults> sampling : m_samplings)
			{
				for (QSharedPointer<iAMember> member : sampling->GetMembers())
				{
					QVector<DoubleImage::Pointer> probImgs = member->GetProbabilityImgs(labelCount);
					auto memberEntropy = CalculateEntropyImage<DoubleImage>(probImgs);
					double sum = 0;
					itk::ImageRegionConstIterator<DoubleImage> it(memberEntropy, memberEntropy->GetLargestPossibleRegion());
					it.GoToBegin();
					while (!it.IsAtEnd())
					{
						int binIdx = clamp(0, m_entropyBinCount-1, mapValue(0.0, 1.0, 0, m_entropyBinCount, it.Get()));
						sum += it.Get();
						++m_entropyHistogram[binIdx];
						++it;
					}
					double entropyAvg = sum / numberOfPixels;
					it.GoToBegin();
					double diffsum = 0;
					while (!it.IsAtEnd())
					{
						int binIdx = clamp(0, m_entropyBinCount-1, mapValue(0.0, 1.0, 0, m_entropyBinCount, it.Get()));
						diffsum += std::pow(it.Get() - entropyAvg, 2);
						++m_entropyHistogram[binIdx];
						++it;
					}
					//iAITKIO::writeFile(cachePath + "/algorithmEntropy"+QString::number(memberIdx)+".mhd", m_entropyAvgEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
					double entropyVar = diffsum / numberOfPixels;
					m_memberEntropyAvg.push_back(entropyAvg);
					m_memberEntropyVar.push_back(entropyVar);
					++memberIdx;
					AddImageInPlace(m_entropyAvgEntropy, memberEntropy);
				}
			}
			entropySumLoopMeasure.stop();
			StoreHistogram(cachePath + "/algorithmEntropyHistogram.csv", m_entropyHistogram, m_entropyBinCount);
			StoreValues(cachePath + "/algorithmEntropyMean.csv", m_memberEntropyAvg);
			StoreValues(cachePath + "/algorithmEntropyVar.csv", m_memberEntropyVar);
			iAPerformanceHelper entropySumDivLoopMeasure;
			entropySumDivLoopMeasure.start("Entropy Sum Division");
			MultiplyImageInPlace(m_entropyAvgEntropy, factor);
			entropySumDivLoopMeasure.stop();
			iAITKIO::writeFile(cachePath + "/avgAlgEntropyAvgEntropy.mhd", m_entropyAvgEntropy.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}

		if (!LoadCachedImage<DoubleImage>(m_neighbourhoodAvgEntropy3x3, cachePath + "/entropyNeighbourhood3x3.mhd", "neighbourhood entropy (3x3)") ||
			!LoadCachedImage<DoubleImage>(m_neighbourhoodAvgEntropy5x5, cachePath + "/entropyNeighbourhood5x5.mhd", "neighbourhood entropy (5x5)"))
		{
			iAPerformanceHelper neighbourEntropyMeasure;
			neighbourEntropyMeasure.start("Neighbourhood Entropy Loop");
			m_neighbourhoodAvgEntropy3x3 = CreateImage<DoubleImage>(size, spacing);
			m_neighbourhoodAvgEntropy5x5 = CreateImage<DoubleImage>(size, spacing);
			for (QSharedPointer<iASamplingResults> sampling : m_samplings)
			{
				for (QSharedPointer<iAMember> member : sampling->GetMembers())
				{
					auto labelImgOrig = member->LabelImage();
					auto labelImg = dynamic_cast<IntImage*>(labelImgOrig.GetPointer());
					DoubleImage::Pointer neighbourEntropyImg3x3 = NeighbourhoodEntropyImage(labelImg, labelCount, 1, size, spacing);
					DoubleImage::Pointer neighbourEntropyImg5x5 = NeighbourhoodEntropyImage(labelImg, labelCount, 2, size, spacing);
					AddImageInPlace(m_neighbourhoodAvgEntropy3x3, neighbourEntropyImg3x3);
					AddImageInPlace(m_neighbourhoodAvgEntropy5x5, neighbourEntropyImg5x5);
				}
			}
			MultiplyImageInPlace(m_neighbourhoodAvgEntropy3x3, factor);
			MultiplyImageInPlace(m_neighbourhoodAvgEntropy5x5, factor);
			neighbourEntropyMeasure.stop();
			iAITKIO::writeFile(cachePath + "/entropyNeighbourhood3x3.mhd", m_neighbourhoodAvgEntropy3x3.GetPointer(), itk::ImageIOBase::DOUBLE, true);
			iAITKIO::writeFile(cachePath + "/entropyNeighbourhood5x5.mhd", m_neighbourhoodAvgEntropy5x5.GetPointer(), itk::ImageIOBase::DOUBLE, true);
		}


		m_entropy.resize(SourceCount);

		iAPerformanceHelper imgConversionMeasure;
		imgConversionMeasure.start("Image Conversion");
		m_entropy[LabelDistributionEntropy] = ConvertITK2VTK<DoubleImage>(m_labelDistrEntropy);
		m_entropy[AvgAlgorithmEntropyEntrSum] = ConvertITK2VTK<DoubleImage>(m_entropyAvgEntropy);
		m_entropy[AvgAlgorithmEntropyProbSum] = ConvertITK2VTK<DoubleImage>(m_probSumEntropy);
		m_entropy[Neighbourhood3x3Entropy] = ConvertITK2VTK<DoubleImage>(m_neighbourhoodAvgEntropy3x3);
		m_entropy[Neighbourhood5x5Entropy] = ConvertITK2VTK<DoubleImage>(m_neighbourhoodAvgEntropy5x5);

		imgConversionMeasure.stop();
	}
	catch (itk::ExceptionObject & excp)
	{
		DEBUG_LOG(QString("ITK ERROR: %1").arg(excp.what()));
	}
}


vtkImagePointer iAEnsemble::GetEntropy(int source) const
{
	return m_entropy[source];
}


const char* const UncertaintyNames[] = {
	"Label Distribution",
	"Algorithm (Entropy Sum Avg.)",
	"Algorithm (Prob. Sum Entropy)",
	"3x3 Neighbourhood",
	"5x5 Neighbourhood",
};


QString iAEnsemble::GetSourceName(int sourceIdx) const
{
	return UncertaintyNames[sourceIdx];
}


bool iAEnsemble::LoadSampling(QString const & fileName, int labelCount, int id)
{
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

iAEnsemble::iAEnsemble(int entropyBinCount) :
	m_labelCount(-1),
	m_entropyBinCount(entropyBinCount)
{
	m_entropyHistogram = new double[entropyBinCount];
	std::fill(m_entropyHistogram, m_entropyHistogram + entropyBinCount, 0);
}


iAEnsemble::~iAEnsemble()
{
	delete[] m_entropyHistogram;
}


QVector<IntImage::Pointer> const & iAEnsemble::GetLabelDistribution() const
{
	return m_labelDistr;
}


int iAEnsemble::LabelCount() const
{
	return m_labelCount;
}


double * iAEnsemble::EntropyHistogram() const
{
	return m_entropyHistogram;
}


int iAEnsemble::EntropyBinCount() const
{
	return m_entropyBinCount;
}

size_t iAEnsemble::MemberCount() const
{
	return m_memberEntropyAvg.size();
}

QSharedPointer<iAMember> const iAEnsemble::Member(size_t memberIdx) const
{
	int s = 0;
	for (int s=0; s<m_samplings.size(); ++s)
	{
		if (memberIdx < m_samplings[s]->size())
		{
			return m_samplings[s]->Get(memberIdx);
		}
		memberIdx -= m_samplings[s]->size();
	}
	return QSharedPointer<iAMember>();
}

std::vector<double> const & iAEnsemble::MemberAttribute(size_t idx) const
{
	switch (idx)
	{
	default:
	case UncertaintyMean: return m_memberEntropyAvg;
	case UncertaintyVar: return m_memberEntropyVar;
	}
}
