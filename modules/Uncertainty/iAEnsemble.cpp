// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAEnsemble.h"

#include "iAEnsembleDescriptorFile.h"
#include "iASamplingResults.h"
#include "iASingleResult.h"

#include <iAConnector.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iAToolsITK.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkConstNeighborhoodIterator.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>

#include <cassert>


iAUncertaintyImages::~iAUncertaintyImages()
{}


std::shared_ptr<iAEnsemble> iAEnsemble::Create(int entropyBinCount,
	std::shared_ptr<iAEnsembleDescriptorFile> ensembleFile)
{
	auto result = std::shared_ptr<iAEnsemble>(new iAEnsemble(entropyBinCount));
	QMap<int, QString> const & samplings = ensembleFile->samplings();
	for (int key : samplings.keys())
	{
		if (!result->LoadSampling(samplings[key], key))
		{
			LOG(lvlError, QString("Ensemble: Could not load sampling '%1'!").arg(samplings[key]));
			return std::shared_ptr<iAEnsemble>();
		}
	}
	result->m_ensembleFile = ensembleFile;
	result->m_labelCount = ensembleFile->labelCount();
	result->m_cachePath = QFileInfo(ensembleFile->fileName()).absolutePath() + "/cache";
	result->CreateUncertaintyImages();
	if (!ensembleFile->referenceImage().isEmpty())
	{
		iAITKIO::PixelType pixelType;
		iAITKIO::ScalarType scalarType;
		auto itkImg = iAITKIO::readFile(ensembleFile->referenceImage(), pixelType, scalarType, false);
		assert(pixelType == iAITKIO::PixelType::SCALAR);
		result->m_referenceImage = dynamic_cast<IntImage*>(itkImg.GetPointer());
	}
	// load sub ensembles:
	for (int i = 0; i < ensembleFile->subEnsembleCount(); ++i)
	{
		result->AddSubEnsemble(ensembleFile->subEnsemble(i), ensembleFile->subEnsembleID(i));
	}
	return result;
}

std::shared_ptr<iAEnsemble> iAEnsemble::Create(int entropyBinCount,
	QVector<std::shared_ptr<iASingleResult> > member,
	std::shared_ptr<iASamplingResults> superSet,	int labelCount, QString const & cachePath, int id,
	IntImage::Pointer reference)
{
	auto result = std::shared_ptr<iAEnsemble>(new iAEnsemble(entropyBinCount));
	auto samplingResults = std::make_shared<iASamplingResults>(superSet->attributes(),
		"Subset", superSet->path(), superSet->executable(), superSet->additionalArguments(), superSet->name(), id);
	samplingResults->setMembers(member);
	result->m_samplings.push_back(samplingResults);
	result->m_cachePath = cachePath;
	result->m_labelCount = labelCount;
	result->m_referenceImage = reference;
	result->CreateUncertaintyImages();
	return result;
}

namespace
{
	//! Calculate an entropy image out of a given collection of images.
	//!
	//! For each voxel, the given collection is interpreted as separate probability distribution
	//! @param distribution the collection of images considered as probability distribution
	//! @param normalize whether to normalize the entropy to the range [0..1]
	//!        the maximum entropy appears when there is equal distribution among all alternatives
	//!        (i.e. if distribution has N elements, then the entropy is maximum for a voxel if all
	//!        values of the distribution for that voxel have the value 1/N)
	//! @param probFactor factor to apply to the elements of distribution before calculating the entropy
	//!        from it. This is useful if the given distribution does not contain probabilities, but
	//!        e.g. a "histogram", that is, a number of occurences of that particular value among the
	//!        set for which the entropy should be calculated. In that case, specify 1/(size of set)
	//!        as factor
	//! @return the entropy for each voxel in form of an image with pixel type double
	//!        in case that the given distribution was empty, an empty smart ptr will be returned
	//!        (equivalent of null).
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
		auto result = createImage<DoubleImage>(size, spacing);
		assert(size[0] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
		       size[1] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
		       size[2] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()));
		itk::Index<3> idx; // optimize speed via iterators / direct access?
		for (idx[0] = 0; static_cast<itk::SizeValueType>(idx[0]) < size[0]; ++idx[0])
		{
			for (idx[1] = 0; static_cast<itk::SizeValueType>(idx[1]) < size[1]; ++idx[1])
			{
				for (idx[2] = 0; static_cast<itk::SizeValueType>(idx[2]) < size[2]; ++idx[2])
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
		iAITKIO::PixelType pixelType;
		iAITKIO::ScalarType scalarType;
		iAITKIO::ImagePointer img = iAITKIO::readFile(fileName, pixelType, scalarType, false);
		assert(pixelType == iAITKIO::PixelType::SCALAR);
		imgPointer = dynamic_cast<TImage*>(img.GetPointer()); // check pixelType?
		if (!imgPointer)
		{
			LOG(lvlError, QString("Error loading %1!").arg(label));
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
		con.setImage(itkImg);
		return con.vtkImage();
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
			LOG(lvlError, QString("Couldn't open %1 for reading!").arg(fileName));
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
			LOG(lvlWarn, "Different histogram bin count than anticipated, readjusting buffer size.");
			delete[] destination;
			binCount = static_cast<int>(lines.size());
			destination = new double[binCount];
		}
		size_t cur = 0;
		bool ok;
		for (QString line : lines)
		{
			destination[cur] = line.toDouble(&ok);
			if (!ok)
			{
				LOG(lvlError, QString("Error while trying to convert %1 to number in histogram reading!").arg(line));
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
			LOG(lvlError, QString("Couldn't open %1 for output!").arg(fileName));
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
			LOG(lvlError, QString("Couldn't open %1 for output!").arg(fileName));
			return false;
		}
		QTextStream outStream(&out);
		for (size_t i = 0; i < values.size(); ++i)
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
			LOG(lvlError, QString("Couldn't open %1 for reading!").arg(fileName));
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
				LOG(lvlError, QString("Error while trying to convert %1 to number in histogram reading!").arg(line));
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
	DoubleImage::Pointer result = createImage<DoubleImage>(size, spacing);
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
		else // if on boundary
		{
			for (int l = 0; l < labelCount; ++l)
			{
				double prob = static_cast<double>(labelHistogram[l]) / valueCount;
				if (prob > 0) // to avoid infinity - we take 0, which is appropriate according to limit of 0 times infinity
				{
					entropy += (prob * std::log(prob));
				}
			}
			double localLimit = std::log(valueCount);  // max entropy: - N* (1/N * log(1/N)) = log(N)
			double localNormalizeFactor = 1.0 / localLimit;
			entropy = clamp(0.0, localLimit, -entropy * localNormalizeFactor);
		}
		outIt.Set(entropy);
		++it;
		++outIt;
	}
	delete[] labelHistogram;
	return result;
}

void iAEnsemble::CreateUncertaintyImages()
{
	QDir qdir;
	if (!qdir.mkpath(m_cachePath))
	{
		LOG(lvlError, QString("Can't create cache directory %1!").arg(m_cachePath));
		return;
	}
	if (m_labelCount <= 0)
	{
		LOG(lvlError, QString("Invalid label count: %1").arg(m_labelCount));
		return;
	}
	try
	{
		// also load slice images here?
		if (m_samplings.size() == 0 || m_samplings[0]->members().size() == 0)
		{
			LOG(lvlError, "No samplings or no members found!");
			return;
		}
		itk::Index<3> idx;
		itk::Size<3> size;               size   .Fill(0);
		itk::Vector<double, 3> spacing;  spacing.Fill(1);

		// also calculate neighbourhood uncertainty here?
		size_t count = 0;
		for (std::shared_ptr<iASamplingResults> sampling : m_samplings)
		{
			count += sampling->members().size();
		}
		double factor = 1.0 / count;

		if (LoadCachedImageSeries<IntImage>(m_labelDistr, m_cachePath+"/labelDistribution", 0, m_labelCount, "Label Distribution"))
		{
			size = m_labelDistr[0]->GetLargestPossibleRegion().GetSize();
			spacing = m_labelDistr[0]->GetSpacing();
		}
		else
		{
			m_labelDistr.clear();
			for (std::shared_ptr<iASamplingResults> sampling : m_samplings)
			{
				for (std::shared_ptr<iASingleResult> member : sampling->members())
				{
					iAITKIO::ImagePointer labelBaseImg = member->labelImage();
					auto intlabelImg = dynamic_cast<IntImage*>(labelBaseImg.GetPointer());
					if (m_labelDistr.empty())
					{	// initialize empty sums:
						for (int i = 0; i < m_labelCount; ++i)
						{	// allocateImage automatically initializes to 0
							auto labelSumI = createImage<IntImage>(intlabelImg);
							m_labelDistr.push_back(labelSumI);
						}
						size = intlabelImg->GetLargestPossibleRegion().GetSize();
						spacing = intlabelImg->GetSpacing();
					}
					assert(size[0] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
					       size[1] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
					       size[2] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()));
					for (idx[0] = 0; static_cast<itk::SizeValueType>(idx[0]) < size[0]; ++idx[0])
					{
						for (idx[1] = 0; static_cast<itk::SizeValueType>(idx[1]) < size[1]; ++idx[1])
						{
							for (idx[2] = 0; static_cast<itk::SizeValueType>(idx[2]) < size[2]; ++idx[2])
							{
								int label = intlabelImg->GetPixel(idx);
								// optimize speed via iterators / direct access?
								m_labelDistr[label]->SetPixel(idx, m_labelDistr[label]->GetPixel(idx) + 1);
							}
						}
					}
				}
			}
			for (int i = 0; i < m_labelCount; ++i)
			{
				iAITKIO::writeFile(m_cachePath + "/labelDistribution" +QString::number(i)+".mhd",
					m_labelDistr[i].GetPointer(), iAITKIO::ScalarType::INT, true);
			}
		}

		if (!LoadCachedImage<DoubleImage>(m_labelDistrEntropy, m_cachePath + "/labelDistributionEntropy.mhd", "label distribution entropy"))
		{
			m_labelDistrEntropy = CalculateEntropyImage<IntImage>(m_labelDistr, true, factor);
			iAITKIO::writeFile(m_cachePath + "/labelDistributionEntropy.mhd", m_labelDistrEntropy.GetPointer(), iAITKIO::ScalarType::DOUBLE, true);
		}

		if (!LoadCachedImage<DoubleImage>(m_entropyAvgEntropy, m_cachePath + "/avgAlgEntropyAvgEntropy.mhd", "average algorithm entropy (from algorithm entropy average)")
			|| !LoadHistogram(m_cachePath+"/algorithmEntropyHistogram.csv", m_entropyHistogram, m_entropyBinCount)
			|| !LoadValues(m_cachePath + "/algorithmEntropyMean.csv", m_memberEntropyAvg)
			|| !LoadValues(m_cachePath + "/algorithmEntropyVar.csv", m_memberEntropyVar))
		{
			m_entropyAvgEntropy = createImage<DoubleImage>(size, spacing);
			//int memberIdx = 0;
			double numberOfPixels = size[0] * size[1] * size[2];
			for (std::shared_ptr<iASamplingResults> sampling : m_samplings)
			{
				for (std::shared_ptr<iASingleResult> member : sampling->members())
				{
					QVector<DoubleImage::Pointer> probImgs = member->probabilityImgs(m_labelCount);
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
					//iAITKIO::writeFile(cachePath + "/algorithmEntropy"+QString::number(memberIdx)+".mhd", m_entropyAvgEntropy.GetPointer(), iAITKIO::ScalarType::DOUBLE, true);
					double entropyVar = diffsum / numberOfPixels;
					m_memberEntropyAvg.push_back(entropyAvg);
					m_memberEntropyVar.push_back(entropyVar);
					//++memberIdx;
					AddImageInPlace(m_entropyAvgEntropy, memberEntropy);
				}
			}
			StoreHistogram(m_cachePath + "/algorithmEntropyHistogram.csv", m_entropyHistogram, m_entropyBinCount);
			StoreValues(m_cachePath + "/algorithmEntropyMean.csv", m_memberEntropyAvg);
			StoreValues(m_cachePath + "/algorithmEntropyVar.csv", m_memberEntropyVar);
			MultiplyImageInPlace(m_entropyAvgEntropy, factor);
			iAITKIO::writeFile(m_cachePath + "/avgAlgEntropyAvgEntropy.mhd", m_entropyAvgEntropy.GetPointer(), iAITKIO::ScalarType::DOUBLE, true);
		}

		if (!LoadCachedImage<DoubleImage>(m_neighbourhoodAvgEntropy3x3, m_cachePath + "/entropyNeighbourhood3x3.mhd", "neighbourhood entropy (3x3)"))
		{
			m_neighbourhoodAvgEntropy3x3 = createImage<DoubleImage>(size, spacing);
			for (std::shared_ptr<iASamplingResults> sampling : m_samplings)
			{
				for (std::shared_ptr<iASingleResult> member : sampling->members())
				{
					auto labelImgOrig = member->labelImage();
					auto labelImg = dynamic_cast<IntImage*>(labelImgOrig.GetPointer());
					DoubleImage::Pointer neighbourEntropyImg3x3 = NeighbourhoodEntropyImage(labelImg, m_labelCount, 1, size, spacing);
					AddImageInPlace(m_neighbourhoodAvgEntropy3x3, neighbourEntropyImg3x3);
				}
			}
			MultiplyImageInPlace(m_neighbourhoodAvgEntropy3x3, factor);
			iAITKIO::writeFile(m_cachePath + "/entropyNeighbourhood3x3.mhd", m_neighbourhoodAvgEntropy3x3.GetPointer(), iAITKIO::ScalarType::DOUBLE, true);
		}

		m_entropy.resize(SourceCount);
		m_entropy[LabelDistributionEntropy] = ConvertITK2VTK<DoubleImage>(m_labelDistrEntropy);
		m_entropy[AvgAlgorithmEntropyEntrSum] = ConvertITK2VTK<DoubleImage>(m_entropyAvgEntropy);
		m_entropy[Neighbourhood3x3Entropy] = ConvertITK2VTK<DoubleImage>(m_neighbourhoodAvgEntropy3x3);
	}
	catch (itk::ExceptionObject & excp)
	{
		LOG(lvlError, QString("ITK ERROR: %1").arg(excp.what()));
	}
}

void iAEnsemble::writeFullDataFile(QString const & filename, bool writeIntensities, bool writeMemberLabels, bool writeMemberProbabilities, bool writeEnsembleUncertainties, std::map<size_t, std::shared_ptr<iADataSet>> dataSets)
{
	QFile allDataFile(filename);
	if (!allDataFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open file '%1' for writing!").arg(filename));
		return;
	}
	QTextStream out(&allDataFile);
	// ... write all features in this format:
	// <label> 1:<feature1value> 2:<feature2value> ...
	// ...
	itk::Index<3> idx;
	auto size = m_referenceImage->GetLargestPossibleRegion().GetSize();

	// create cache for member / probability images
	QVector<iAITKIO::ImagePointer> memberImageCache;
	QVector<QVector<DoubleImage::Pointer>> memberProbImageCache;
	for (auto s : m_samplings)
	{
		for (auto m : s->members())
		{
			auto itkImg = m->labelImage();
			memberImageCache.push_back(itkImg);

			auto prob = m->probabilityImgs(LabelCount());
			memberProbImageCache.push_back(prob);
		}
	}

	assert(size[0] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
	       size[1] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()) &&
	       size[2] < static_cast<itk::SizeValueType>(std::numeric_limits<itk::IndexValueType>::max()));
	// collect feature values for each pixel:
	for (idx[2] = 0; static_cast<itk::SizeValueType>(idx[2]) < size[2]; ++idx[2])
	{
		for (idx[1] = 0; static_cast<itk::SizeValueType>(idx[1]) < size[1]; ++idx[1])
		{
			for (idx[0] = 0; static_cast<itk::SizeValueType>(idx[0]) < size[0]; ++idx[0])
			{
				QString line(QString::number(m_referenceImage->GetPixel(idx))+" ");

				int curFeature = 0;

				if (writeIntensities)
				{
					for (size_t m = 0; m < dataSets.size(); ++m)
					{
						//for (size_t c = 0; c < dataSets[m]->componentCount(); ++c)
						//{
						auto img = dynamic_cast<iAImageData*>(dataSets[m].get())->vtkImage();
						line += QString::number(++curFeature) + ":" +
							QString::number(img->GetScalarComponentAsDouble(static_cast<int>(idx[0]), static_cast<int>(idx[1]), static_cast<int>(idx[2]), 0)) + " ";
						//}
					}
				}

				for (int m=0; m < memberImageCache.size(); ++m)
				{
					if (writeMemberLabels)
					{
						// all member labels
						auto memberLabelImg = dynamic_cast<IntImage*>(memberImageCache[m].GetPointer());
						line += QString::number(++curFeature) + ":" + QString::number(memberLabelImg->GetPixel(idx)) + " ";
					}

					if (writeMemberProbabilities)
					{
						// all member uncertainties:
						for (int l = 0; l < LabelCount(); ++l)
						{
							line += QString::number(++curFeature) + ":" + QString::number(memberProbImageCache[m][l]->GetPixel(idx)) + " ";
						}
					}
				}

				if (writeEnsembleUncertainties)
				{
					// all uncertainty / entropy images:
					for (int e = 0; e < SourceCount; ++e)
					{
						line += QString::number(++curFeature) + ":" +
							QString::number(m_entropy[e]->GetScalarComponentAsDouble(static_cast<int>(idx[0]), static_cast<int>(idx[1]), static_cast<int>(idx[2]), 0)) + " ";
					}
				}
				       // cut last space:
				out << line.left(line.size()-1) << Qt::endl;
			}
		}
	}
	allDataFile.close();
}

vtkImagePointer iAEnsemble::GetEntropy(int source) const
{
	return m_entropy[source];
}

vtkImagePointer iAEnsemble::GetReference() const
{
	return ConvertITK2VTK<IntImage>(m_referenceImage);
}

bool iAEnsemble::HasReference() const
{
	return m_referenceImage;
}

namespace
{
	const char* const UncertaintyNames[] = {
		"Algorithm Uncertainty",
		"Neighborhood Uncertainty",
		"Ensemble Uncertainty",
	};
}

QString iAEnsemble::GetSourceName(int sourceIdx) const
{
	return UncertaintyNames[sourceIdx];
}

bool iAEnsemble::LoadSampling(QString const & fileName, int id)
{
	if (fileName.isEmpty())
	{
		LOG(lvlError, "No filename given, not loading.");
		return false;
	}
	std::shared_ptr<iASamplingResults> samplingResults = iASamplingResults::load(fileName, id);
	if (!samplingResults)
	{
		LOG(lvlError, "Loading Sampling failed.");
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

std::shared_ptr<iASingleResult> const iAEnsemble::Member(qsizetype memberIdx) const
{
	for (qsizetype s=0; s<m_samplings.size(); ++s)
	{
		assert(m_samplings[s]->size() > 0);
		if (memberIdx < m_samplings[s]->size())
		{
			return m_samplings[s]->get(memberIdx);
		}
		memberIdx -= m_samplings[s]->size();
	}
	return std::shared_ptr<iASingleResult>();
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

std::shared_ptr<iASamplingResults> iAEnsemble::Sampling(qsizetype idx) const
{
	return m_samplings[idx];
}

QString const & iAEnsemble::CachePath() const
{
	return m_cachePath;
}

std::shared_ptr<iAEnsemble> iAEnsemble::AddSubEnsemble(QVector<int> memberIDs, int newEnsembleID)
{
	QVector<std::shared_ptr<iASingleResult> > members;
	for (int memberID : memberIDs)
	{
		members.push_back(Member(memberID));
	}
	QString cachePath = m_cachePath + QString("/sub%1").arg(newEnsembleID);
	auto newEnsemble = iAEnsemble::Create(EntropyBinCount(), members, Sampling(0), LabelCount(), cachePath, newEnsembleID, m_referenceImage);
	m_subEnsembles.push_back(newEnsemble);
	return newEnsemble;
}

QVector<std::shared_ptr<iAEnsemble> > iAEnsemble::SubEnsembles() const
{
	return m_subEnsembles;
}

int iAEnsemble::ID() const
{
	if (m_samplings.size() > 1)
	{
		LOG(lvlWarn, "Ensemble with more than one sampling -> could make problems with ensemble IDs (1:1 mapping currently from Sampling ID to Ensemble ID!)");
	}
	return m_samplings[0]->id();
}

std::shared_ptr<iAEnsembleDescriptorFile> iAEnsemble::EnsembleFile()
{
	return m_ensembleFile;
}
