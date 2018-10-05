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
		std::vector<std::vector<iAFiberDistance> > & bestMatches, double diagonalLength, double maxLength)
	{
		size_t refFiberCount = refTable->GetNumberOfRows();
		bestMatches.resize(iARefDistCompute::DistanceMetricCount);
		for (int d = 0; d<iARefDistCompute::DistanceMetricCount; ++d)
		{
			std::vector<iAFiberDistance> distances;
			if (d < 3)
			{
				distances.resize(refFiberCount);
				for (size_t refFiberID = 0; refFiberID < refFiberCount; ++refFiberID)
				{
					iAFiberData refFiber(refTable, refFiberID, mapping);
					distances[refFiberID].index = refFiberID;
					double curDistance = getDistance(fiber, refFiber, d, diagonalLength, maxLength);
					distances[refFiberID].distance = curDistance;
				}
			}
			else
			{ // compute overlap measures only for the best-matching fibers according to metric 2:
				distances.resize(bestMatches[1].size());
				for (size_t bestMatchID = 0; bestMatchID < bestMatches[1].size(); ++bestMatchID)
				{
					size_t refFiberID = bestMatches[1][bestMatchID].index;
					iAFiberData refFiber(refTable, refFiberID, mapping);
					distances[bestMatchID].index = refFiberID;
					double curDistance = getDistance(fiber, refFiber, d, diagonalLength, maxLength);
					distances[bestMatchID].distance = curDistance;
				}
			}
			std::sort(distances.begin(), distances.end());
			std::copy(distances.begin(), distances.begin() + iARefDistCompute::MaxNumberOfCloseFibers, std::back_inserter(bestMatches[d]));
		}
	}
}

int iARefDistCompute::MaxNumberOfCloseFibers = 25;

iARefDistCompute::iARefDistCompute(QSharedPointer<iAFiberResultsCollection> data, int referenceID):
	m_data(data),
	m_referenceID(referenceID)
{}

void iARefDistCompute::run()
{
	// "register" other datasets to reference:
	auto & ref = m_data->result[m_referenceID];
	auto const & mapping = *ref.mapping.data();
	double const * cxr = m_data->splomData->paramRange(mapping[iACsvConfig::CenterX]),
		*cyr = m_data->splomData->paramRange(mapping[iACsvConfig::CenterY]),
		*czr = m_data->splomData->paramRange(mapping[iACsvConfig::CenterZ]);
	double a = cxr[1] - cxr[0], b = cyr[1] - cyr[0], c = czr[1] - czr[0];
	double diagLength = std::sqrt(std::pow(a, 2) + std::pow(b, 2) + std::pow(c, 2));
	double const * lengthRange = m_data->splomData->paramRange(mapping[iACsvConfig::Length]);
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
			diffs.resize(iAFiberCharData::FiberValueCount+DistanceMetricCount);
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				auto & timeStepDiffs = diffs[diffID].timestep;
				timeStepDiffs.resize(timeStepCount);
				for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
				{
					// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
					size_t refFiberID = d.refDiffFiber[fiberID].dist[BestDistanceMetric][0].index;
					timeStepDiffs[timeStep] = d.timeValues[timeStep][fiberID][diffID]
						- ref.table->GetValue(refFiberID, diffCols[diffID]).ToDouble();
				}
			}
			for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
			{
				auto & timeStepDiffs = diffs[iAFiberCharData::FiberValueCount + distID].timestep;
				timeStepDiffs.resize(timeStepCount);
				size_t refFiberID = d.refDiffFiber[fiberID].dist[distID][0].index;
				iAFiberData refFiber(ref.table, refFiberID, mapping);
				for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
				{
					iAFiberData fiber(d.timeValues[timeStep][fiberID]);
					double dist = getDistance(fiber, refFiber, distID, diagLength, maxLength);
					timeStepDiffs[timeStep] = dist;
				}
			}
		}
	}
	size_t splomID = 0;
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		if (resultID == m_referenceID)
		{
			splomID += d.fiberCount;
			continue;
		}
		for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
		{
			auto & diffData = d.refDiffFiber[fiberID];
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				size_t tableColumnID = m_data->splomData->numParams() - (iAFiberCharData::FiberValueCount + DistanceMetricCount + EndColumns) + diffID;
				double lastValue = diffData.diff[diffID].timestep[d.timeValues.size() - 1];
				m_data->splomData->data()[tableColumnID][splomID] = lastValue;
				d.table->SetValue(fiberID, tableColumnID, lastValue); // required for coloring 3D view by these diffs!
			}
			for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
			{
				double dist = diffData.dist[distID][0].distance;
				size_t tableColumnID = m_data->splomData->numParams() - (DistanceMetricCount + EndColumns) + distID;
				m_data->splomData->data()[tableColumnID][splomID] = dist;
				d.table->SetValue(fiberID, tableColumnID, dist); // required for coloring 3D view by these distances!
			}
			++splomID;
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
			auto & bestFiberBestDist = d.refDiffFiber[fiberID].dist[BestDistanceMetric][0];
			size_t refFiberID = bestFiberBestDist.index;
			refDistSum[refFiberID] += bestFiberBestDist.distance;
			refMatchCount[refFiberID] += 1;
		}
	}
	size_t colID = m_data->result[m_referenceID].table->GetNumberOfColumns();
	addColumn(m_data->result[m_referenceID].table, 0, "AvgDistance", ref.fiberCount);
	m_data->avgRefFiberMatch.resize(ref.fiberCount);
	for (size_t fiberID = 0; fiberID < ref.fiberCount; ++fiberID)
	{
		double value = (refMatchCount[fiberID] == 0) ? -1 : refDistSum[fiberID] / refMatchCount[fiberID];
		m_data->avgRefFiberMatch[fiberID] = value;
		m_data->result[m_referenceID].table->SetValue(fiberID, colID, value);
		//DEBUG_LOG(QString("Fiber %1: matches=%2, distance sum=%3, average=%4")
		//	.arg(fiberID).arg(refDistSum[fiberID]).arg(refMatchCount[fiberID]).arg(value));
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
