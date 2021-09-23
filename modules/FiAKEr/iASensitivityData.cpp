/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASensitivityData.h"

#include "iAFiberResult.h"
#include "iARefDistCompute.h"    // for CacheFileQtDataStreamVersion, etc.

// charts:
#include <iASPLOMData.h>

// Segmentation - for distance measure stuff!
#include "iAVectorTypeImpl.h"
#include "iAVectorDistanceImpl.h"

#include <iALog.h>
#include <iAPerformanceHelper.h>
#include <iAProgress.h>
#include <iAToolsVTK.h>
#include <qthelper/iAQtEndl.h>

#include <vtkImageData.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace
{
	using HistogramType = QVector<double>;

	const QString DissimilarityMatrixCacheFileIdentifier("DissimilarityMatrixCache");
	const quint32 DissimilarityMatrixCacheFileVersion(3);
	// change from v1 to v2:
	//   - changed data types in iAFiberSimilarity:
	//     - index        : quint64 -> quint32
	//     - dissimilarity: double -> float
	// change from v2 to v3:
	//   - non-symmetric matrix (to find respective, "directed" best matches in the other result)

	double distributionDifference(HistogramType const& distr1, HistogramType const& distr2, int diffType)
	{
		assert(distr1.size() == distr2.size());
		QSharedPointer<iAVectorType> dist1Ptr(new iARefVector<HistogramType>(distr1));
		QSharedPointer<iAVectorType> dist2Ptr(new iARefVector<HistogramType>(distr2));
		if (diffType == 0)
		{
			/*
		// (start of) distance between AVERAGEs - (can that be useful?)
		// approximate average over all values by building sum weighted by histogram
		for (int i = 0; i < distr1.size(); ++i)
		{

		}
		*/
			iAL2NormDistance l2Dist;
			return l2Dist.GetDistance(dist1Ptr, dist2Ptr);
		}
		else if (diffType == 1)
		{
			iAJensenShannonDistance jsDist;
			return jsDist.GetDistance(dist1Ptr, dist2Ptr);
		}
		else
		{
			LOG(lvlError, QString("invalid diffType %1").arg(diffType));
			return 0;
		}
	}
	inline bool boundingBoxesIntersect(iAAABB const& bb1, iAAABB const& bb2)
	{
		const int MinIdx = 0, MaxIdx = 1;
		return bb1[MaxIdx].x() > bb2[MinIdx].x() && bb2[MaxIdx].x() > bb1[MinIdx].x() &&
			bb1[MaxIdx].y() > bb2[MinIdx].y() && bb2[MaxIdx].y() > bb1[MinIdx].y() &&
			bb1[MaxIdx].z() > bb2[MinIdx].z() && bb2[MaxIdx].z() > bb1[MinIdx].z();
	}

	std::vector<size_t> intersectingBoundingBox(iAAABB const& fixedFiberBB, std::vector<iAAABB> const& otherFiberBBs)
	{
		std::vector<size_t> fiberIDs;
		for (size_t f = 0; f < otherFiberBBs.size(); ++f)
		{
			if (boundingBoxesIntersect(fixedFiberBB, otherFiberBBs[f]))
			{
				fiberIDs.push_back(f);
			}
		}
		return fiberIDs;
	}

	void getBestMatches2(iAFiberData const& fiber, std::vector<iAFiberData> const& otherFibers,
		QVector<QVector<iAFiberSimilarity>>& bestMatches, std::vector<size_t> const& candidates, double diagonalLength,
		double maxLength, std::vector<std::pair<int, bool>>& measuresToCompute)
	{
		int bestMatchesStartIdx = bestMatches.size();
		assert(measuresToCompute.size() < std::numeric_limits<int>::max());
		assert(bestMatchesStartIdx + measuresToCompute.size() < std::numeric_limits<int>::max());
		int numOfNewMeasures = static_cast<int>(measuresToCompute.size());
		bestMatches.resize(bestMatchesStartIdx + numOfNewMeasures);
		auto maxNumberOfCloseFibers = std::min(static_cast<int>(candidates.size()),
			std::min(iARefDistCompute::MaxNumberOfCloseFibers,
				static_cast<iARefDistCompute::ContainerSizeType>(otherFibers.size())));
		for (int d = 0; d < numOfNewMeasures; ++d)
		{
			QVector<iAFiberSimilarity> similarities;
			similarities.resize(static_cast<int>(candidates.size()));
			for (qvectorsizetype bestMatchID = 0; bestMatchID < static_cast<qvectorsizetype>(candidates.size());
				 ++bestMatchID)
			{
				size_t refFiberID = candidates[bestMatchID];
				similarities[bestMatchID].index = static_cast<quint32>(refFiberID);
				double curDissimilarity = getDissimilarity(
					fiber, otherFibers[refFiberID], measuresToCompute[d].first, diagonalLength, maxLength);
				if (std::isnan(curDissimilarity))
				{
					curDissimilarity = 0;
				}
				similarities[bestMatchID].dissimilarity = curDissimilarity;
			}
			std::sort(similarities.begin(), similarities.end());
			std::copy(similarities.begin(), similarities.begin() + maxNumberOfCloseFibers,
				std::back_inserter(bestMatches[bestMatchesStartIdx + d]));
		}
	}

	/*
	iAVec3i mapPointToIndex(iAVec3f const& pt, double const* origin, double const* spacing, int const* size)
	{
		iAVec3i result;
		for (int i = 0; i < 3; ++i)
		{
			result[i] = clamp(0, size[i] - 1, static_cast<int>(std::floor((pt[i] - origin[i]) / spacing[i])));
		}
		return result;
	}
*/
	const size_t NoMatch = std::numeric_limits<size_t>::max();

	size_t findMatch(iADissimilarityMatrixType const& dissimMatrix,
		iASensitivityData::UniqueFibersT const& uniqueFibers,
		iASensitivityData::FiberToUniqueMapT const& mapFiberToUnique, iASensitivityData::FiberKeyT const& curF,
		int measureIdx)
	{
		size_t r1 = curF.first;
		size_t f1 = curF.second;
		for (size_t r0 = 0; r0 < r1; ++r0)
		{
			// Questions:
			// - do all need to match or only one?
			// - one direction or both?
			auto& fiberDissim = dissimMatrix[static_cast<qvectorsizetype>(r1)][static_cast<qvectorsizetype>(r0)]
									.fiberDissim[static_cast<qvectorsizetype>(f1)];
			if (fiberDissim.size() == 0 ||
				fiberDissim[measureIdx].size() == 0)  // potentially, there is no match between results at all
			{
				continue;
			}
			auto& match = fiberDissim[measureIdx][0];
			if (dblApproxEqual(match.dissimilarity, 1.0f))  // only non-match, skip as well
			{
				continue;
			}
			auto f0 = match.index;
			auto cand = std::make_pair(r0, f0);
			size_t uniqueID = mapFiberToUnique[cand];
			for (size_t m = 0; m < uniqueFibers[uniqueID].size(); ++m)
			{
				auto rm = uniqueFibers[uniqueID][m].first;
				if (rm == r1)  // if another fiber of same result was already added to that unique fiber,
				{              // we need to skip it for matching checks... maybe debug output?
					continue;
				}
				auto fm = uniqueFibers[uniqueID][m].second;
				// check reverse match(es):
				auto& revMatchDissim = dissimMatrix[static_cast<qvectorsizetype>(rm)][static_cast<qvectorsizetype>(r1)]
										   .fiberDissim[static_cast<qvectorsizetype>(fm)];
				if (revMatchDissim.size() == 0 || revMatchDissim[measureIdx].size() == 0 ||
					revMatchDissim[measureIdx][0].index != f1)
				{
					/*
					LOG(lvlDebug, QString("r1=%1, f1=%2: Match to rm=%3, fm=%4 (m=%5) "
						"isn't a reverse match; that's fID=%6!")
						.arg(r1).arg(f1).arg(rm).arg(fm).arg(m).arg(bestMatchoffmInr1));
					*/
				}
				if (rm == r0)
				{
					continue;
				}
				// check "other" fibers in same unique fiber cluster for match:
				auto matchOther = dissimMatrix[static_cast<qvectorsizetype>(r1)][static_cast<qvectorsizetype>(r0)]
									  .fiberDissim[static_cast<qvectorsizetype>(f1)][measureIdx][0];
				if (fm != matchOther.index)
				{
					/*
					LOG(lvlDebug, QString("r1=%1, f1=%2: Match not confirmed for m=%3 (rm=%4, fm=%5);"
						" best match would be fID=%6!")
						.arg(r1).arg(f1).arg(m).arg(rm).arg(fm).arg(matchOther.index));
					*/
				}
			}
			return uniqueID;
		}
		return NoMatch;
	}

	void projectFiberToImage(iAFiberData const& fiberData, iAAABB const& bb, vtkSmartPointer<vtkImageData> img,
		int const size[3], iAVec3d const& spacing, iAVec3d const& origin)
	{
		// naive / brute force way:
		// for each voxel v
		//     check if any part of v is covered by current fiber (by checking all edge points?)
		//     output: covered ? 1 : 0
		//  (variant: instead of 1/0, check percentage to which voxel is covered by fiber?) -> probably hard to do

		// optimization:
		//     take AABB for fiber
		//     compute index range for AABB
		// only iterate over this range
		iAVec3i minC = (bb[0] - origin) / spacing, maxC = (bb[1] - origin) / spacing;
		for (int i = 0; i < 3; ++i)
		{
			if (minC[i] < 0 || minC[i] > size[i] || maxC[i] < 0 || maxC[i] > size[i])
			{
				LOG(lvlDebug, QString("Interesting fiber: outside bb!"));
			}
			minC[i] = clamp(0, size[i] + 1, minC[i]);
			maxC[i] = clamp(0, size[i] + 1, maxC[i]);
		}

#pragma omp parallel for
		for (int x = minC[0]; x < maxC[0]; ++x)
		{
			for (int y = minC[1]; y < maxC[1]; ++y)
			{
				for (int z = minC[2]; z < maxC[2]; ++z)
				{
					iAVec3d coord(x, y, z);
					// get the 8 corners of the voxel cube (in no particular order, just based on offsets)
					const size_t CornerCount = 8;
					iAVec3f corners[CornerCount];
					corners[0] = origin + coord * spacing;
					for (int i = 0; i < 3; ++i)
					{
						iAVec3d tmpcoord(coord);
						tmpcoord[i] += 1;
						corners[1 + i] = origin + tmpcoord * spacing;
						tmpcoord[(i + 1) % 3] += 1;
						corners[4 + i] = origin + tmpcoord * spacing;
					}
					corners[7] = origin + (coord + 1) * spacing;
					bool match = false;
					for (size_t c = 0; c < CornerCount; ++c)
					{
						if (pointContainedInFiber(corners[c], fiberData))
						{
							match = true;
							break;
						}
					}
					if (match)
					{
						img->SetScalarComponentFromDouble(x, y, z, 0, img->GetScalarComponentAsDouble(x, y, z, 0) + 1);
					}
				}
			}
		}
	}

}  // namespace


QDataStream& operator<<(QDataStream& out, const iAResultPairInfo& pairInfo)
{
	out << pairInfo.avgDissim;
	out << pairInfo.fiberDissim;
	return out;
}

QDataStream& operator>>(QDataStream& in, iAResultPairInfo& pairInfo)
{
	in >> pairInfo.avgDissim;
	in >> pairInfo.fiberDissim;
	return in;
}


iASensitivityData::iASensitivityData(QSharedPointer<iAFiberResultsCollection> data, QStringList const& paramNames,
	std::vector<std::vector<double>> const& paramValues) :
	m_data(data), m_paramNames(paramNames), m_paramValues(paramValues), m_aborted(false)
{
}

void iASensitivityData::compute(iAProgress* progress)
{
	progress->setStatus("Storing parameter set values");
	for (size_t p = 0; p < m_paramValues[0].size(); p += m_starGroupSize)
	{
		QVector<double> parameterSet;
		for (size_t v = 0; v < m_paramValues.size(); ++v)
		{
			parameterSet.push_back(m_paramValues[v][p]);
		}
		paramSetValues.push_back(parameterSet);
		progress->emitProgress(static_cast<int>(100 * p / m_paramValues[0].size()));
	}

	progress->setStatus("Computing characteristics distribution (histogram) for all results.");
	// TODO: common storage for that in data!
	m_charHistograms.resize(static_cast<int>(m_data->result.size()));

	int numCharSelected = m_charSelected.size();
	/*
	for (auto selCharIdx = 0; selCharIdx < m_charSelected.size(); ++selCharIdx)
	{
		double rangeMin = m_data->spmData->paramRange(m_charSelected[selCharIdx])[0];
		double rangeMax = m_data->spmData->paramRange(m_charSelected[selCharIdx])[1];
		LOG(lvlInfo, QString("Characteristic idx=%1, charIdx=%2 (%3): %4-%5")
			.arg(selCharIdx).arg(m_charSelected[selCharIdx]).arg(charactName(selCharIdx))
			.arg(rangeMin).arg(rangeMax));
	}
	*/

	for (size_t rIdx = 0; rIdx < m_data->result.size(); ++rIdx)
	{
		progress->emitProgress(static_cast<int>(100 * rIdx / m_data->result.size()));
		auto const& r = m_data->result[rIdx];
		// TODO: skip some columns? like ID...
		m_charHistograms[static_cast<qvectorsizetype>(rIdx)].reserve(numCharSelected);
		for (auto charIdx : m_charSelected)
		{
			// make sure of all histograms for the same characteristic have the same range
			double rangeMin = m_data->spmData->paramRange(charIdx)[0];
			double rangeMax = m_data->spmData->paramRange(charIdx)[1];
			std::vector<double> fiberData(r.fiberCount);
			for (size_t fiberID = 0; fiberID < r.fiberCount; ++fiberID)
			{
				fiberData[fiberID] = r.table->GetValue(fiberID, charIdx).ToDouble();
			}
			auto histogram = createHistogram(fiberData, m_histogramBins, rangeMin, rangeMax);
			m_charHistograms[static_cast<qvectorsizetype>(rIdx)].push_back(histogram);
		}
	}
	if (m_aborted)
	{
		return;
	}

	// for each characteristic
	//     for each varied parameter
	//         for each selected characteristics difference measure
	//             for each "aggregation type" - left only, right only, average/? over full range
	//                 for each point in parameter space (of base sampling method)
	//                     compute local change by that difference measure

	progress->setStatus("Computing sensitivities based on characteristics distribution differences.");
	const int NumOfVarianceAggregation = 4;

	paramStep.fill(0.0, m_variedParams.size());
	sensitivityField.resize(numCharSelected);
	aggregatedSensitivities.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		progress->emitProgress(100 * charIdx / numCharSelected);
		//int charactID = m_charSelected[charIdx];
		//auto charactName = spmData->parameterName(charactID);
		//LOG(lvlDebug, QString("Characteristic %1 (%2):").arg(charIdx).arg(charactName));
		sensitivityField[charIdx].resize(m_charDiffMeasure.size());
		aggregatedSensitivities[charIdx].resize(m_charDiffMeasure.size());
		for (int diffMeasureIdx = 0; diffMeasureIdx < m_charDiffMeasure.size(); ++diffMeasureIdx)
		{
			//LOG(lvlDebug, QString("    Difference Measure %1 (%2)").arg(diffMeasure).arg(DistributionDifferenceMeasureNames()[diffMeasure]));
			auto& field = sensitivityField[charIdx][diffMeasureIdx];
			field.resize(NumOfVarianceAggregation);
			auto& agg = aggregatedSensitivities[charIdx][diffMeasureIdx];
			agg.resize(NumOfVarianceAggregation);
			for (int i = 0; i < NumOfVarianceAggregation; ++i)
			{
				field[i].resize(m_variedParams.size());
				agg[i].fill(0.0, m_variedParams.size());
			}
			for (int paramIdx = 0; paramIdx < m_variedParams.size(); ++paramIdx)
			{
				for (int i = 0; i < NumOfVarianceAggregation; ++i)
				{
					field[i][paramIdx].resize(paramSetValues.size());
				}
				// TODO: unify with other loops over STARs
				//QString paramName(m_paramNames[m_variedParams[paramIdx]]);
				//LOG(lvlDebug, QString("  Parameter %1 (%2):").arg(paramIdx).arg(paramName));
				int origParamColIdx = m_variedParams[paramIdx];
				// aggregation types:
				//     - for now: one step average, left only, right only, average over all steps
				//     - future: overall (weighted) average, values over multiples of step size, ...
				int numAllLeft = 0,
					numAllRight = 0,
					numAllLeftRight = 0,
					numAllTotal = 0;
				for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
				{
					int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

					// first - then + steps (both skipped if value +/- step exceeds bounds
					double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
					double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
					double paramDiff = paramStartParamVal - groupStartParamVal;
					//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
					//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
					//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));

					if (paramStep[paramIdx] == 0)
					{
						paramStep[paramIdx] = std::abs(paramDiff);
						LOG(lvlInfo, QString("Param %1 step width: %2").arg(paramIdx).arg(paramStep[paramIdx]));
					}

					double leftVar = 0;
					int numLeftRight = 0;
					if (paramDiff > 0)
					{
						leftVar = distributionDifference(
							m_charHistograms[resultIdxGroupStart][charIdx],
							m_charHistograms[resultIdxParamStart][charIdx],
							m_charDiffMeasure[diffMeasureIdx]);
						//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
						++numLeftRight;
						++numAllLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < m_numOfSTARSteps)
					{
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
						paramDiff = paramStartParamVal - paramVal;
						++k;
					}
					double rightVar = 0;
					if (paramDiff < 0) // additional check required??
					{
						int firstPosStepIdx = resultIdxParamStart + (k - 1);
						rightVar = distributionDifference(
							m_charHistograms[resultIdxGroupStart][charIdx],
							m_charHistograms[firstPosStepIdx][charIdx],
							m_charDiffMeasure[diffMeasureIdx]);
						//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
						++numLeftRight;
						++numAllRight;
					}
					double sumTotal = 0;
					bool wasSmaller = true;
					for (int i = 0; i < m_numOfSTARSteps; ++i)
					{
						int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
						if (paramVal > paramStartParamVal && wasSmaller)
						{
							wasSmaller = false;
							compareIdx = resultIdxGroupStart;
						}
						double difference = distributionDifference(
							m_charHistograms[compareIdx][charIdx],
							m_charHistograms[resultIdxParamStart + i][charIdx],
							m_charDiffMeasure[diffMeasureIdx]);
						sumTotal += difference;
					}
					numAllLeftRight += numLeftRight;
					numAllTotal += m_numOfSTARSteps;
					double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
					double meanTotal = sumTotal / m_numOfSTARSteps;
					//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
					//LOG(lvlDebug, QString("        (sum total var = %1) / (m_numOfSTARSteps = %2)  = %3")
					//	.arg(sumTotal).arg(m_numOfSTARSteps).arg(meanTotal));
					field[0][paramIdx][paramSetIdx] = meanLeftRightVar;
					field[1][paramIdx][paramSetIdx] = leftVar;
					field[2][paramIdx][paramSetIdx] = rightVar;
					field[3][paramIdx][paramSetIdx] = meanTotal;

					agg[0][paramIdx] += meanLeftRightVar;
					agg[1][paramIdx] += leftVar;
					agg[2][paramIdx] += rightVar;
					agg[3][paramIdx] += meanTotal;
				}
				assert(numAllLeftRight == (numAllLeft + numAllRight));
				agg[0][paramIdx] /= numAllLeftRight;
				agg[1][paramIdx] /= numAllLeft;
				agg[2][paramIdx] /= numAllRight;
				agg[3][paramIdx] /= numAllTotal;
				//LOG(lvlDebug, QString("      LeftRight=%1, Left=%2, Right=%3, Total=%4")
				//	.arg(agg[0]).arg(agg[1]).arg(agg[2]).arg(agg[3]));
			}
		}
	}
	if (m_aborted)
	{
		return;
	}
	progress->setStatus("Computing fiber count histogram.");

	QVector<double> fiberCounts;
	for (size_t resultIdx = 0; resultIdx < m_data->result.size(); ++resultIdx)
	{
		fiberCounts.push_back(m_data->result[resultIdx].fiberCount);
	}
	m_fiberCountRange[0] = m_fiberCountRange[1] = std::numeric_limits<double>::infinity();
	fiberCountHistogram = createHistogram(fiberCounts, m_histogramBins,
		m_fiberCountRange[0], m_fiberCountRange[1], true);
	if (m_aborted)
	{
		return;
	}

	progress->setStatus("Computing fiber count sensitivities.");
	// TODO: unify with other loops over STARs
	sensitivityFiberCount.resize(NumOfVarianceAggregation);
	aggregatedSensitivitiesFiberCount.resize(NumOfVarianceAggregation);
	for (int i = 0; i < NumOfVarianceAggregation; ++i)
	{
		sensitivityFiberCount[i].resize(m_variedParams.size());
		aggregatedSensitivitiesFiberCount[i].fill(0.0, m_variedParams.size());
	}
	for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
	{
		progress->emitProgress(100 * paramIdx / m_variedParams.size());
		int origParamColIdx = m_variedParams[paramIdx];
		for (int i = 0; i < NumOfVarianceAggregation; ++i)
		{
			sensitivityFiberCount[i][paramIdx].resize(paramSetValues.size());
		}
		int numAllLeft = 0,
			numAllRight = 0,
			numAllLeftRight = 0,
			numAllTotal = 0;
		for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
		{
			int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
			int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

			// first - then + steps (both skipped if value +/- step exceeds bounds
			double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
			double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
			double paramDiff = paramStartParamVal - groupStartParamVal;
			//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
			//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
			//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));

			assert(paramStep[paramIdx] != 0);
			if (paramStep[paramIdx] == 0)
			{
				paramStep[paramIdx] = std::abs(paramDiff);
			}

			double leftVar = 0;
			int numLeftRight = 0;
			if (paramDiff > 0)
			{
				leftVar = std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount) -
					m_data->result[resultIdxParamStart].fiberCount);
				//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
				++numLeftRight;
				++numAllLeft;
			}

			int k = 1;
			while (paramDiff > 0 && k < m_numOfSTARSteps)
			{
				double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
				paramDiff = paramStartParamVal - paramVal;
				++k;
			}
			double rightVar = 0;
			if (paramDiff < 0) // additional check required??
			{
				int firstPosStepIdx = resultIdxParamStart + (k - 1);
				rightVar = std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount) -
					m_data->result[firstPosStepIdx].fiberCount);
				//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
				++numLeftRight;
				++numAllRight;
			}
			double sumTotal = 0;
			bool wasSmaller = true;
			for (int i = 0; i < m_numOfSTARSteps; ++i)
			{
				int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
				double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
				if (paramVal > paramStartParamVal && wasSmaller)
				{
					wasSmaller = false;
					compareIdx = resultIdxGroupStart;
				}
				double difference = std::abs(static_cast<double>(m_data->result[compareIdx].fiberCount) -
					m_data->result[resultIdxParamStart + i].fiberCount);
				sumTotal += difference;
			}
			numAllLeftRight += numLeftRight;
			numAllTotal += m_numOfSTARSteps;
			double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
			double meanTotal = sumTotal / m_numOfSTARSteps;
			//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
			//LOG(lvlDebug, QString("        (sum total var = %1) / (m_numOfSTARSteps = %2)  = %3")
			//	.arg(sumTotal).arg(m_numOfSTARSteps).arg(meanTotal));
			sensitivityFiberCount[0][paramIdx][paramSetIdx] = meanLeftRightVar;
			sensitivityFiberCount[1][paramIdx][paramSetIdx] = leftVar;
			sensitivityFiberCount[2][paramIdx][paramSetIdx] = rightVar;
			sensitivityFiberCount[3][paramIdx][paramSetIdx] = meanTotal;

			aggregatedSensitivitiesFiberCount[0][paramIdx] += meanLeftRightVar;
			aggregatedSensitivitiesFiberCount[1][paramIdx] += leftVar;
			aggregatedSensitivitiesFiberCount[2][paramIdx] += rightVar;
			aggregatedSensitivitiesFiberCount[3][paramIdx] += meanTotal;
		}
	}
	if (m_aborted)
	{
		return;
	}

	progress->setStatus("Compute variation histogram");
	//charHistHist.resize(numCharSelected);
	charHistVar.resize(numCharSelected);
	charHistVarAgg.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		progress->emitProgress(100 * charIdx / numCharSelected);
		//charHistHist[charIdx].resize(NumOfVarianceAggregation);
		charHistVar[charIdx].resize(NumOfVarianceAggregation);
		charHistVarAgg[charIdx].resize(NumOfVarianceAggregation);
		for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
		{
			//charHistHist[charIdx][aggIdx].resize(m_variedParams.size());
			charHistVar[charIdx][aggIdx].resize(m_variedParams.size());
			charHistVarAgg[charIdx][aggIdx].resize(m_variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
		{
			for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
			{
				//charHistHist[charIdx][aggIdx][paramIdx].resize(m_histogramBins);
				charHistVar[charIdx][aggIdx][paramIdx].resize(m_histogramBins);
				charHistVarAgg[charIdx][aggIdx][paramIdx].fill(0.0, m_histogramBins);
			}
			// TODO: unify with other loops over STARs?
			int origParamColIdx = m_variedParams[paramIdx];

			for (int bin = 0; bin < m_histogramBins; ++bin)
			{
				int numAllLeft = 0,
					numAllRight = 0,
					numAllTotal = 0;
				for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
				{
					//charHistHist[charIdx][aggIdx][paramIdx][bin].resize(paramSetValues.size());
					charHistVar[charIdx][aggIdx][paramIdx][bin].resize(paramSetValues.size());
				}
				for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
				{
					int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;
					/*
					for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
					{
						charHistHist[charIdx][aggIdx][paramIdx][bin][paramSetIdx].resize(paramSetValues.size());
					}
					*/
					// first - then + steps (both skipped if value +/- step exceeds bounds
					double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
					double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
					double paramDiff = paramStartParamVal - groupStartParamVal;
					//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
					//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
					//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));
					assert(paramStep[paramIdx] != 0);
					if (paramStep[paramIdx] == 0)
					{
						paramStep[paramIdx] = std::abs(paramDiff);
					}

					int numLeftRight = 0;
					/*
					for (int agg = 0; agg < NumOfVarianceAggregation; ++agg)
					{
						charHistHist[charIdx][agg][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxGroupStart][charIdx][bin]);
					}
					*/
					charHistVar[charIdx][0][paramIdx][bin][paramSetIdx] = 0;
					if (paramDiff > 0)
					{
						// left-only:
						//charHistHist[charIdx][0][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxParamStart][charIdx][bin]);
						// left+right:
						//charHistHist[charIdx][2][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxGroupStart][charIdx][bin]);
						charHistVar[charIdx][0][paramIdx][bin][paramSetIdx] =
							std::abs(m_charHistograms[resultIdxGroupStart][charIdx][bin] - m_charHistograms[resultIdxParamStart][charIdx][bin]);
						charHistVarAgg[charIdx][0][paramIdx][bin] += charHistVar[charIdx][0][paramIdx][bin][paramSetIdx];
						charHistVarAgg[charIdx][2][paramIdx][bin] += charHistVar[charIdx][0][paramIdx][bin][paramSetIdx];
						//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
						++numLeftRight;
						++numAllLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < m_numOfSTARSteps)
					{
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
						paramDiff = paramStartParamVal - paramVal;
						++k;
					}
					charHistVar[charIdx][1][paramIdx][bin][paramSetIdx] = 0;
					if (paramDiff < 0) // additional check required??
					{
						int firstPosStepIdx = resultIdxParamStart + (k - 1);
						// left-only:
						//charHistHist[charIdx][1][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[firstPosStepIdx][charIdx][bin]);
						// left+right:
						//charHistHist[charIdx][2][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[firstPosStepIdx][charIdx][bin]);
						charHistVar[charIdx][1][paramIdx][bin][paramSetIdx] =
							std::abs(m_charHistograms[resultIdxGroupStart][charIdx][bin] - m_charHistograms[firstPosStepIdx][charIdx][bin]);
						charHistVarAgg[charIdx][1][paramIdx][bin] += charHistVar[charIdx][1][paramIdx][bin][paramSetIdx];
						charHistVarAgg[charIdx][2][paramIdx][bin] += charHistVar[charIdx][1][paramIdx][bin][paramSetIdx];
						//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
						++numLeftRight;
						++numAllRight;
					}
					charHistVar[charIdx][2][paramIdx][bin][paramSetIdx] /= numLeftRight;
					bool wasSmaller = true;
					charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] = 0;
					numAllTotal += m_numOfSTARSteps;
					for (int i = 0; i < m_numOfSTARSteps; ++i)
					{
						//charHistHist[charIdx][3][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxParamStart + i][charIdx][bin]);
						int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
						if (paramVal > paramStartParamVal && wasSmaller)
						{
							wasSmaller = false;
							compareIdx = resultIdxGroupStart;
						}
						charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] +=
							std::abs(m_charHistograms[compareIdx][charIdx][bin] -
								m_charHistograms[resultIdxParamStart + i][charIdx][bin]);
					}
					charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] /= m_numOfSTARSteps;//charHistHist[charIdx][3][paramIdx][bin][paramSetIdx].size();
					charHistVarAgg[charIdx][3][paramIdx][bin] += charHistVar[charIdx][3][paramIdx][bin][paramSetIdx];
				}
				assert(numAllTotal == paramSetValues.size() * m_numOfSTARSteps);
				charHistVarAgg[charIdx][0][paramIdx][bin] /= numAllLeft;
				charHistVarAgg[charIdx][1][paramIdx][bin] /= numAllRight;
				charHistVarAgg[charIdx][2][paramIdx][bin] /= (numAllLeft + numAllRight);
				charHistVarAgg[charIdx][3][paramIdx][bin] /= numAllTotal;
			}
		}
	}
	if (m_aborted)
	{
		return;
	}

	progress->setStatus("Computing average characteristics histogram.");
	charHistAvg.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		charHistAvg[charIdx].fill(0, m_histogramBins);
		for (int resultIdx = 0; resultIdx < m_charHistograms.size(); ++resultIdx)
		{
			for (int bin = 0; bin < m_histogramBins; ++bin)
			{
				charHistAvg[charIdx][bin] += m_charHistograms[resultIdx][charIdx][bin];
			}
		}
		for (int bin = 0; bin < m_histogramBins; ++bin)
		{
			charHistAvg[charIdx][bin] /= m_charHistograms.size();
		}
	}

	progress->setStatus("Loading cached dissimilarities between all result pairs.");
	progress->emitProgress(0);
	QVector<int> measures;

	if (readDissimilarityMatrixCache(measures))
	{
		m_resultDissimMeasures.clear();
		for (auto m : measures)
		{
			m_resultDissimMeasures.push_back(std::make_pair(m, true));
		}
	}
	else
	{
		progress->setStatus("Computing dissimilarity between all result pairs.");
		// Thoughts on per-object sensitivity:
		// required: 1-1 match between fibers
		// currently compute on the fly, based on bounding boxes of fibers;
		// for further improvement, spatial subdivision structure would be required

		// Questions:
		// options for characteristic comparison:
		//    1. compute characteristic distribution difference
		//        - advantage: dissimilarity measure independent
		//        - disadvantage: distribution could be same even if lots of differences for single fibers
		//    2. compute matching fibers; then compute characteristic difference; then average this
		//        - advantage: represents actual differences better
		//        - disadvantage: depending on dissimilarity measure (since best match could be computed per dissimiliarity measure
		//    example: compare
		//         - result 1 with fibers a (len=5), b (len=3) and c (len=2)
		//         - result 2 with fibers A (len 3), B (len=2) and C (len=5)
		//         - best matches between result1&2: a <-> A, b <-> B, c <-> C
		//         - option 1 -> exactly the same, 1x5, 1x3, 1x2
		//         - option 2 -> length differences: 2, 1, 3
		int measureCount = static_cast<int>(m_resultDissimMeasures.size());
		int resultCount = static_cast<int>(m_data->result.size());
		m_resultDissimMatrix = iADissimilarityMatrixType(
			resultCount,
			QVector<iAResultPairInfo>(resultCount, iAResultPairInfo(measureCount)));

		for (size_t m = 0; m < m_resultDissimMeasures.size(); ++m)
		{
			measures.push_back(m_resultDissimMeasures[m].first);
		}

		//double overallPairs = resultCount * (resultCount - 1) / 2;
		// for all result pairs r1 and r2, for every fiber f in r1 find those fibers in r2 which best match f
		double overallPairs = static_cast<double>(resultCount) * resultCount;
		size_t curCheckedPairs = 0;
		for (int r1 = 0; r1 < resultCount && !m_aborted; ++r1)
		{
			auto& res1 = m_data->result[r1];
			auto const& mapping = *res1.mapping.data();
			// TODO: only center -> should use bounding box instead!
			double const* cxr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterX]),
				* cyr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterY]),
				* czr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterZ]);
			double a = cxr[1] - cxr[0], b = cyr[1] - cyr[0], c = czr[1] - czr[0];
			double diagonalLength = std::sqrt(std::pow(a, 2) + std::pow(b, 2) + std::pow(c, 2));
			double const* lengthRange = m_data->spmData->paramRange(mapping[iACsvConfig::Length]);
			double maxLength = lengthRange[1] - lengthRange[0];
			for (int r2 = 0; r2 < resultCount && !m_aborted; ++r2)
			{
				auto& mat = m_resultDissimMatrix[r1][r2];
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] = 0;
				}
				if (r1 == r2)
				{
					continue;
				}
				progress->setStatus(QString("Computing dissimilarity between results %1 and %2.").arg(r1).arg(r2));
				int r1FibCount = static_cast<int>(m_data->result[r1].fiberCount);
				std::vector<int> r2MatchCount(measureCount, 0);
				auto& dissimilarities = mat.fiberDissim;
				dissimilarities.resize(r1FibCount);
				// not ideal: for loop seems to be not ideally parallelizable,
				// one spike where 100% is used, then going down to nearly 0, until next loop starts
				int noCanDo = 0;
				size_t candSum = 0;
#pragma omp parallel for reduction(+ : noCanDo, candSum)
				for (int fiberID = 0; fiberID < r1FibCount; ++fiberID)
				{
					auto candidates = intersectingBoundingBox(m_data->result[r1].fiberBB[fiberID], m_data->result[r2].fiberBB);
					if (candidates.size() == 0)
					{
						++noCanDo; // thread-safe
						continue;
					}
					candSum += candidates.size();
					getBestMatches2(m_data->result[r1].fiberData[fiberID], m_data->result[r2].fiberData,
						dissimilarities[fiberID], candidates, diagonalLength, maxLength,
						m_resultDissimMeasures);
					for (int m = 0; m < measureCount; ++m)
					{
						if (dissimilarities[fiberID][m].size() > 0)
						{
							++r2MatchCount[m];
							mat.avgDissim[m] += dissimilarities[fiberID][m][0].dissimilarity;
						}
					}
				}
				LOG(lvlDebug, QString("Result %1x%2: %3 candidates on average, %4 with no bounding box intersections out of %5")
					.arg(r1).arg(r2).arg(candSum / r1FibCount).arg(noCanDo).arg(r1FibCount));
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] /= r2MatchCount[m];
				}
				++curCheckedPairs;
				progress->emitProgress(curCheckedPairs * 100.0 / overallPairs);
			}
		}
		if (m_aborted)
		{
			return;
		}
		writeDissimilarityMatrixCache(measures);
	}
	if (m_resultDissimMatrix.size() == 0)
	{
		LOG(lvlWarn, "Dissimilarity matrix not available!");
		return;
	}

	// determine dissimilarity ranges:
	m_resultDissimRanges.resize(static_cast<qvectorsizetype>(m_resultDissimMeasures.size()));
	for (int m = 0; m < m_resultDissimRanges.size(); ++m)
	{
		m_resultDissimRanges[m].first = std::numeric_limits<double>::max();
		m_resultDissimRanges[m].second = std::numeric_limits<double>::lowest();
	}
	for (qvectorsizetype r1 = 1; r1 < static_cast<qvectorsizetype>(m_data->result.size()) && !m_aborted; ++r1)
	{
		for (qvectorsizetype r2 = 0; r2 < static_cast<qvectorsizetype>(m_data->result.size()) && !m_aborted; ++r2)
		{
			if (r1 == r2)
			{
				continue;
			}
			for (int m = 0; m < m_resultDissimRanges.size(); ++m)
			{
				double dissim = m_resultDissimMatrix[r1][r2].avgDissim[m];
				if (dissim < m_resultDissimRanges[m].first)
				{
					m_resultDissimRanges[m].first = dissim;
				}
				if (dissim > m_resultDissimRanges[m].second)
				{
					m_resultDissimRanges[m].second = dissim;
				}
			}
		}
	}
	for (int m = 0; m < m_resultDissimRanges.size(); ++m)
	{
		LOG(lvlDebug,
			QString("m: %1; range: %2..%3")
			.arg(m)
			.arg(m_resultDissimRanges[m].first)
			.arg(m_resultDissimRanges[m].second));
	}
	if (m_aborted)
	{
		return;
	}

	// dissimilarity measure (index in m_resultDissimMeasures)
	// variation aggregation (see iASensitivityInfo::create)
	// parameter index (second index in paramSetValues / allParamValues)
	// parameter set index (first index in paramSetValues)
	//sensDissimField;
	//aggregatedSensDissim;

	progress->setStatus("Computing aggregated dissimilarity sensitivities");
	int measureCount = static_cast<int>(m_resultDissimMeasures.size());
	// TODO: unify with other loops over STARs
	sensDissimField.resize(measureCount);
	aggregatedSensDissim.resize(measureCount);

	for (int m = 0; m < measureCount && !m_aborted; ++m)
	{
		QVector<double> dissimValuesUsed;
		sensDissimField[m].resize(NumOfVarianceAggregation);
		aggregatedSensDissim[m].resize(NumOfVarianceAggregation);
		for (int a = 0; a < NumOfVarianceAggregation; ++a)
		{
			sensDissimField[m][a].resize(m_variedParams.size());
			aggregatedSensDissim[m][a].fill(0.0, m_variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
		{
			progress->emitProgress(100 * paramIdx / m_variedParams.size());
			int origParamColIdx = m_variedParams[paramIdx];
			for (int a = 0; a < NumOfVarianceAggregation; ++a)
			{
				sensDissimField[m][a][paramIdx].resize(paramSetValues.size());
			}
			int numAllLeft = 0, numAllRight = 0, numAllLeftRight = 0, numAllTotal = 0;
			for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
			{
				int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
				int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

				// first - then + steps (both skipped if value +/- step exceeds bounds
				double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
				double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
				double paramDiff = paramStartParamVal - groupStartParamVal;
				//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
				//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
				//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));
				assert(paramStep[paramIdx] != 0);
				if (paramStep[paramIdx] == 0)
				{
					paramStep[paramIdx] = std::abs(paramDiff);
				}

				double leftVar = 0;
				int numLeftRight = 0;
				if (paramDiff > 0)
				{
					leftVar = m_resultDissimMatrix[resultIdxGroupStart][resultIdxParamStart].avgDissim[m];
					//std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount) - m_data->result[resultIdxParamStart].fiberCount);
				//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
					++numLeftRight;
					++numAllLeft;
				}

				int k = 1;
				while (paramDiff > 0 && k < m_numOfSTARSteps)
				{
					double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
					paramDiff = paramStartParamVal - paramVal;
					++k;
				}
				double rightVar = 0;
				if (paramDiff < 0)  // additional check required??
				{
					int firstPosStepIdx = resultIdxParamStart + (k - 1);
					rightVar = m_resultDissimMatrix[resultIdxGroupStart][firstPosStepIdx].avgDissim[m];
					// std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount) -m_data->result[firstPosStepIdx].fiberCount);
				//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
					++numLeftRight;
					++numAllRight;
				}
				double sumTotal = 0;
				bool wasSmaller = true;
				for (int i = 0; i < m_numOfSTARSteps; ++i)
				{
					int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
					double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
					if (paramVal > paramStartParamVal && wasSmaller)
					{
						wasSmaller = false;
						compareIdx = resultIdxGroupStart;
					}
					double difference = m_resultDissimMatrix[compareIdx][resultIdxParamStart + i].avgDissim[m];
					dissimValuesUsed.push_back(difference);
					//std::abs(static_cast<double>(m_data->result[compareIdx].fiberCount) -	m_data->result[resultIdxParamStart + i].fiberCount);
					sumTotal += difference;
				}
				numAllLeftRight += numLeftRight;
				numAllTotal += m_numOfSTARSteps;
				double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
				double meanTotal = sumTotal / m_numOfSTARSteps;
				//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
				//LOG(lvlDebug, QString("        (sum total var = %1) / (m_numOfSTARSteps = %2)  = %3")
				//	.arg(sumTotal).arg(m_numOfSTARSteps).arg(meanTotal));
				sensDissimField[m][0][paramIdx][paramSetIdx] = meanLeftRightVar;
				sensDissimField[m][1][paramIdx][paramSetIdx] = leftVar;
				sensDissimField[m][2][paramIdx][paramSetIdx] = rightVar;
				sensDissimField[m][3][paramIdx][paramSetIdx] = meanTotal;

				aggregatedSensDissim[m][0][paramIdx] += meanLeftRightVar;
				aggregatedSensDissim[m][1][paramIdx] += leftVar;
				aggregatedSensDissim[m][2][paramIdx] += rightVar;
				aggregatedSensDissim[m][3][paramIdx] += meanTotal;
			}
		}
		QPair<double, double> dissimRange;
		dissimRange.first = dissimRange.second = std::numeric_limits<double>::infinity();
		auto dissimHistogram = createHistogram(dissimValuesUsed, m_histogramBins, dissimRange.first, dissimRange.second, false);
		m_dissimRanges.push_back(dissimRange);
		m_dissimHistograms.push_back(dissimHistogram);
	}

	progress->setStatus("Computing sensitivities based on pairwise characteristics differences.");
	sensitivityFieldPWDiff.resize(numCharSelected);
	aggregatedSensitivitiesPWDiff.resize(numCharSelected);
	const int MeasureIdx = 0;
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		progress->emitProgress(100 * charIdx / numCharSelected);
		auto& field = sensitivityFieldPWDiff[charIdx];
		field.resize(NumOfVarianceAggregation);
		auto& agg = aggregatedSensitivitiesPWDiff[charIdx];
		agg.resize(NumOfVarianceAggregation);
		for (int i = 0; i < NumOfVarianceAggregation; ++i)
		{
			field[i].resize(m_variedParams.size());
			agg[i].fill(0.0, m_variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < m_variedParams.size(); ++paramIdx)
		{
			for (int i = 0; i < NumOfVarianceAggregation; ++i)
			{
				field[i][paramIdx].resize(paramSetValues.size());
			}
			// TODO: unify with other loops over STARs
			int origParamColIdx = m_variedParams[paramIdx];
			// aggregation types:
			//     - for now: one step average, left only, right only, average over all steps
			//     - future: overall (weighted) average, values over multiples of step size, ...
			int numAllLeft = 0, numAllRight = 0, numAllLeftRight = 0, numAllTotal = 0;
			for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
			{
				int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
				int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

				// first - then + steps (both skipped if value +/- step exceeds bounds
				double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
				double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
				double paramDiff = paramStartParamVal - groupStartParamVal;
				double leftVar = 0;
				int numLeftRight = 0;
				if (paramDiff > 0)
				{
					leftVar = characteristicsDifference(m_charSelected[charIdx], resultIdxGroupStart, resultIdxParamStart, MeasureIdx);
					++numLeftRight;
					++numAllLeft;
				}

				int k = 1;
				while (paramDiff > 0 && k < m_numOfSTARSteps)
				{
					double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
					paramDiff = paramStartParamVal - paramVal;
					++k;
				}
				double rightVar = 0;
				if (paramDiff < 0)  // additional check required??
				{
					int firstPosStepIdx = resultIdxParamStart + (k - 1);
					rightVar = characteristicsDifference(m_charSelected[charIdx], resultIdxGroupStart, firstPosStepIdx, MeasureIdx);
					++numLeftRight;
					++numAllRight;
				}
				double sumTotal = 0;
				bool wasSmaller = true;
				for (int i = 0; i < m_numOfSTARSteps; ++i)
				{
					int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
					double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
					if (paramVal > paramStartParamVal && wasSmaller)
					{
						wasSmaller = false;
						compareIdx = resultIdxGroupStart;
					}
					double difference = characteristicsDifference(m_charSelected[charIdx], compareIdx, resultIdxParamStart + i, MeasureIdx);
					sumTotal += difference;
				}
				numAllLeftRight += numLeftRight;
				numAllTotal += m_numOfSTARSteps;
				double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
				double meanTotal = sumTotal / m_numOfSTARSteps;
				field[0][paramIdx][paramSetIdx] = meanLeftRightVar;
				field[1][paramIdx][paramSetIdx] = leftVar;
				field[2][paramIdx][paramSetIdx] = rightVar;
				field[3][paramIdx][paramSetIdx] = meanTotal;

				agg[0][paramIdx] += meanLeftRightVar;
				agg[1][paramIdx] += leftVar;
				agg[2][paramIdx] += rightVar;
				agg[3][paramIdx] += meanTotal;
			}
			assert(numAllLeftRight == (numAllLeft + numAllRight));
			agg[0][paramIdx] /= numAllLeftRight;
			agg[1][paramIdx] /= numAllLeft;
			agg[2][paramIdx] /= numAllRight;
			agg[3][paramIdx] /= numAllTotal;
		}
	}
}

double oneSidedCharacteristicsDifference(int charIdx, iAResultPairInfo const& mat, iAFiberResult const & r0, iAFiberResult const & r1, qvectorsizetype measureIdx)
{
	double diffSum = 0.0;
	for (qvectorsizetype f0 = 0; f0 < r0.fiberCount; ++f0)
	{
		// take best-matching fiber in r1
		// compute difference in characteristic charIdx
		size_t f1 = mat.fiberDissim[f0][measureIdx][0]
						.index;  // best match to fiber f0 in result r1 with dissimilarity measure measureIdx
		double f0Val = r0.table->GetValue(f0, charIdx).ToDouble();
		double f1Val = r1.table->GetValue(f1, charIdx).ToDouble();
		diffSum += std::abs(f0Val - f1Val);
	}
	diffSum /= r0.fiberCount;
	return diffSum;
}

// pairwise CharacteristicsDiffs:
// - direction? both?
// go over all fibers in result a, sum up differences in given characteristic to best match in result b
// maybe use different difference measures?#
double iASensitivityData::characteristicsDifference(int charIdx, qvectorsizetype r0Idx, qvectorsizetype r1Idx, int measureIdx)
{
	// get result data:
	auto& r0 = m_data->result[r0Idx];
	auto& r1 = m_data->result[r1Idx];

	// get dissimilarity matrix:
	auto& mat01 = m_resultDissimMatrix[r0Idx][r1Idx];
	auto& mat10 = m_resultDissimMatrix[r1Idx][r0Idx];

	double diff01 = oneSidedCharacteristicsDifference(charIdx, mat01, r0, r1, measureIdx);
	double diff10 = oneSidedCharacteristicsDifference(charIdx, mat10, r1, r0, measureIdx);
	return (diff01 + diff10) / 2.0;
	// compute geometric average
}

void iASensitivityData::computeSpatialOverview(iAProgress* progress)
{
	// check for cached spatial overview image:
	if (QFile::exists(spatialOverviewCacheFileName()))
	{
		progress->setStatus(QString("Loading cached spatial overview from '%1'.").arg(spatialOverviewCacheFileName()));
		readImage(spatialOverviewCacheFileName(), false, m_spatialOverview);
		return;
	}

	// initialize 3D overview:
	// required: for each result, and each fiber - quality of match to best-matching fiber in all others
	// 	   Q: how to aggregate one value per fiber?
	// 	      A: just average?
	// 	   Q: how to handle no match?
	// 	   Q:
	size_t resultCount = m_data->result.size();
	int const volSize = 128;
	int const size[3] = {volSize, volSize, volSize};
	// find bounding box that accomodates all results:
	std::vector<iAAABB> resultBBs;
	for (size_t r = 0; r < resultCount; ++r)
	{
		resultBBs.push_back(m_data->result[r].bbox);
	}
	iAAABB overallBB;
	mergeBoundingBoxes(overallBB, resultBBs);

	// create volume V of dimensionality XxYxZ
	iAVec3d const spacing = (overallBB[1] - overallBB[0]) / volSize;
	LOG(lvlDebug,
		QString("Overview volume: box (tl=%1, %2, %3; br=%4, %5, %6), spacing (%7, %8, %9)")
			.arg(overallBB[0][0])
			.arg(overallBB[0][1])
			.arg(overallBB[0][2])
			.arg(overallBB[1][0])
			.arg(overallBB[1][1])
			.arg(overallBB[1][2])
			.arg(spacing[0])
			.arg(spacing[1])
			.arg(spacing[2]));

	const int MeasureIdx = 0;

	progress->setStatus("Computing fiber volume percentage.");
	progress->emitProgress(0);
	iAVec3d origin = overallBB[0];

	QFile volPercentOutFile(volumePercentageCacheFileName());
	if (!volPercentOutFile.open(QIODevice::WriteOnly))
	{
		LOG(lvlError,
			QString("FIAKER fiber volume percentage: Cannot open file %1 for writing!")
				.arg(volumePercentageCacheFileName()));
		return;
	}
	QTextStream volPercentOut(&volPercentOutFile);
	size_t overallVoxels = volSize * volSize * volSize;
	volPercentOut << "ResultID,Percentage,FiberVoxel(overall=" << overallVoxels << ")" << QTENDL;
	for (size_t r = 0; r < resultCount && !m_aborted; ++r)
	{
		auto resultFiberImg = allocateImage(VTK_FLOAT, size, spacing.data());
		resultFiberImg->SetOrigin(origin.data());
		fillImage(resultFiberImg, 0);
		auto const& d = m_data->result[r];
		for (size_t f = 0; f < d.fiberCount && !m_aborted; ++f)
		{
			projectFiberToImage(d.fiberData[f], d.fiberBB[f], resultFiberImg, size, spacing, origin);
		}
		// count voxels != 0:
		size_t fiberVoxels = 0;
		FOR_VTKIMG_PIXELS(resultFiberImg, x, y, z)
		{
			if (resultFiberImg->GetScalarComponentAsDouble(x, y, z, 0) != 0)
			{
				++fiberVoxels;
			}
			if (m_aborted)
			{
				break;
			}
		}
		double volPercent = (100.0 * fiberVoxels) / overallVoxels;
		volPercentOut << r << "," << volPercent << "," << fiberVoxels << QTENDL;
		LOG(lvlInfo,
			QString("Result %1: Fibers take up ~ %2 % of the volume (%3 of %4 voxels).")
				.arg(r)
				.arg(volPercent)
				.arg(fiberVoxels)
				.arg(overallVoxels));
	}
	volPercentOutFile.close();

	// NEW spatial overview over variability:

	// find overall bounding box
	// determine (fixed?) dimensions (s_x, s_y, s_z)

	// build list of "unique" fibers:
	// empty unique fiber list u
	// for each result r1:
	//     for each fiber f1 in r1:
	//         fiberkey r0, f0 = best match to r1,f1 in u (where r1 != r0)
	//         if there exists such a f0 (and f1 is also listed as best match for f0 (and all its "matches"))
	//             add (r1, f1) as new "synonym" for (r0, f0) in u
	//         (? else, if f1 is not listed as best match for f0 (and all its "matches")
	// 	           add as new unique fiber but somehow mark it linked to f0
	//         ?) [[-> in first implementation, measure how many such exist, but no special handling]]
	//         else
	// 	           add (r1, f1) as new unique fiber

	// Optimization:
	// while adding fibers to unique fiber list:
	// also create reverse lookup list for fibers:
	// map (rID, fID) -> uniqueID
	// then during match finding:
	//     check all results < current result num
	//     check closest match for that result
	//     go to that uniqueID and add
	//         (and maybe check others?)

	iAPerformanceHelper h;
	h.start("Finding unique fibers", false);
	progress->setStatus("Finding unique fibers");
	progress->emitProgress(0);
	for (size_t r1 = 0; r1 < resultCount && !m_aborted; ++r1)
	{
		auto const& d = m_data->result[r1];
		for (size_t f1 = 0; f1 < d.fiberCount; ++f1)
		{
			auto curFiber = std::make_pair(r1, f1);
			//FiberKeyT matchFiber;
			auto idx = findMatch(
				m_resultDissimMatrix, m_uniqueFibers, m_mapFiberToUnique, curFiber, MeasureIdx);
			if (idx != NoMatch)
			{
				m_uniqueFibers[idx].push_back(curFiber);
				m_mapFiberToUnique.insert(curFiber, idx);
			}
			// special handling if match found but no reverse match / no match to all "synonyms" ?
			else
			{
				std::vector<iASensitivityData::FiberKeyT> vec;
				vec.push_back(curFiber);
				m_uniqueFibers.push_back(vec);
				m_mapFiberToUnique.insert(curFiber, m_uniqueFibers.size() - 1);
			}
		}
		progress->emitProgress(100 * (r1 + 1) / resultCount);
	}
	h.stop();

	LOG(lvlDebug, QString("Found %1 unique fibers across results!").arg(m_uniqueFibers.size()));

	// build per-fiber variability images:
	// empty list of images of per-fiber variability v
	// for each unique fiber f in u:
	//     create empty image i (all 0)
	//     for each synonym s:
	//         project s into voxel space (-> resulting into image i_s with 1 in voxels through which s passes through and 0 where it doesn't)
	//         increase value of all voxels which are covered by s by 1
	//         (alternative: add i_s to i)
	//     create "probability" image out of i
	//     (by dividing it through number of results in ensemble?)
	progress->emitProgress(0);
	h.start("Determining fiber variation images", false);
	// maybe operate with "raw" buffers here instead of vtkImageData objects??
	std::vector<vtkSmartPointer<vtkImageData>> perUniqueFiberVars;
	m_spatialOverview = allocateImage(VTK_FLOAT, size, spacing.data());
	m_spatialOverview->SetOrigin(origin.data());
	fillImage(m_spatialOverview, 0);
	for (size_t uIdx = 0; uIdx < m_uniqueFibers.size() && !m_aborted; ++uIdx)
	{
		progress->setStatus(QString("Determining fiber variation image for unique fiber %1").arg(uIdx));
		auto const& u = m_uniqueFibers[uIdx];
		auto uniqueFiberVarImg = allocateImage(VTK_FLOAT, size, spacing.data());
		uniqueFiberVarImg->SetOrigin(origin.data());
		fillImage(uniqueFiberVarImg, 0);
		for (auto s : u)
		{
			if (m_aborted)
			{
				break;
			}
			auto const& r = m_data->result[s.first];
			projectFiberToImage(r.fiberData[s.second], r.fiberBB[s.second], uniqueFiberVarImg, size, spacing, origin);
		}
		multiplyImage(uniqueFiberVarImg, 1.0 / u.size());
		perUniqueFiberVars.push_back(uniqueFiberVarImg);

		addImages(m_spatialOverview, uniqueFiberVarImg);

		storeImage(uniqueFiberVarImg, uniqueFiberVarCacheFileName(uIdx));

		progress->emitProgress(100 * (uIdx + 1) / m_uniqueFibers.size());
	}
	multiplyImage(m_spatialOverview, m_uniqueFibers.size());
	h.stop();

	// build overall variability image: (-> incorporated in loop above)
	// empty image (s_x, s_y, s_z) a (as average over all unique fiber images:)
	// for each unique fiber image i:
	//     add a to i
	// divide a by number of unique fibers

	storeImage(m_spatialOverview, spatialOverviewCacheFileName());
}

QString iASensitivityData::cacheFileName(QString fileName) const
{
	return QString("%1/cache/%2").arg(m_data->folder).arg(fileName);
}

QString iASensitivityData::dissimilarityMatrixCacheFileName() const
{
	return cacheFileName("dissimilarityMatrix.cache");
}

QString iASensitivityData::volumePercentageCacheFileName() const
{
	return cacheFileName("volumePercentages.csv");
}

QString iASensitivityData::spatialOverviewCacheFileName() const
{
	return cacheFileName("spatialOverview-v0.mhd");
}

QString iASensitivityData::uniqueFiberVarCacheFileName(size_t uIdx) const
{
	return cacheFileName(QString("uniqueFiberVar-%1-v0.mhd").arg(uIdx));
}

bool iASensitivityData::readDissimilarityMatrixCache(QVector<int>& measures)
{
	QFile cacheFile(dissimilarityMatrixCacheFileName());
	// unify with verifyOpenCacheFile?
	if (!cacheFile.exists())
	{
		return false;
	}
	if (!cacheFile.open(QFile::ReadOnly))
	{
		LOG(lvlError, QString("Couldn't open file %1 for reading!").arg(cacheFile.fileName()));
		return false;
	}
	// unify with readResultRefComparison / common cache file version/identifier pattern?
	QDataStream in(&cacheFile);
	in.setVersion(CacheFileQtDataStreamVersion);
	QString identifier;
	in >> identifier;
	if (identifier != DissimilarityMatrixCacheFileIdentifier)
	{
		LOG(lvlError,
			QString("FIAKER cache file '%1': Unknown cache file format - found identifier %2 does not match expected "
					"identifier %3. Please delete file and let it be recreated!")
				.arg(cacheFile.fileName())
				.arg(identifier)
				.arg(DissimilarityMatrixCacheFileIdentifier));
		return false;
	}
	quint32 version;
	in >> version;
	if (version < DissimilarityMatrixCacheFileVersion)
	{
		LOG(lvlError,
			QString("FIAKER cache file '%1': Too old, incompatible cache version %2; "
					"")
				.arg(cacheFile.fileName())
				.arg(version));
		return false;
	}
	if (version > DissimilarityMatrixCacheFileVersion)
	{
		LOG(lvlError,
			QString("FIAKER cache file '%1': Invalid or too high version number (%2), expected %3 or less. Please "
					"delete file and let it be recreated!")
				.arg(cacheFile.fileName())
				.arg(version)
				.arg(DissimilarityMatrixCacheFileVersion));
		return false;
	}
	in >> measures;
	in >> m_resultDissimMatrix;
	cacheFile.close();
	return true;
}

void iASensitivityData::writeDissimilarityMatrixCache(QVector<int> const& measures) const
{
	QFileInfo fi(QFileInfo(dissimilarityMatrixCacheFileName()).absolutePath());
	if ((fi.exists() && !fi.isDir()) || !QDir(fi.absoluteFilePath()).mkpath("."))
	{
		LOG(lvlError, QString("Could not create output directory '%1'").arg(fi.absoluteFilePath()));
		return;
	}
	QFile cacheFile(dissimilarityMatrixCacheFileName());
	if (!cacheFile.open(QFile::WriteOnly))
	{
		LOG(lvlError, QString("Couldn't open file %1 for writing!").arg(cacheFile.fileName()));
		return;
	}
	QDataStream out(&cacheFile);
	out.setVersion(CacheFileQtDataStreamVersion);
	// write header:
	out << DissimilarityMatrixCacheFileIdentifier;
	out << DissimilarityMatrixCacheFileVersion;

	out << measures;
	out << m_resultDissimMatrix;
}

void iASensitivityData::abort()
{
	m_aborted = true;
}