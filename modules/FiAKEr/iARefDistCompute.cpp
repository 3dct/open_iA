/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iARefDistCompute.h"

#include "iAFiberCharData.h"
#include "iAFiberData.h"

#include "iACsvConfig.h"

// Core:
#include <charts/iASPLOMData.h>
#include <iAConsole.h>
#include <iAStringHelper.h>
#include <iAvec3.h>

#include <vtkTable.h>
#include <vtkVariant.h>

#include <QDataStream>
#include <QDir>

#include <array>
#include <cassert>

// OpenMP
#ifndef __APPLE__
#ifndef __MACOSX
#include <omp.h>
#endif
#endif

namespace
{
	const int CacheFileVersion2NumberOfMeasures = 20;
	QString ResultCacheFileIdentifier("FIAKERResultCacheFile");
	QString AverageCacheFileIdentifier("FIAKERAverageCacheFile");
	QDataStream::Version CacheFileQtDataStreamVersion(QDataStream::Qt_5_6);
	//QString CacheFileClosestFibers("ClosestFibers");
	//QString CacheFileResultPattern("Result%1");
	quint32 AverageCacheFileVersion(1);
	quint32 ResultRefCacheFileVersion(3);

	bool verifyOpenCacheFile(QFile & cacheFile)
	{
		if (!cacheFile.exists())
		{
			return false;
		}
		if (!cacheFile.open(QFile::ReadOnly))
		{
			DEBUG_LOG(QString("Couldn't open file %1 for reading!").arg(cacheFile.fileName()));
			return false;
		}
		return true;
	}
}

iARefDistCompute::ContainerSizeType iARefDistCompute::MaxNumberOfCloseFibers = 25;

iARefDistCompute::iARefDistCompute(QSharedPointer<iAFiberResultsCollection> data, size_t referenceID) :
	m_data(data),
	m_referenceID(referenceID),
	m_columnsBefore(data->spmData->numParams()),
	m_optimizationMeasureIdx(0),
	m_bestMeasure(0)
{
}

bool iARefDistCompute::setMeasuresToCompute(std::vector<std::pair<int, bool>> const& measuresToCompute,
	int optimizationMeasure, int bestMeasure)
{
	m_measuresToCompute = measuresToCompute;
	std::sort(m_measuresToCompute.begin(), m_measuresToCompute.end(),
		[](std::pair<int, bool> const& a, std::pair<int, bool> const& b)
		{
			return !a.second && b.second;
		});
	//QVector<size_t> allMeasures(m_data->m_measures);
	for (auto m : m_measuresToCompute)
	{
		m_data->m_measures.push_back(m.first);
	}
	auto optMeasIt = std::find(m_data->m_measures.begin(), m_data->m_measures.end(), optimizationMeasure);
	auto optMeasItNew = std::find(m_measuresToCompute.begin(), m_measuresToCompute.end(), std::make_pair(optimizationMeasure, true));
	if (optMeasIt == m_data->m_measures.end() || optMeasItNew != m_measuresToCompute.end())
	{
		DEBUG_LOG("Measure to use for optimization not found, or uses optimization itself!");
		return false;
	}
	else
	{
		m_optimizationMeasureIdx = optMeasIt - m_data->m_measures.begin();
	}
	auto it = std::find(m_data->m_measures.begin(), m_data->m_measures.end(), bestMeasure);
	if (it == m_data->m_measures.end())
	{
		DEBUG_LOG("Selected best measure not found!");
		return false;
	}
	else
	{
		m_bestMeasure = it - m_data->m_measures.begin();
	}
	return true;
}

void getBestMatches(iAFiberData const& fiber,
	QMap<uint, uint> const& mapping,
	vtkTable* refTable,
	QVector<QVector<iAFiberSimilarity> >& bestMatches,
	std::map<size_t, std::vector<iAVec3f> > const& refCurveInfo,
	double diagonalLength, double maxLength,
	std::vector<std::pair<int, bool>>& measuresToCompute, int optimizationMeasureIdx)
{
	iARefDistCompute::ContainerSizeType refFiberCount = refTable->GetNumberOfRows();
	int bestMatchesStartIdx = bestMatches.size();
	assert(measuresToCompute.size() < std::numeric_limits<int>::max());
	assert(bestMatchesStartIdx + measuresToCompute.size() < std::numeric_limits<int>::max());
	int numOfNewMeasures = static_cast<int>(measuresToCompute.size());
	bestMatches.resize(bestMatchesStartIdx + numOfNewMeasures);
	auto maxNumberOfCloseFibers = std::min(iARefDistCompute::MaxNumberOfCloseFibers, refFiberCount);
	for (int d = 0; d < numOfNewMeasures; ++d)
	{
		QVector<iAFiberSimilarity> similarities;
		bool optimize = measuresToCompute[d].second;
		if (optimize && (optimizationMeasureIdx < 0 || optimizationMeasureIdx >= d))
		{
			DEBUG_LOG(QString("Invalid optimization measure base: Index %1 is outside of valid range 0 .. %2; disabling optimization!")
				.arg(optimizationMeasureIdx)
				.arg(d - 1));
			optimize = false;
		}
		if (!optimize)
		{
			similarities.resize(refFiberCount);
			for (iARefDistCompute::ContainerSizeType refFiberID = 0; refFiberID < refFiberCount; ++refFiberID)
			{
				auto it = refCurveInfo.find(refFiberID);
				iAFiberData refFiber(refTable, refFiberID, mapping, (it != refCurveInfo.end()) ? it->second : std::vector<iAVec3f>());
				similarities[refFiberID].index = refFiberID;
				double curDissimilarity = getDissimilarity(fiber, refFiber, measuresToCompute[d].first, diagonalLength, maxLength);
				if (std::isnan(curDissimilarity))
				{
					curDissimilarity = 0;
				}
				similarities[refFiberID].dissimilarity = curDissimilarity;
			}
		}
		else
		{	// compute overlap measures only for the best-matching fibers according to a simpler metric:
			auto& otherMatches = bestMatches[bestMatchesStartIdx+optimizationMeasureIdx];
			similarities.resize(otherMatches.size());
			for (iARefDistCompute::ContainerSizeType bestMatchID = 0; bestMatchID < otherMatches.size(); ++bestMatchID)
			{
				size_t refFiberID = otherMatches[bestMatchID].index;
				auto it = refCurveInfo.find(refFiberID);
				iAFiberData refFiber(refTable, refFiberID, mapping, (it != refCurveInfo.end()) ? it->second : std::vector<iAVec3f>());
				similarities[bestMatchID].index = refFiberID;
				double curDissimilarity = getDissimilarity(fiber, refFiber, measuresToCompute[d].first, diagonalLength, maxLength);
				if (std::isnan(curDissimilarity))
				{
					curDissimilarity = 0;
				}
				similarities[bestMatchID].dissimilarity = curDissimilarity;
			}
		}
		std::sort(similarities.begin(), similarities.end());
		std::copy(similarities.begin(), similarities.begin() + maxNumberOfCloseFibers, std::back_inserter(bestMatches[bestMatchesStartIdx+d]));
	}
}

void iARefDistCompute::run()
{
	QString cachePath(m_data->folder + "/cache/");
	QDir().mkdir(cachePath);
	QString referenceName(QFileInfo(m_data->result[m_referenceID].fileName).completeBaseName());
	m_progress.setStatus("Computing the distance of fibers in all results to the fibers in reference and find best matching ones, "
		"and the difference between consecutive steps.");
	auto & ref = m_data->result[m_referenceID];

	auto const & mapping = *ref.mapping.data();
	/*
	std::array<size_t, iAFiberCharData::FiberValueCount> diffCols = {
		mapping[iACsvConfig::StartX],  mapping[iACsvConfig::StartY],  mapping[iACsvConfig::StartZ],
		mapping[iACsvConfig::EndX],    mapping[iACsvConfig::EndY],    mapping[iACsvConfig::EndZ],
		mapping[iACsvConfig::CenterX], mapping[iACsvConfig::CenterY], mapping[iACsvConfig::CenterZ],
		mapping[iACsvConfig::Phi], mapping[iACsvConfig::Theta],
		mapping[iACsvConfig::Length],
		mapping[iACsvConfig::Diameter]
	};
	*/
	// get values for normalization:
	double const * cxr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterX]),
		*cyr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterY]),
		*czr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterZ]);
	double a = cxr[1] - cxr[0], b = cyr[1] - cyr[0], c = czr[1] - czr[0];
	m_diagonalLength = std::sqrt(std::pow(a, 2) + std::pow(b, 2) + std::pow(c, 2));
	double const * lengthRange = m_data->spmData->paramRange(mapping[iACsvConfig::Length]);
	m_maxLength = lengthRange[1] - lengthRange[0];
	bool recomputeAverages = false;
	std::vector<bool> writeResultCache(m_data->result.size(), false);
	bool first = true;
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		QString resultName(QFileInfo(m_data->result[resultID].fileName).completeBaseName());
		QString resultCacheFileName(cachePath + QString("refDist_%1_%2.cache").arg(referenceName).arg(resultName));
		QFile cacheFile(resultCacheFileName);
		bool readCacheResult = readResultRefComparison(cacheFile, resultID, first);
		bool skip = (resultID == m_referenceID) || readCacheResult;
		auto& d = m_data->result[resultID];
		if (resultID != m_referenceID && readCacheResult && d.avgDifference.size() == 0)
		{
			DEBUG_LOG(QString("FIAKER cache file %1: The average differences have size 0, probably due to a previous bug in FIAKER."
				" Triggering re-computation to fix this; to fix this permanently, please delete the 'cache' subfolder!")
				.arg(resultCacheFileName)
			);
			recomputeAverages = true;
		}
		m_progress.emitProgress(static_cast<int>(100.0 * resultID / m_data->result.size()));
		if (skip)
		{
			continue;
		}
		if (m_measuresToCompute.size() == 0)
		{
			DEBUG_LOG(QString("Tried to set reference without specifying which measure to use. "
				"This might happen if you use an old project file. In this case, please select the reference again"))
			return;
		}
		writeResultCache[resultID] = true;
		recomputeAverages = true; // if any result is not loaded from cache, we have to recompute averages
		qint64 const fiberCount = d.table->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
#pragma omp parallel for
		for (qint64 fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			auto it = d.curveInfo.find(fiberID);
			// find the best-matching fibers in reference & compute difference:
			iAFiberData fiber(d.table, fiberID, mapping, (it != d.curveInfo.end())? it->second : std::vector<iAVec3f>());
			getBestMatches(fiber, mapping, ref.table, d.refDiffFiber[fiberID].dist, ref.curveInfo,
				m_diagonalLength, m_maxLength, m_measuresToCompute, m_optimizationMeasureIdx);
		}
/*
		// Computing the difference between consecutive steps.
#pragma omp parallel for
		for (qint64 fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			if (d.stepData == iAFiberCharData::SimpleStepData)
			{
				iARefDistCompute::ContainerSizeType stepCount = static_cast<int>(d.stepValues.size());
				auto & diffs = d.refDiffFiber[fiberID].diff;
				diffs.resize(diffs.size + );
				for (iARefDistCompute::ContainerSizeType diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
				{
					auto & stepDiffs = diffs[diffID].step;
					stepDiffs.resize(stepCount);
					for (iARefDistCompute::ContainerSizeType step = 0; step < stepCount; ++step)
					{
						// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
						size_t refFiberID = d.refDiffFiber[fiberID].dist[BestSimilarityMeasure][0].index;
						stepDiffs[step] = d.stepValues[step][fiberID][diffID]
							- ref.table->GetValue(refFiberID, diffCols[diffID]).ToDouble();
					}
				}
				for (iARefDistCompute::ContainerSizeType distID = 0; distID < SimilarityMeasureCount; ++distID)
				{
					auto & stepDiffs = diffs[iAFiberCharData::FiberValueCount + distID].step;
					stepDiffs.resize(stepCount);
					size_t refFiberID = d.refDiffFiber[fiberID].dist[distID][0].index;

					iAFiberData refFiber(ref.table, refFiberID, mapping, std::vector<iAVec3f>());
					for (iARefDistCompute::ContainerSizeType step = 0; step < stepCount; ++step)
					{
						iAFiberData fiber(d.stepValues[step][fiberID]);
						double dist = getDissimilarity(fiber, refFiber, distID, diagLength, maxLength);
						stepDiffs[step] = dist;
					}
				}
			}
			else if (data.stepData == iAFiberCharData::CurvedStepData)
			{
				// difference computation for curved step data ...
			}
		}
		*/
	}
	m_progress.setStatus("Updating tables with data computed so far.");
	size_t spmID = 0;
	auto measureNames = getAvailableDissimilarityMeasureNames();
	for (size_t m = 0; m < m_measuresToCompute.size(); ++m)
	{
		m_data->spmData->addParameter(measureNames[m_measuresToCompute[m].first]);
	}
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		for (size_t m = 0; m < m_measuresToCompute.size(); ++m)
		{
			addColumn(d.table, 0, measureNames[m_measuresToCompute[m].first].toStdString().c_str(), d.fiberCount);
		}
		/*
		if (resultID == m_referenceID)
		{
			spmID += d.fiberCount;
			continue;
		}
		*/
		assert(d.fiberCount < std::numeric_limits<iARefDistCompute::ContainerSizeType>::max());
		for (iARefDistCompute::ContainerSizeType fiberID = 0; fiberID < static_cast<iARefDistCompute::ContainerSizeType>(d.fiberCount); ++fiberID)
		{
			//if (d.stepData == iAFiberCharData::SimpleStepData) ???
			/*
			for (iARefDistCompute::ContainerSizeType diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				size_t tableColumnID = m_data->spmData->numParams() -
					(iAFiberCharData::FiberValueCount + SimilarityMeasureCount + EndColumns) + diffID;
				double lastValue = (d.stepData == iAFiberCharData::SimpleStepData) ?
						diffData.diff[diffID].step[static_cast<int>(d.stepValues.size() - 1)] : 0;
				if (std::isnan(lastValue))
				{
					lastValue = 0;
				}
				m_data->spmData->data()[tableColumnID][spmID] = lastValue;
				d.table->SetValue(fiberID, tableColumnID, lastValue); // required for coloring 3D view by these diffs + used below for average!
			}
			*/
			assert(m_measuresToCompute.size() < std::numeric_limits<iARefDistCompute::ContainerSizeType>::max());
			for (iARefDistCompute::ContainerSizeType measureID = 0; measureID < static_cast<iARefDistCompute::ContainerSizeType>(m_measuresToCompute.size()); ++measureID)
			{
				double dissimilarity = (resultID == m_referenceID) ? 0 : d.refDiffFiber[fiberID].dist[measureID][0].dissimilarity;
				if (std::isnan(dissimilarity))
				{   // TODO: Find out under which circumstances this happens
					dissimilarity = 0;
				}
				size_t tableColumnID = m_columnsBefore + measureID;
				m_data->spmData->data()[tableColumnID][spmID] = dissimilarity;
				d.table->SetValue(fiberID, tableColumnID, dissimilarity); // required for coloring 3D view by these similarities + used below for average!
			}
			++spmID;
		}
	}

	// Computing reference differences:
	QString avgCacheFileName(cachePath + QString("refAvg_%1.cache").arg(referenceName));
	QFile avgCacheFile(avgCacheFileName);
	if (recomputeAverages // if any of the results was not loaded from cache
		|| !readAverageMeasures(avgCacheFile))  // or cache file not found / number of results cached previously is not the same as currently loaded
	{
		m_progress.setStatus("Summing up match quality (+ whether there is a match) for all reference fibers.");
		std::vector<double> refDistSum(ref.fiberCount, 0.0);
		std::vector<double> refMatchCount(ref.fiberCount, 0.0);
		for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		{
			if (resultID == m_referenceID)
			{
				continue;
			}
			auto & d = m_data->result[resultID];
			assert(d.fiberCount < std::numeric_limits<iARefDistCompute::ContainerSizeType>::max());
			for (iARefDistCompute::ContainerSizeType fiberID = 0; fiberID < static_cast<int>(d.fiberCount); ++fiberID)
			{
				auto & bestFiberBestDist = d.refDiffFiber[fiberID].dist[m_bestMeasure][0];
				size_t refFiberID = bestFiberBestDist.index;
				refDistSum[refFiberID] += bestFiberBestDist.dissimilarity;
				refMatchCount[refFiberID] += 1;
			}
		}
		//size_t colID = m_data->result[m_referenceID].table->GetNumberOfColumns();
		addColumn(m_data->result[m_referenceID].table, 0, "AvgSimilarity", ref.fiberCount);
		m_data->avgRefFiberMatch.resize(static_cast<int>(ref.fiberCount));
		for (size_t fiberID = 0; fiberID < ref.fiberCount; ++fiberID)
		{
			double value = (refMatchCount[fiberID] == 0) ? -1 : refDistSum[fiberID] / refMatchCount[fiberID];
			m_data->avgRefFiberMatch[static_cast<int>(fiberID)] = value;
		}
		m_progress.setStatus("Computing average differences/similarities for each result.");
		iARefDistCompute::ContainerSizeType diffCount = static_cast<int>(m_measuresToCompute.size());
		// std::vector resize has an additional optional argument for default value for new entries,
		// in QVector, the same can be achieved via fill method (but argument order is reversed!)
		//m_data->maxAvgDifference.resize(diffCount, std::numeric_limits<double>::min());
		m_data->maxAvgDifference.fill(std::numeric_limits<double>::min(), diffCount);
		for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		{
			if (resultID == m_referenceID)
			{
				continue;
			}
			auto & d = m_data->result[resultID];
			//d.avgDifference.resize(diffCount, 0.0);
			d.avgDifference.fill(0.0, diffCount);
			for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
			{
				for (iARefDistCompute::ContainerSizeType diffID = 0; diffID < diffCount; ++diffID)
				{
					size_t tableColumnID = m_columnsBefore + diffID;
					double value = std::abs(d.table->GetValue(fiberID, tableColumnID).ToDouble());
					d.avgDifference[diffID] += value;
				}
			}
			for (iARefDistCompute::ContainerSizeType diffID = 0; diffID < diffCount; ++diffID)
			{
				d.avgDifference[diffID] /= d.fiberCount;
				if (d.avgDifference[diffID] > m_data->maxAvgDifference[diffID])
				{
					m_data->maxAvgDifference[diffID] = d.avgDifference[diffID];
				}
			}
		}
		writeAverageMeasures(avgCacheFile);
	}
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		if (!writeResultCache[resultID])
		{
			continue;
		}
		QString resultName(QFileInfo(m_data->result[resultID].fileName).completeBaseName());
		QString resultCacheFileName(cachePath + QString("refDist_%1_%2.cache").arg(referenceName).arg(resultName));
		QFile cacheFile(resultCacheFileName);
		writeResultRefComparison(cacheFile, resultID);
	}

	size_t colID = m_data->result[m_referenceID].table->GetNumberOfColumns() -1 ;
	for (int fiberID = 0; fiberID < static_cast<int>(ref.fiberCount); ++fiberID)
	{
		m_data->result[m_referenceID].table->SetValue(fiberID, colID, m_data->avgRefFiberMatch[fiberID]);
		//DEBUG_LOG(QString("Fiber %1: matches=%2, similarity sum=%3, average=%4")
		//	.arg(fiberID).arg(refDistSum[fiberID]).arg(refMatchCount[fiberID]).arg(value));
	}
}

bool iARefDistCompute::readResultRefComparison(QFile& cacheFile, size_t resultID, bool& first)
{
	if (!verifyOpenCacheFile(cacheFile))
	{
		return false;
	}
	DEBUG_LOG(QString("Reading FIAKER cache file '%1'...").arg(cacheFile.fileName()));
	QDataStream in(&cacheFile);
	in.setVersion(CacheFileQtDataStreamVersion);
	QString identifier;
	in >> identifier;
	if (identifier != ResultCacheFileIdentifier)
	{
		DEBUG_LOG(QString("FIAKER cache file '%1': Unknown cache file format - found identifier %2 does not match expected identifier %3.")
			.arg(cacheFile.fileName())
			.arg(identifier).arg(ResultCacheFileIdentifier));
		return false;
	}
	quint32 version;
	in >> version;
	if (version == 1)
	{
		DEBUG_LOG(QString("FIAKER cache file '%1': Found file of version 1 which is known to have wrong dc1 and do3 computations. "
			"If you want to recompute with correct values, please delete the 'cache' subfolder!")
			.arg(cacheFile.fileName()));
	}
	if (version > ResultRefCacheFileVersion)
	{
		DEBUG_LOG(QString("FIAKER cache file '%1': Invalid or too high version number (%2), expected %3 or less.")
			.arg(cacheFile.fileName())
			.arg(version).arg(ResultRefCacheFileVersion));
		return false;
	}
	if (first)
	{
		QVector<qulonglong> cachedMeasures;
		if (version <= 2)
		{
			for (int m = 0; m < CacheFileVersion2NumberOfMeasures; ++m)
			{
				cachedMeasures.push_back(m);
			}
		}
		else
		{
			in >> cachedMeasures;
		}
		for (auto m : cachedMeasures)
		{
			m_data->m_measures.push_back(m);
			m_measuresToCompute.push_back(std::make_pair(m, false)); //
		}
		first = false;
	}
	else if (version > 2)
	{
		QVector<qulonglong> cachedMeasures;
		in >> cachedMeasures;
	}
	auto & d = m_data->result[resultID];
	in >> d.refDiffFiber;
	in >> d.avgDifference;
	return true;
}

void iARefDistCompute::writeResultRefComparison(QFile& cacheFile, size_t resultID)
{
	DEBUG_LOG(QString("Writing FIAKER cache file '%1'...").arg(cacheFile.fileName()));
	if (!cacheFile.open(QFile::WriteOnly))
	{
		DEBUG_LOG(QString("Couldn't open file %1 for writing!").arg(cacheFile.fileName()));
		return;
	}
	QDataStream out(&cacheFile);
	out.setVersion(CacheFileQtDataStreamVersion);
	// write header:
	out << ResultCacheFileIdentifier;
	out << ResultRefCacheFileVersion;
	// write data:
	out << m_data->m_measures;
	out << m_data->result[resultID].refDiffFiber;
	out << m_data->result[resultID].avgDifference;
	cacheFile.close();
}

void iARefDistCompute::writeAverageMeasures(QFile& cacheFile)
{
	DEBUG_LOG(QString("Writing FIAKER cache file '%1'...").arg(cacheFile.fileName()));
	if (!cacheFile.open(QFile::WriteOnly))
	{
		DEBUG_LOG(QString("Couldn't open file %1 for writing!").arg(cacheFile.fileName()));
		return;
	}
	QDataStream out(&cacheFile);
	out.setVersion(CacheFileQtDataStreamVersion);
	// write header:
	out << AverageCacheFileIdentifier;
	out << AverageCacheFileVersion;
	// write data:
	out << static_cast<quint32>(m_data->result.size());
	out << m_data->avgRefFiberMatch;
	out << m_data->maxAvgDifference;
	cacheFile.close();
}

bool iARefDistCompute::readAverageMeasures(QFile& cacheFile)
{
	if (!verifyOpenCacheFile(cacheFile))
	{
		return false;
	}
	DEBUG_LOG(QString("Reading FIAKER cache file '%1'...").arg(cacheFile.fileName()));
	QDataStream in(&cacheFile);
	in.setVersion(CacheFileQtDataStreamVersion);
	QString identifier;
	in >> identifier;
	if (identifier != AverageCacheFileIdentifier)
	{
		DEBUG_LOG(QString("FIAKER cache file '%1': Unknown cache file format - found identifier %2 does not match expected identifier %3.")
			.arg(cacheFile.fileName()).arg(identifier).arg(AverageCacheFileIdentifier));
		return false;
	}
	quint32 version;
	in >> version;
	if (version > AverageCacheFileVersion)
	{
		DEBUG_LOG(QString("FIAKER cache file '%1': Invalid or too high version number (%2), expected %3 or less.")
			.arg(cacheFile.fileName()).arg(version).arg(AverageCacheFileVersion));
		return false;
	}
	quint32 numberOfResults;
	in >> numberOfResults;
	if (numberOfResults != m_data->result.size())
	{
		DEBUG_LOG(QString("FIAKER cache file '%1': Number of results stored there (%2) is not the same as currently loaded (%3)! Recomputing all averages")
			.arg(cacheFile.fileName()).arg(numberOfResults).arg(m_data->result.size()));
		return false;
	}
	in >> m_data->avgRefFiberMatch;
	in >> m_data->maxAvgDifference;
	return true;
}

iAProgress* iARefDistCompute::progress()
{
	return &m_progress;
}

size_t iARefDistCompute::referenceID() const
{
	return m_referenceID;
}

size_t iARefDistCompute::columnsBefore() const
{
	return m_columnsBefore;
}

size_t iARefDistCompute::columnsAdded() const
{
	return m_measuresToCompute.size();
}
