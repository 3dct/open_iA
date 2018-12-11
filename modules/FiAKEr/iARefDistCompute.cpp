/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iARefDistCompute.h"

#include "iAFiberCharData.h"
#include "iAFiberData.h"

#include "iACsvConfig.h"

#include "charts/iASPLOMData.h"
#include "iAConsole.h"
#include "iAStringHelper.h"
#include "iAvec3.h"

#include <vtkTable.h>
#include <vtkVariant.h>

#include <array>

namespace
{
	void getBestMatches(iAFiberData const & fiber, QMap<uint, uint> const & mapping, vtkTable* refTable,
		std::vector<std::vector<iAFiberSimilarity> > & bestMatches, double diagonalLength, double maxLength)
	{
		size_t refFiberCount = refTable->GetNumberOfRows();
		bestMatches.resize(iARefDistCompute::SimilarityMeasureCount);
		for (int d = 0; d<iARefDistCompute::SimilarityMeasureCount; ++d)
		{
			std::vector<iAFiberSimilarity> similarities;
			if (d < iARefDistCompute::OverlapMeasureStart)
			{
				similarities.resize(refFiberCount);
				for (size_t refFiberID = 0; refFiberID < refFiberCount; ++refFiberID)
				{
					iAFiberData refFiber(refTable, refFiberID, mapping);
					similarities[refFiberID].index = refFiberID;
					double curSimilarity = getSimilarity(fiber, refFiber, d, diagonalLength, maxLength);
					similarities[refFiberID].similarity = curSimilarity;
				}
			}
			else
			{ // compute overlap measures only for the best-matching fibers according to metric 2:
				auto & otherMatches = bestMatches[iARefDistCompute::BestMeasureWithoutOverlap];
				similarities.resize(otherMatches.size());
				for (size_t bestMatchID = 0; bestMatchID < otherMatches.size(); ++bestMatchID)
				{
					size_t refFiberID = otherMatches[bestMatchID].index;
					iAFiberData refFiber(refTable, refFiberID, mapping);
					similarities[bestMatchID].index = refFiberID;
					double curSimilarity = getSimilarity(fiber, refFiber, d, diagonalLength, maxLength);
					similarities[bestMatchID].similarity = curSimilarity;
				}
			}
			std::sort(similarities.begin(), similarities.end());
			std::copy(similarities.begin(), similarities.begin() + iARefDistCompute::MaxNumberOfCloseFibers, std::back_inserter(bestMatches[d]));
		}
	}
}

int iARefDistCompute::MaxNumberOfCloseFibers = 25;

iARefDistCompute::iARefDistCompute(QSharedPointer<iAFiberResultsCollection> data, int referenceID):
	m_data(data),
	m_referenceID(referenceID)
{}

QStringList iARefDistCompute::getSimilarityMeasureNames()
{
	QStringList result;
	result.push_back("dc₁");
	result.push_back("dc₂");
	result.push_back("dp₁");
	result.push_back("dp₂");
	result.push_back("dp₃");
	result.push_back("do₁");
	result.push_back("do₂");
	result.push_back("do₃");
	return result;
}

void iARefDistCompute::run()
{
	// "register" other datasets to reference:
	auto & ref = m_data->result[m_referenceID];
	auto const & mapping = *ref.mapping.data();
	double const * cxr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterX]),
		*cyr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterY]),
		*czr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterZ]);
	double a = cxr[1] - cxr[0], b = cyr[1] - cyr[0], c = czr[1] - czr[0];
	double diagLength = std::sqrt(std::pow(a, 2) + std::pow(b, 2) + std::pow(c, 2));
	double const * lengthRange = m_data->spmData->paramRange(mapping[iACsvConfig::Length]);
	double maxLength = lengthRange[1] - lengthRange[0];

	for (size_t resultID = 0; resultID <  m_data->result.size(); ++resultID)
	{
		m_progress.EmitProgress(static_cast<int>(100.0 * resultID / m_data->result.size()));
		auto & d = m_data->result[resultID];
		if (resultID == m_referenceID)
			continue;
		size_t fiberCount = d.table->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// find the best-matching fibers in reference & compute difference:
			iAFiberData fiber(d.table, fiberID, mapping);
			getBestMatches(fiber, mapping, ref.table,
				d.refDiffFiber[fiberID].dist, diagLength, maxLength);
		}
	}
	std::array<size_t, iAFiberCharData::FiberValueCount> diffCols = {
		mapping[iACsvConfig::StartX],  mapping[iACsvConfig::StartY],  mapping[iACsvConfig::StartZ],
		mapping[iACsvConfig::EndX],    mapping[iACsvConfig::EndY],    mapping[iACsvConfig::EndZ],
		mapping[iACsvConfig::CenterX], mapping[iACsvConfig::CenterY], mapping[iACsvConfig::CenterZ],
		mapping[iACsvConfig::Phi], mapping[iACsvConfig::Theta],
		mapping[iACsvConfig::Length],
		mapping[iACsvConfig::Diameter]
	};
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto& d = m_data->result[resultID];
		if (resultID == m_referenceID)
			continue;
		size_t fiberCount = d.table->GetNumberOfRows();
		d.refDiffFiber.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			size_t timeStepCount = d.timeValues.size();
			auto & diffs = d.refDiffFiber[fiberID].diff;
			diffs.resize(iAFiberCharData::FiberValueCount+SimilarityMeasureCount);
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				auto & timeStepDiffs = diffs[diffID].timestep;
				timeStepDiffs.resize(timeStepCount);
				for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
				{
					// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
					size_t refFiberID = d.refDiffFiber[fiberID].dist[BestSimilarityMeasure][0].index;
					timeStepDiffs[timeStep] = d.timeValues[timeStep][fiberID][diffID]
						- ref.table->GetValue(refFiberID, diffCols[diffID]).ToDouble();
				}
			}
			for (size_t distID = 0; distID < SimilarityMeasureCount; ++distID)
			{
				auto & timeStepDiffs = diffs[iAFiberCharData::FiberValueCount + distID].timestep;
				timeStepDiffs.resize(timeStepCount);
				size_t refFiberID = d.refDiffFiber[fiberID].dist[distID][0].index;
				iAFiberData refFiber(ref.table, refFiberID, mapping);
				for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
				{
					iAFiberData fiber(d.timeValues[timeStep][fiberID]);
					double dist = getSimilarity(fiber, refFiber, distID, diagLength, maxLength);
					timeStepDiffs[timeStep] = dist;
				}
			}
		}
	}
	size_t spmID = 0;
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		if (resultID == m_referenceID)
		{
			spmID += d.fiberCount;
			continue;
		}
		for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
		{
			auto & diffData = d.refDiffFiber[fiberID];
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				size_t tableColumnID = m_data->spmData->numParams() - (iAFiberCharData::FiberValueCount + SimilarityMeasureCount + EndColumns) + diffID;
				double lastValue = d.timeValues.size() > 0 ? diffData.diff[diffID].timestep[d.timeValues.size() - 1] : 0;
				m_data->spmData->data()[tableColumnID][spmID] = lastValue;
				d.table->SetValue(fiberID, tableColumnID, lastValue); // required for coloring 3D view by these diffs + used below for average!
			}
			for (size_t distID = 0; distID < SimilarityMeasureCount; ++distID)
			{
				double similarity = diffData.dist[distID][0].similarity;
				size_t tableColumnID = m_data->spmData->numParams() - (SimilarityMeasureCount + EndColumns) + distID;
				m_data->spmData->data()[tableColumnID][spmID] = similarity;
				d.table->SetValue(fiberID, tableColumnID, similarity); // required for coloring 3D view by these similarities + used below for average!
			}
			++spmID;
		}
	}

	std::vector<double> refDistSum(ref.fiberCount, 0.0);
	std::vector<double> refMatchCount(ref.fiberCount, 0.0);
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;
		auto & d = m_data->result[resultID];
		for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
		{
			auto & bestFiberBestDist = d.refDiffFiber[fiberID].dist[BestSimilarityMeasure][0];
			size_t refFiberID = bestFiberBestDist.index;
			refDistSum[refFiberID] += bestFiberBestDist.similarity;
			refMatchCount[refFiberID] += 1;
		}
	}
	size_t colID = m_data->result[m_referenceID].table->GetNumberOfColumns();
	addColumn(m_data->result[m_referenceID].table, 0, "AvgSimilarity", ref.fiberCount);
	m_data->avgRefFiberMatch.resize(ref.fiberCount);
	for (size_t fiberID = 0; fiberID < ref.fiberCount; ++fiberID)
	{
		double value = (refMatchCount[fiberID] == 0) ? -1 : refDistSum[fiberID] / refMatchCount[fiberID];
		m_data->avgRefFiberMatch[fiberID] = value;
		m_data->result[m_referenceID].table->SetValue(fiberID, colID, value);
		//DEBUG_LOG(QString("Fiber %1: matches=%2, similarity sum=%3, average=%4")
		//	.arg(fiberID).arg(refDistSum[fiberID]).arg(refMatchCount[fiberID]).arg(value));
	}

	// compute average differences/similarities:
	size_t diffCount = iAFiberCharData::FiberValueCount+SimilarityMeasureCount;
	m_data->maxAvgDifference.resize(diffCount, std::numeric_limits<double>::min());
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;
		auto & d = m_data->result[resultID];
		d.avgDifference.resize(diffCount, 0.0);
		for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
		{
			for (size_t diffID = 0; diffID < diffCount; ++diffID)
			{
				size_t tableColumnID = m_data->spmData->numParams() - (iAFiberCharData::FiberValueCount + SimilarityMeasureCount + EndColumns) + diffID;
				double value = std::abs(d.table->GetValue(fiberID, tableColumnID).ToDouble());
				d.avgDifference[diffID] += value;
			}
		}
		for (size_t diffID = 0; diffID < diffCount; ++diffID)
		{
			d.avgDifference[diffID] /= d.fiberCount;
			if (d.avgDifference[diffID] > m_data->maxAvgDifference[diffID])
				m_data->maxAvgDifference[diffID] = d.avgDifference[diffID];
		}
	}
}

iAProgress* iARefDistCompute::progress()
{
	return &m_progress;
}

size_t iARefDistCompute::referenceID() const
{
	return m_referenceID;
}
