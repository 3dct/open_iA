/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASensitivityInfo.h"

// Core
#include <charts/iASPLOMData.h>
#include <charts/qcustomplot.h>
#include <charts/iAScatterPlotWidget.h>
#include <io/iAFileUtils.h>
#include <iAColorTheme.h>
#include <iAJobListView.h>
#include <iALog.h>
#include <iALUT.h>
#include <iAMathUtility.h>
#include <iARunAsync.h>
#include <iAStackedBarChart.h>    // for add HeaderLabel
#include <iAStringHelper.h>
#include <iAvec3.h>
#include <qthelper/iADockWidgetWrapper.h>
#include <qthelper/iAQTtoUIConnector.h>

// FeatureScout
#include "iACsvVectorTableCreator.h"

// Segmentation
#include "iAVectorTypeImpl.h"
#include "iAVectorDistanceImpl.h"

// FIAKER
#include "iAAlgorithmInfo.h"
#include "iAFiberCharData.h"
#include "iAFiberData.h"
#include "iAMeasureSelectionDlg.h"
#include "iAMultidimensionalScaling.h"
#include "iAParameterInfluenceView.h"
#include "iARefDistCompute.h"
#include "iASensitivityDialog.h"
#include "ui_DissimilarityMatrix.h"
#include "ui_SensitivitySettings.h"

#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariant.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableView>
#include <QtConcurrent>
#include <QTextStream>
#include <QVBoxLayout>

#include <array>
#include <set>

namespace
{
	const int LayoutSpacing = 4;
	const QString DefaultStackedBarColorTheme("Brewer Accent (max. 8)");
	QStringList const& AggregationNames()
	{
		static QStringList Names = QStringList() << "Mean left+right" << "Left only" << "Right only" << "Mean of all neighbours in STAR";
		return Names;
	}

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
	QColor ParamColor(150, 150, 255, 255);
	QColor OutputColor(255, 200, 200, 255);
	}

// Factor out as generic CSV reading class also used by iACsvIO?
bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator,
	iACsvTableCreator& tblCreator, size_t resultCount, int numColumns)
{
	if (!QFile::exists(fileName))
	{
		LOG(lvlError, "Error loading csv file, file does not exist.");
		return false;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Unable to open file '%1': %2").arg(fileName).arg(file.errorString()));
		return false;
	}
	QTextStream in(&file);
	in.setCodec(encoding.toStdString().c_str());
	auto headers = in.readLine().split(columnSeparator);
	tblCreator.initialize(headers, resultCount);
	size_t row = 0;
	while (!in.atEnd() && row < resultCount)
	{
		QString line = in.readLine();
		if (line.trimmed().isEmpty()) // skip empty lines
		{
			continue;
		}
		tblCreator.addRow(row, stringToVector<std::vector<double>, double>(line, columnSeparator, numColumns));
		++row;
	}
	if (!in.atEnd())
	{
		LOG(lvlWarn, "Found additional rows at end...");
		return false;
	}
	return true;
}

using HistogramType = QVector<double>;

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

namespace
{
std::array<iAVec3f, 2> computeFiberBBox(std::vector<iAVec3f> const& points, float radius)
{
	std::array<iAVec3f, 2> result;
	result[0].fill(std::numeric_limits<float>::max());
	result[1].fill(std::numeric_limits<float>::lowest());
	for (int p = 0; p < points.size(); ++p)
	{
		auto const & pt = points[p].data();
		for (int i = 0; i < 3; ++i)
		{
			result[0][i] = std::min(result[0][i], pt[i] - radius);
			result[1][i] = std::max(result[1][i], pt[i] + radius);
		}
	}
	return result;
}
using FiberBBT = std::array<iAVec3f, 2>;

inline bool boundingBoxesIntersect(FiberBBT const& bb1, FiberBBT const& bb2)
{
	const int MinIdx = 0, MaxIdx = 1;
	return
		bb1[MaxIdx].x() > bb2[MinIdx].x() && bb2[MaxIdx].x() > bb1[MinIdx].x() &&
		bb1[MaxIdx].y() > bb2[MinIdx].y() && bb2[MaxIdx].y() > bb1[MinIdx].y() &&
		bb1[MaxIdx].z() > bb2[MinIdx].z() && bb2[MaxIdx].z() > bb1[MinIdx].z();
}

std::vector<int> intersectingBoundingBox(FiberBBT const& fixedFiberBB, std::vector<FiberBBT> const& otherFiberBBs)
{
	std::vector<int> resultIDs;
	for (int r = 0; r < otherFiberBBs.size(); ++r)
	{
		if (boundingBoxesIntersect(fixedFiberBB, otherFiberBBs[r]))
		{
			resultIDs.push_back(r);
		}
	}
	return resultIDs;
}

}

void iASensitivityInfo::abort()
{
	m_aborted = true;
}

QSharedPointer<iASensitivityInfo> iASensitivityInfo::create(QMainWindow* child,
	QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW,
	int histogramBins, QString parameterSetFileName, QVector<int> const & charSelected,
	QVector<int> const & charDiffMeasure, int maxColumns)
{
	if (parameterSetFileName.isEmpty())
	{
		parameterSetFileName = QFileDialog::getOpenFileName(child,
			"Sensitivity: Parameter Sets file", data->folder,
			"Comma-Separated Values (*.csv);;");
	}
	if (parameterSetFileName.isEmpty())
	{
		LOG(lvlInfo, "Empty parameter set filename / aborted.");
		return QSharedPointer<iASensitivityInfo>();
	}
	iACsvVectorTableCreator tblCreator;
	if (!readParameterCSV(parameterSetFileName, "UTF-8", ",", tblCreator, data->result.size(), maxColumns))
	{
		return QSharedPointer<iASensitivityInfo>();
	}
	auto const & paramValues = tblCreator.table();
	auto const& paramNames = tblCreator.header();
	// csv assumed to contain header line (names of parameters), and one row per parameter set;
	// parameter set contains an ID as first column and a filename as last row
	if (paramValues.size() <= 2 || paramValues[0].size() <= 3)
	{
		LOG(lvlError, QString("Invalid parameter set file: expected at least 2 data rows (actual: %1) "
			"and at least 3 columns (ID, filename, and one parameter; actual: %2")
			.arg(paramValues.size() > 0 ? paramValues[0].size() : -1)
			.arg(paramValues.size())
		);
		return QSharedPointer<iASensitivityInfo>();
	}
	// data in m_paramValues is indexed [col(=parameter index)][row(=parameter set index)]
	QSharedPointer<iASensitivityInfo> sensitivity(
		new iASensitivityInfo(data, parameterSetFileName, paramNames, paramValues, child, nextToDW));

	// find min/max, for all columns except ID and filename (maybe we could reuse SPM data ranges here?)
	QVector<double> valueMin(static_cast<int>(paramValues.size() - 2));
	QVector<double> valueMax(static_cast<int>(paramValues.size() - 2));
	//LOG(lvlInfo, QString("Parameter values size: %1x%2").arg(paramValues.size()).arg(paramValues[0].size()));
	for (int p = 1; p < paramValues.size() - 1; ++p)
	{           // - 1 because of skipping ID
		valueMin[p - 1] = *std::min_element(paramValues[p].begin(), paramValues[p].end());
		valueMax[p - 1] = *std::max_element(paramValues[p].begin(), paramValues[p].end());
	}

	// countOfVariedParams = number of parameters for which min != max:
	for (int p = 0; p < valueMin.size(); ++p)
	{
		if (valueMin[p] != valueMax[p])
		{
			sensitivity->m_variedParams.push_back(p + 1); // +1 because valueMin/valueMax don't contain ID
		}
	}
	if (sensitivity->m_variedParams.size() == 0)
	{
		LOG(lvlError, "Invalid sampling: No parameter was varied!");
		return QSharedPointer<iASensitivityInfo>();
	}
	//LOG(lvlInfo, QString("Found the following parameters to vary (number: %1): %2")
	//	.arg(sensitivity->m_variedParams.size())
	//	.arg(joinAsString(sensitivity->m_variedParams, ",", [&paramNames](int const& i) { return paramNames[i]; })));

	// find out how many additional parameter sets were added per STAR:
	//   - go to first value row; take value of first varied parameter as v
	//   - go down rows, as long as either
	//        first varied parameter has same value as v
	//        or distance of current value of first varied parameter is a multiple
	//        of the distance between its first row value and second row value
	double checkValue0 = paramValues[sensitivity->m_variedParams[0]][0];
	const double RemainderCheckEpsilon = 1e-12;
	double curCheckValue = paramValues[sensitivity->m_variedParams[0]][1];
	double diffCheck = std::abs(curCheckValue - checkValue0);
	//LOG(lvlDebug, QString("checkValue0=%1, curCheckValue=%2, diffCheck=%3").arg(checkValue0).arg(curCheckValue).arg(diffCheck));
	double remainder = 0;
	int row = 2;
	while (row < paramValues[sensitivity->m_variedParams[0]].size() &&
		(remainder < RemainderCheckEpsilon || 	// "approximately a multiple" is not so easy with double
			(std::abs(diffCheck - remainder) < RemainderCheckEpsilon) || // remainder could also be close to but smaller than diffCheck
			(dblApproxEqual(curCheckValue, checkValue0))))
	{
		curCheckValue = paramValues[sensitivity->m_variedParams[0]][row];
		remainder = std::abs(std::fmod(std::abs(curCheckValue - checkValue0), diffCheck));
		//LOG(lvlDebug, QString("Row %1: curCheckValue=%2, checkValue0=%3, remainder=%4")
		//	.arg(row).arg(curCheckValue).arg(checkValue0).arg(remainder));
		++row;
	}
	sensitivity->m_starGroupSize = row - 1;
	sensitivity->m_numOfSTARSteps = (sensitivity->m_starGroupSize - 1) / sensitivity->m_variedParams.size();
	//LOG(lvlInfo,QString("Determined that there are groups of size: %1; number of STAR points per parameter: %2")
	//	.arg(sensitivity->m_starGroupSize)
	//	.arg(sensitivity->m_numOfSTARSteps)
	//);

	// select output features to compute sensitivity for:
	// - the loaded and computed ones (length, orientation, ...)
	// - dissimilarity measure(s)
	if (charSelected.isEmpty())
	{
		iASensitivityDialog dlg(data);
		if (dlg.exec() != QDialog::Accepted)
		{
			return QSharedPointer<iASensitivityInfo>();
		}
		sensitivity->m_charSelected = dlg.selectedCharacteristics();
		sensitivity->m_charDiffMeasure = dlg.selectedDiffMeasures();
	}
	else
	{
		sensitivity->m_charSelected = charSelected;
		sensitivity->m_charDiffMeasure = charDiffMeasure;
	}
	//sensitivity->dissimMeasure = dlg.selectedMeasures();
	sensitivity->m_histogramBins = histogramBins;
	if (sensitivity->m_charSelected.size() == 0 || sensitivity->m_charDiffMeasure.size() == 0)
	{
		QMessageBox::warning(child, "Sensitivity", "You have to select at least one characteristic and at least one measure!", QMessageBox::Ok);
		return QSharedPointer<iASensitivityInfo>();
	}
	if (!QFile::exists(sensitivity->dissimilarityMatrixCacheFileName()))
	{
		iAMeasureSelectionDlg selectMeasure;
		if (selectMeasure.exec() != QDialog::Accepted)
		{
			return QSharedPointer<iASensitivityInfo>();
		}
		sensitivity->m_resultDissimMeasures = selectMeasure.measures();
		sensitivity->m_resultDissimOptimMeasureIdx = selectMeasure.optimizeMeasureIdx();
	}
	auto futureWatcher = runAsync([sensitivity]
		{
			sensitivity->compute();
		},
		[sensitivity]
		{
			sensitivity->createGUI();
		});
	iAJobListView::get()->addJob("Sensitivity computation", &sensitivity->m_progress, futureWatcher, sensitivity.data());
	return sensitivity;
}

iASensitivityInfo::iASensitivityInfo(QSharedPointer<iAFiberResultsCollection> data,
	QString const& parameterFileName, QStringList const& paramNames,
	std::vector<std::vector<double>> const & paramValues, QMainWindow* child, QDockWidget* nextToDW) :
	m_data(data),
	m_paramNames(paramNames),
	m_paramValues(paramValues),
	m_parameterFileName(parameterFileName),
	m_child(child),
	m_nextToDW(nextToDW),
	m_aborted(false)
{
}

void getBestMatches2(iAFiberData const& fiber, std::vector<iAFiberData> const& otherFibers,
	QVector<QVector<iAFiberSimilarity>>& bestMatches, std::vector<int> const& candidates,
	double diagonalLength, double maxLength, std::vector<std::pair<int, bool>>& measuresToCompute)
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
		for (iARefDistCompute::ContainerSizeType bestMatchID = 0; bestMatchID < candidates.size(); ++bestMatchID)
		{
			size_t refFiberID = candidates[bestMatchID];
			similarities[bestMatchID].index = static_cast<quint32>(refFiberID);
			double curDissimilarity =
				getDissimilarity(fiber, otherFibers[refFiberID], measuresToCompute[d].first, diagonalLength, maxLength);
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

void iASensitivityInfo::compute()
{
	m_progress.setStatus("Storing parameter set values");
	for (int p = 0; p < m_paramValues[0].size(); p += m_starGroupSize)
	{
		QVector<double> parameterSet;
		for (int v = 0; v < m_paramValues.size(); ++v)
		{
			parameterSet.push_back(m_paramValues[v][p]);
		}
		paramSetValues.push_back(parameterSet);
		m_progress.emitProgress(static_cast<int>(100 * p / m_paramValues[0].size()));
	}

	m_progress.setStatus("Computing characteristics distribution (histogram) for all results.");
	// TODO: common storage for that in data!
	m_charHistograms.resize(static_cast<int>(m_data->result.size()));

	int numCharSelected = m_charSelected.size();
	for (auto selCharIdx = 0; selCharIdx < m_charSelected.size(); ++selCharIdx)
	{
		double rangeMin = m_data->spmData->paramRange(m_charSelected[selCharIdx])[0];
		double rangeMax = m_data->spmData->paramRange(m_charSelected[selCharIdx])[1];
		LOG(lvlInfo, QString("Characteristic idx=%1, charIdx=%2 (%3): %4-%5")
			.arg(selCharIdx).arg(m_charSelected[selCharIdx]).arg(charactName(selCharIdx))
			.arg(rangeMin).arg(rangeMax));
	}

	for (int rIdx = 0; rIdx < m_data->result.size(); ++rIdx)
	{
		m_progress.emitProgress(static_cast<int>(100 * rIdx / m_data->result.size()));
		auto const& r = m_data->result[rIdx];
		// TODO: skip some columns? like ID...
		m_charHistograms[rIdx].reserve(numCharSelected);
		for (auto charIdx: m_charSelected)
		{
			// make sure of all histograms for the same characteristic have the same range
			double rangeMin = m_data->spmData->paramRange(charIdx)[0];
			double rangeMax = m_data->spmData->paramRange(charIdx)[1];
			std::vector<double> fiberData(r.fiberCount);
			for (size_t fiberID = 0; fiberID < r.fiberCount; ++fiberID)
			{
				fiberData[fiberID] = r.table->GetValue(fiberID, charIdx).ToDouble();
			}
			auto histogram = createHistogram(
				fiberData, m_histogramBins, rangeMin, rangeMax);
			m_charHistograms[rIdx].push_back(histogram);
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

	m_progress.setStatus("Computing characteristics sensitivities.");
	const int NumOfVarianceAggregation = 4;

	paramStep.fill(0.0, m_variedParams.size());
	sensitivityField.resize(numCharSelected);
	aggregatedSensitivities.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		m_progress.emitProgress(100 * charIdx / numCharSelected);
		//int charactID = m_charSelected[charIdx];
		//auto charactName = m_data->spmData->parameterName(charactID);
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
						LOG(lvlInfo, QString("Param Step (param %1): %2").arg(paramIdx).arg(paramStep[paramIdx]));
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
						int compareIdx = (i==0)? resultIdxGroupStart : (resultIdxParamStart + i - 1);
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

	m_progress.setStatus("Computing fiber count sensitivities.");

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
		m_progress.emitProgress(100 * paramIdx / m_variedParams.size());
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

			if (paramStep[paramIdx] == 0)
			{
				paramStep[paramIdx] = std::abs(paramDiff);
			}

			double leftVar = 0;
			int numLeftRight = 0;
			if (paramDiff > 0)
			{
				leftVar = std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount)
					- m_data->result[resultIdxParamStart].fiberCount);
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
				rightVar = std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount)
					- m_data->result[firstPosStepIdx].fiberCount);
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
				double difference = std::abs(static_cast<double>(m_data->result[compareIdx].fiberCount)
					- m_data->result[resultIdxParamStart + i].fiberCount);
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

	m_progress.setStatus("Compute variation histogram");
	//charHistHist.resize(numCharSelected);
	charHistVar.resize(numCharSelected);
	charHistVarAgg.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		m_progress.emitProgress(100 * charIdx / numCharSelected);
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
							std::abs(m_charHistograms[compareIdx][charIdx][bin] - m_charHistograms[resultIdxParamStart + i][charIdx][bin]);
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

	m_progress.setStatus("Loading cached dissimilarities between all result pairs.");
	m_progress.emitProgress(0);
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
		m_progress.setStatus("Computing dissimilarities between all result pairs.");
		int measureCount = static_cast<int>(m_resultDissimMeasures.size());
		int resultCount = static_cast<int>(m_data->result.size());
		m_resultDissimMatrix = iADissimilarityMatrixType(resultCount,
			QVector<iAResultPairInfo>(resultCount, iAResultPairInfo(measureCount)));

		for (size_t m = 0; m < m_resultDissimMeasures.size(); ++m)
		{
			measures.push_back(m_resultDissimMeasures[m].first);
		}

		//m_progress.setStatus("Creating fiber data objects.");
		std::vector<std::vector<iAFiberData>> resFib(resultCount);
		for (int resultID = 0; resultID < resultCount && !m_aborted; ++resultID)
		{
			auto& result = m_data->result[resultID];
			auto const& mapping = *result.mapping.data();
			std::vector<iAFiberData> fiberData(result.fiberCount);
			for (size_t fiberID=0; fiberID < result.fiberCount; ++fiberID)
			{
				auto it = result.curveInfo.find(fiberID);
				fiberData[fiberID] = iAFiberData(result.table, fiberID, mapping,
					(it != result.curveInfo.end()) ? it->second : std::vector<iAVec3f>());
			}
			resFib[resultID] = fiberData;
		}

		//m_progress.setStatus("Computing bounding boxes for all fibers.");
		std::vector<std::vector<FiberBBT>> fiberBoundingBox(resultCount);
		for (int resultID = 0; resultID < resultCount && !m_aborted; ++resultID)
		{
			std::vector<FiberBBT> fibersBBox(m_data->result[resultID].fiberCount);
			for (size_t fiberID = 0; fiberID < fibersBBox.size(); ++fiberID)
			{
				fibersBBox[fiberID] = computeFiberBBox(
					resFib[resultID][fiberID].curvedPoints.size() > 0 ?
						resFib[resultID][fiberID].curvedPoints:
						resFib[resultID][fiberID].pts,
					resFib[resultID][fiberID].diameter / 2.0);
			}
			fiberBoundingBox[resultID] = fibersBBox;
		}

		m_progress.setStatus("Computing dissimilarity between all result pairs.");
		// fill "upper" half
		double overallPairs = resultCount * (resultCount - 1) / 2;
		size_t curCheckedPairs = 0;
		for (int r1 = 0; r1 < resultCount - 1 && !m_aborted; ++r1)
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
			for (int r2 = r1 + 1; r2 < resultCount && !m_aborted; ++r2)
			{
				auto& mat = m_resultDissimMatrix[r1][r2];
				m_progress.setStatus(QString("Computing dissimilarity between results %1 and %2.").arg(r1).arg(r2));
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] = 0;
				}
				int r2FibCount = static_cast<int>(m_data->result[r2].fiberCount);
				auto& dissimilarities = mat.fiberDissim;
				dissimilarities.resize(r2FibCount);
				// not ideal: for loop seems to be not ideally parallelizable,
				// one spike where 100% is used, then going down to nearly 0, until next loop starts
				int noCanDo = 0;
				size_t candSum = 0;
#pragma omp parallel for reduction(+ : noCanDo, candSum)
				for (int fiberID = 0; fiberID < r2FibCount; ++fiberID)
				{
					auto candidates =
						intersectingBoundingBox(fiberBoundingBox[r2][fiberID], fiberBoundingBox[r1]);
					if (candidates.size() == 0)
					{
						++noCanDo; // thread-safe
					}
					candSum += candidates.size();
					getBestMatches2(resFib[r2][fiberID], resFib[r1], dissimilarities[fiberID],
						candidates, diagonalLength, maxLength, m_resultDissimMeasures);
					for (int m = 0; m < measureCount; ++m)
					{
						mat.avgDissim[m] += dissimilarities[fiberID][m][0].dissimilarity;
					}
				}
				LOG(lvlInfo, QString("Result %1x%2: %3 candidates on average, %4 with no bounding box intersections out of %5")
					.arg(r1).arg(r2).arg(candSum / r2FibCount).arg(noCanDo).arg(r2FibCount));
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] /= r2FibCount;
				}
				++curCheckedPairs;
				m_progress.emitProgress(curCheckedPairs * 100.0 / overallPairs);
			}
		}
		// fill diagonal with 0
		for (int r = 0; r < resultCount && !m_aborted; ++r)
		{
			for (int m = 0; m < measureCount; ++m)
			{
				m_resultDissimMatrix[r][r].avgDissim[m] = 0;
			}
		}
		// copy other half triangle:
		for (int r1 = 1; r1 < resultCount && !m_aborted; ++r1)
		{
			for (int r2 = 0; r2 < r1; ++r2)
			{
				for (int m = 0; m < measureCount; ++m)
				{
					m_resultDissimMatrix[r1][r2].avgDissim[m] = m_resultDissimMatrix[r2][r1].avgDissim[m];
				}
			}
		}
		if (m_aborted)
		{
			return;
		}
		writeDissimilarityMatrixCache(measures);
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

	int measureCount = static_cast<int>(m_resultDissimMeasures.size());
		// TODO: unify with other loops over STARs
	sensDissimField.resize(measureCount);
	aggregatedSensDissim.resize(measureCount);

	for (int m = 0; m < measureCount && !m_aborted; ++m)
	{
		sensDissimField[m].resize(NumOfVarianceAggregation);
		aggregatedSensDissim[m].resize(NumOfVarianceAggregation);
		for (int a = 0; a < NumOfVarianceAggregation; ++a)
		{
			sensDissimField[m][a].resize(m_variedParams.size());
			aggregatedSensDissim[m][a].fill(0.0, m_variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
		{
			m_progress.emitProgress(100 * paramIdx / m_variedParams.size());
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
	}
}

QString iASensitivityInfo::dissimilarityMatrixCacheFileName() const
{
	return m_data->folder + "/cache/dissimilarityMatrix.cache";
}

namespace
{
	const QString DissimilarityMatrixCacheFileIdentifier("DissimilarityMatrixCache");
	const quint32 DissimilarityMatrixCacheFileVersion(1);
}

bool iASensitivityInfo::readDissimilarityMatrixCache(QVector<int>& measures)
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
		LOG(lvlError, QString("FIAKER cache file '%1': Unknown cache file format - found identifier %2 does not match expected identifier %3. Please delete file and let it be recreated!")
			.arg(cacheFile.fileName())
			.arg(identifier).arg(DissimilarityMatrixCacheFileIdentifier));
		return false;
	}
	quint32 version;
	in >> version;
	if (version > DissimilarityMatrixCacheFileVersion)
	{
		LOG(lvlError, QString("FIAKER cache file '%1': Invalid or too high version number (%2), expected %3 or less. Please delete file and let it be recreated!")
			.arg(cacheFile.fileName())
			.arg(version).arg(DissimilarityMatrixCacheFileVersion));
		return false;
	}
	in >> measures;
	in >> m_resultDissimMatrix;
	cacheFile.close();
	return true;
}

void iASensitivityInfo::writeDissimilarityMatrixCache(QVector<int> const& measures) const
{
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

typedef iAQTtoUIConnector<QWidget, Ui_SensitivitySettings> iASensitivitySettingsUI;

class iASensitivitySettingsView: public iASensitivitySettingsUI
{
public:
	iASensitivitySettingsView(iASensitivityInfo* sensInf)
	{
		cmbboxMeasure->addItems(DistributionDifferenceMeasureNames());
		cmbboxAggregation->addItems(AggregationNames());
		QStringList characteristics;
		for (int charIdx = 0; charIdx < sensInf->m_charSelected.size(); ++charIdx)
		{
			characteristics << sensInf->charactName(charIdx);
		}
		cmbboxCharacteristic->addItems(characteristics);

		QStringList dissimilarities;
		for (auto dissim : sensInf->m_resultDissimMeasures)
		{
			dissimilarities << getAvailableDissimilarityMeasureNames()[dissim.first];
		}
		cmbboxDissimilarity->addItems(dissimilarities);

		connect(cmbboxMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::changeMeasure);
		connect(cmbboxAggregation, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::changeAggregation);

		connect(cmbboxAggregation, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxCharacteristic, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxOutput, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxOutput, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::updateOutputControls);

		connect(cmbboxDissimilarity, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::updateDissimilarity);
		
		cmbboxAggregation->setMinimumWidth(80);
		cmbboxMeasure->setMinimumWidth(80);
		cmbboxDissimilarity->setMinimumWidth(80);
		cmbboxMeasure->setMinimumWidth(80);
		cmbboxOutput->setMinimumWidth(80);
	}
	int charIdx() const
	{
		return cmbboxCharacteristic->currentIndex();
	}
	int outputIdx() const
	{
		return cmbboxOutput->currentIndex();
	}
	int dissimMeasIdx() const
	{
		return cmbboxDissimilarity->currentIndex();
	}
};


// TODO: needed? remove!
class iAParameterListView : public QWidget
{
public:
	iAParameterListView(QStringList const& paramNames,
		std::vector<std::vector<double>> const& paramValues,
		QVector<int> variedParams,
		iADissimilarityMatrixType const& dissimMatrix)
	{
		auto paramScrollArea = new QScrollArea();
		paramScrollArea->setWidgetResizable(true);
		auto paramList = new QWidget();
		paramScrollArea->setWidget(paramList);
		paramScrollArea->setContentsMargins(0, 0, 0, 0);
		auto paramListLayout = new QGridLayout();
		paramListLayout->setSpacing(LayoutSpacing);
		paramListLayout->setContentsMargins(1, 0, 1, 0);
		paramListLayout->setColumnStretch(0, 1);
		paramListLayout->setColumnStretch(1, 2);

		enum ParamColumns
		{
			NameCol = 0,
			MatrixCol
		};
		addHeaderLabel(paramListLayout, NameCol, "Parameter");
		addHeaderLabel(paramListLayout, MatrixCol, "Sensitivity Matrix");
		for (auto p : variedParams)
		{
			auto paramNameLabel = new QLabel(paramNames[p]);
			paramNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			paramListLayout->addWidget(paramNameLabel, p + 1, NameCol);

			auto paramMatrix = new iAMatrixWidget(dissimMatrix, paramValues, false, false);
			paramMatrix->setSortParameter(p);
			paramMatrix->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			paramMatrix->setData(0);
			paramMatrix->setLookupTable(iALUT::Build(paramMatrix->range(), iALUT::GetColorMapNames()[0], 255, 255));
			m_matrixPerParam.push_back(paramMatrix);
			paramListLayout->addWidget(paramMatrix, p + 1, MatrixCol);
		}
		setLayout(paramListLayout);
	}

	void dissimMatrixMeasureChanged(int idx)
	{
		for (auto paramMatrix : m_matrixPerParam)
		{
			paramMatrix->setData(idx);
			paramMatrix->update();
		}
	}

	void dissimMatrixColorMapChanged(int idx)
	{
		for (auto paramMatrix : m_matrixPerParam)
		{
			paramMatrix->setLookupTable(iALUT::Build(paramMatrix->range(), iALUT::GetColorMapNames()[idx], 255, 255));
			paramMatrix->update();
		}
	}
private:
	std::vector<iAMatrixWidget*> m_matrixPerParam;
};


class iASensitivityGUI
{
public:

	//! @{ Param Influence List
	iAParameterInfluenceView* m_paramInfluenceView;
	//! @}

	//! Overall settings
	iASensitivitySettingsView* m_settings;

	//! Parameter detail
	QCustomPlot* m_paramDetails;

	//! scatter plot for the MDS plot of all results
	iAScatterPlotWidget* m_scatterPlot;
	//! lookup table for points in scatter plot
	QSharedPointer<iALookupTable> m_lut;

	iADockWidgetWrapper* m_dwParamInfluence;

	QSharedPointer<iASPLOMData> m_mdsData;

	iADissimilarityMatrixType m_dissimilarityMatrix;
	iAMatrixWidget* m_matrixWidget;
	iAParameterListView* m_parameterListView;
	iAAlgorithmInfo* m_algoInfo;

	void updateScatterPlotLUT(int starGroupSize, int numOfSTARSteps, size_t resultCount,
		QVector<int> const & variedParams)
	{
		//LOG(lvlDebug, "\nNEW LUT:");
		std::set<int> hiGrp;
		std::set<std::pair<int, int> > hiGrpParam;
		std::set<int> hiGrpAll;
		auto const& hp = m_scatterPlot->highlightedPoints();
		for (auto ptIdx : hp)
		{
			int groupID = ptIdx / starGroupSize;
			if (ptIdx % starGroupSize == 0)
			{
				//LOG(lvlDebug, QString("Selected GROUP: %1").arg(groupID));
				hiGrp.insert(groupID);
			}
			else
			{
				int paramID = ((ptIdx % starGroupSize) - 1) / numOfSTARSteps;
				//LOG(lvlDebug, QString("Selected PARAM: %1, %2").arg(groupID).arg(paramID));
				hiGrpParam.insert(std::make_pair(groupID, paramID));
			}
			hiGrpAll.insert(groupID);
		}
		for (size_t i = 0; i < resultCount; ++i)
		{
			int groupID = static_cast<int>(i / starGroupSize);
			bool highlightGroup = hiGrp.find(groupID) != hiGrp.end();
			QColor c;
			if (i % starGroupSize == 0)
			{
				highlightGroup = highlightGroup ||
					// highlight center also if any of its param subgroups is highlighted:
					std::find_if(hiGrpParam.begin(), hiGrpParam.end(), [groupID](std::pair<int, int> a)
					{
						return a.first == groupID;
					}) != hiGrpParam.end();
				c = QColor(0, 0, highlightGroup ? 255: 192, highlightGroup ? 255 : 128);
				//LOG(lvlDebug, QString("Point %1 (group=%2) : Color=%3, %4, %5, %6")
				//	.arg(i).arg(groupID).arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()));
			}
			else
			{
				int paramID = ((i % starGroupSize) - 1) / numOfSTARSteps;
				bool highlightParam = hiGrpParam.find(std::make_pair(groupID, paramID)) != hiGrpParam.end();
				int colVal = (highlightParam || highlightGroup) ? 128 : 192;
				int redVal = 192;
				c = QColor(redVal, colVal, colVal, highlightGroup ? 192 : 128);
				//LOG(lvlDebug, QString("Point %1 (group=%2, paramID=%3) : Color=%4, %5, %6, %7")
				//	.arg(i).arg(groupID).arg(paramID).arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()));
			}
			m_lut->setColor(i, c);
		}
		m_scatterPlot->setLookupTable(m_lut, m_mdsData->numParams() - 1);

		m_scatterPlot->clearLines();
		// we want to build a separate line for each parameter (i.e. in each branch "direction" of the STAR
		// easiest way is to collect all parameter values in a group (done in the vector of size_t/double pairs),
		// then sort this by the parameter values (since we don't know else how many are smaller or larger than
		// the center value), then take the point indices from this ordered vector to form the line.
		for (auto groupID : hiGrpAll)
		{
			auto groupStart = groupID * starGroupSize;
			for (int parIdx = 0; parIdx < variedParams.size(); ++parIdx)
			{
				using PtData = std::pair<size_t, double>;
				std::vector<PtData> linePtParVal;
				double centerValue = m_mdsData->paramData(parIdx)[groupStart];
				linePtParVal.push_back(std::make_pair(groupStart, centerValue));
				auto paraStart = groupStart + 1 + parIdx * numOfSTARSteps;
				for (int inParaIdx = 0; inParaIdx < numOfSTARSteps; ++inParaIdx)
				{
					size_t ptIdx = paraStart + inParaIdx;
					double paramValue = m_mdsData->paramData(parIdx)[ptIdx];
					linePtParVal.push_back(std::make_pair(ptIdx, paramValue));
				}
				std::sort(linePtParVal.begin(), linePtParVal.end(), [](PtData const& a, PtData const& b)
					{
						return a.second < b.second;
					});
				std::vector<size_t> linePoints(linePtParVal.size());
				for (size_t i = 0; i < linePoints.size(); ++i)
				{
					linePoints[i] = linePtParVal[i].first;
				}
				m_scatterPlot->addLine(linePoints);
			}
		}
	}
};

using iADissimilarityMatrixDockContent = iAQTtoUIConnector<QWidget, Ui_DissimilarityMatrix>;

QWidget* iASensitivityInfo::setupMatrixView(QVector<int> const& measures)
{
	iADissimilarityMatrixDockContent* dissimDockContent = new iADissimilarityMatrixDockContent();
	auto measureNames = getAvailableDissimilarityMeasureNames();
	QStringList computedMeasureNames;
	for (int m = 0; m < measures.size(); ++m)
	{
		computedMeasureNames << measureNames[measures[m]];
	}
	dissimDockContent->cbMeasure->addItems(computedMeasureNames);
	dissimDockContent->cbParameter->addItems(m_paramNames);
	dissimDockContent->cbColorMap->addItems(iALUT::GetColorMapNames());
	connect(dissimDockContent->cbMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::dissimMatrixMeasureChanged);
	connect(dissimDockContent->cbParameter, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::dissimMatrixParameterChanged);
	connect(dissimDockContent->cbColorMap, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::dissimMatrixColorMapChanged);                          // showAxes currently buggy - crashes!
	m_gui->m_matrixWidget = new iAMatrixWidget(m_resultDissimMatrix, m_paramValues, true, false);
	m_gui->m_matrixWidget->setSortParameter(0);
	m_gui->m_matrixWidget->setData(0);
	m_gui->m_matrixWidget->setLookupTable(iALUT::Build(m_gui->m_matrixWidget->range(), iALUT::GetColorMapNames()[0], 255, 255));
	dissimDockContent->matrix->layout()->addWidget(m_gui->m_matrixWidget);
	return dissimDockContent;
}

void iASensitivityInfo::dissimMatrixMeasureChanged(int idx)
{
	m_gui->m_matrixWidget->setData(idx);
	m_gui->m_matrixWidget->update();
	m_gui->m_parameterListView->dissimMatrixMeasureChanged(idx);
}

void iASensitivityInfo::dissimMatrixParameterChanged(int idx)
{
	m_gui->m_matrixWidget->setSortParameter(idx);
	m_gui->m_matrixWidget->update();
}

void iASensitivityInfo::dissimMatrixColorMapChanged(int idx)
{
	m_gui->m_matrixWidget->setLookupTable(iALUT::Build(m_gui->m_matrixWidget->range(), iALUT::GetColorMapNames()[idx], 255, 255));
	m_gui->m_matrixWidget->update();
	m_gui->m_parameterListView->dissimMatrixColorMapChanged(idx);
}

QString iASensitivityInfo::charactName(int selCharIdx) const
{
	return m_data->spmData->parameterName(m_charSelected[selCharIdx])
		.replace("[µm]", "")
		.replace("[µm²]", "")
		.replace("[µm³]", "")
		.replace("[°]", "");
}

namespace
{
	const QString ProjectParameterFile("ParameterSetsFile");
	const QString ProjectCharacteristics("Characteristics");
	const QString ProjectCharDiffMeasures("CharacteristicDifferenceMeasures");
	const QString ProjectMaxParameterCSVColumns("MaxParameterCSVColumns");
	//const QString ProjectResultDissimilarityMeasure("ResultDissimilarityMeasures");
}

void iASensitivityInfo::saveProject(QSettings& projectFile, QString  const& fileName)
{
	projectFile.setValue(ProjectParameterFile, MakeRelative(QFileInfo(fileName).absolutePath(), m_parameterFileName));
	projectFile.setValue(ProjectCharacteristics, joinNumbersAsString(m_charSelected, ","));
	projectFile.setValue(ProjectCharDiffMeasures, joinNumbersAsString(m_charDiffMeasure, ","));
	// stored in cache file:
	//projectFile.setValue(ProjectResultDissimilarityMeasure, joinAsString(m_resultDissimMeasures, ",",
	//	[](std::pair<int, bool> const& a) {return QString::number(a.first)+":"+(a.second?"true":"false"); }));
}

bool iASensitivityInfo::hasData(iASettings const& settings)
{
	return settings.contains(ProjectParameterFile);
}


QSharedPointer<iASensitivityInfo> iASensitivityInfo::load(QMainWindow* child,
	QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW,
	int histogramBins, iASettings const & projectFile,
	QString const& projectFileName)
{
	QString parameterSetFileName = MakeAbsolute(QFileInfo(projectFileName).absolutePath(), projectFile.value(ProjectParameterFile).toString());
	QVector<int> charsSelected = stringToVector<QVector<int>, int>(projectFile.value(ProjectCharacteristics).toString());
	QVector<int> charDiffMeasure = stringToVector<QVector<int>, int>(projectFile.value(ProjectCharDiffMeasures).toString());
	int maxColumns = projectFile.value(ProjectMaxParameterCSVColumns, std::numeric_limits<int>::max()).toInt();
	return iASensitivityInfo::create(child, data, nextToDW, histogramBins, parameterSetFileName, charsSelected, charDiffMeasure, maxColumns);
}

class iASPParamPointInfo: public iAScatterPlotPointInfo
{
public:
	iASPParamPointInfo(iASensitivityInfo const & data, iAFiberResultsCollection const & results) :
		m_data(data),
		m_results(results)
	{}
	QString text(const size_t paramIdx[2], size_t pointIdx) override
	{
		Q_UNUSED(paramIdx);
		QString result(QString("Fiber Count: %1<br/>").arg(m_results.result[pointIdx].fiberCount));
		for (int i = 0; i < m_data.m_variedParams.size(); ++i)
		{
			result +=
				m_data.m_paramNames[m_data.m_variedParams[i]] + ": " +
				QString::number(m_data.m_paramValues[m_data.m_variedParams[i]][pointIdx], 'f', 3) + "<br/>";
				/*
				m_data->parameterName(paramIdx[0]) + ": " +
				QString::number(m_data->paramData(paramIdx[0])[pointIdx]) + "<br>" +
				m_data->parameterName(paramIdx[1]) + ": " +
				QString::number(m_data->paramData(paramIdx[1])[pointIdx])
				*/
		}
		return result;
	}
private:
	iASensitivityInfo const & m_data;
	iAFiberResultsCollection const& m_results;
};

void iASensitivityInfo::createGUI()
{
	if (m_aborted)
	{
		emit aborted();
		return;
	}
	m_gui.reset(new iASensitivityGUI);

	m_gui->m_settings = new iASensitivitySettingsView(this);
	auto dwSettings = new iADockWidgetWrapper(m_gui->m_settings, "Sensitivity Settings", "foeSensitivitySettings");
	m_child->splitDockWidget(m_nextToDW, dwSettings, Qt::Horizontal);

	m_gui->m_paramInfluenceView = new iAParameterInfluenceView(this, ParamColor, OutputColor);
	m_gui->m_dwParamInfluence = new iADockWidgetWrapper(m_gui->m_paramInfluenceView, "Parameter Influence", "foeParamInfluence");
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::parameterChanged, this, &iASensitivityInfo::paramChanged);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::outputSelected, this,
		&iASensitivityInfo::outputChanged);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::barAdded, this, &iASensitivityInfo::outputBarAdded);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::barRemoved, this, &iASensitivityInfo::outputBarRemoved);
	connect(m_gui->m_settings->cmbboxCharacteristic, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::characteristicChanged);
	m_child->splitDockWidget(dwSettings, m_gui->m_dwParamInfluence, Qt::Vertical);

	m_gui->m_paramDetails = new QCustomPlot(m_child);
	auto dwParamDetails = new iADockWidgetWrapper(m_gui->m_paramDetails, "Parameter Details", "foeParamDetails");
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwParamDetails, Qt::Vertical);

	QStringList algoInNames;
	for (auto p : m_variedParams)
	{
		algoInNames.push_back(m_paramNames[p]);
	}
	QStringList algoOutNames;
	for (int charIdx = 0; charIdx < m_charSelected.size(); ++charIdx)
	{
		algoOutNames << charactName(charIdx);
	}
	m_gui->m_algoInfo = new iAAlgorithmInfo("Fiber Reconstruction", algoInNames, algoOutNames, ParamColor, OutputColor);
	m_gui->m_algoInfo->addShownOut(0); // equivalent to addStackedBar in iAParameterInfluenceView constructor!
	connect(m_gui->m_algoInfo, &iAAlgorithmInfo::inputClicked,
		m_gui->m_paramInfluenceView, &iAParameterInfluenceView::setSelectedParam);
	connect(m_gui->m_algoInfo, &iAAlgorithmInfo::outputClicked,
		m_gui->m_paramInfluenceView, &iAParameterInfluenceView::toggleCharacteristic);
	auto dwAlgoInfo = new iADockWidgetWrapper(m_gui->m_algoInfo, "Algorithm Details", "foeAlgorithmInfo");
	m_child->splitDockWidget(dwSettings, dwAlgoInfo, Qt::Horizontal);

	QVector<int> measures;
	for (auto d : m_resultDissimMeasures)
	{
		measures.push_back(d.first);
	}
	QWidget* dissimDockContent = setupMatrixView(measures);
	auto dwDissimMatrix = new iADockWidgetWrapper(dissimDockContent, "Dissimilarity Matrix", "foeMatrix");
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwDissimMatrix, Qt::Vertical);

	m_gui->m_parameterListView = new iAParameterListView(m_paramNames, m_paramValues, m_variedParams, m_resultDissimMatrix);
	auto dwParamView = new iADockWidgetWrapper(m_gui->m_parameterListView, "Parameter View", "foeParameters");
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwParamView, Qt::Vertical);

	m_gui->m_mdsData = QSharedPointer<iASPLOMData>(new iASPLOMData());
	std::vector<QString> spParamNames;
	for (auto p: m_variedParams)
	{
		spParamNames.push_back(m_paramNames[p]);
	}
	spParamNames.push_back("MDS X");
	spParamNames.push_back("MDS Y");
	spParamNames.push_back("ID");
	size_t resultCount = m_data->result.size();
	m_gui->m_mdsData->setParameterNames(spParamNames, resultCount);
	for (int c = 0; c < spParamNames.size(); ++c)
	{
		m_gui->m_mdsData->data()[c].resize(resultCount);
	}
	for (int p=0; p<m_variedParams.size(); ++p)
	{	// set parameter values:
		int origParamIdx = m_variedParams[p];
		for (int r = 0; r < resultCount; ++r)
		{
			m_gui->m_mdsData->data()[p][r] = m_paramValues[origParamIdx][r];
		}
	}
	for (size_t i = 0; i < resultCount; ++i)
	{
		m_gui->m_mdsData->data()[spParamNames.size() - 3][i] = 0.0;  // MDS X
		m_gui->m_mdsData->data()[spParamNames.size() - 2][i] = 0.0;  // MDS Y
		m_gui->m_mdsData->data()[spParamNames.size() - 1][i] = i;    // ID
	}
	m_gui->m_mdsData->updateRanges();
	m_gui->m_scatterPlot = new iAScatterPlotWidget(m_gui->m_mdsData, true);
	m_gui->m_scatterPlot->setPointRadius(5);
	m_gui->m_scatterPlot->setFixPointsEnabled(true);
	m_gui->m_lut.reset(new iALookupTable());
	m_gui->m_lut->setRange(0, m_data->result.size());
	m_gui->m_lut->allocate(m_data->result.size());
	m_gui->updateScatterPlotLUT(m_starGroupSize, m_numOfSTARSteps, m_data->result.size(), m_variedParams);
	m_gui->m_scatterPlot->setPointInfo(QSharedPointer<iAScatterPlotPointInfo>(new iASPParamPointInfo(*this, *m_data.data())));
	auto dwScatterPlot = new iADockWidgetWrapper(m_gui->m_scatterPlot, "Results Overview", "foeScatterPlot");
	connect(m_gui->m_scatterPlot, &iAScatterPlotWidget::pointHighlighted, this, &iASensitivityInfo::spPointHighlighted);
	connect(m_gui->m_scatterPlot, &iAScatterPlotWidget::highlightChanged, this, &iASensitivityInfo::spHighlightChanged);
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwScatterPlot, Qt::Vertical);

	updateDissimilarity();
}

void iASensitivityInfo::changeMeasure(int newMeasure)
{
	m_gui->m_paramInfluenceView->setMeasure(newMeasure);
}

void iASensitivityInfo::changeAggregation(int newAggregation)
{
	m_gui->m_paramInfluenceView->setAggregation(newAggregation);
}

void iASensitivityInfo::paramChanged()
{
	int outType = m_gui->m_settings->outputIdx();
	int paramIdx = m_gui->m_paramInfluenceView->selectedRow();
	m_gui->m_algoInfo->setSelectedInput(paramIdx);
	if (paramIdx == -1)
	{
		LOG(lvlInfo, "No parameter selected.");
		return;
	}
	int selCharIdx = m_gui->m_settings->charIdx();
	int measureIdx = m_gui->m_paramInfluenceView->selectedMeasure();
	int aggrType = m_gui->m_paramInfluenceView->selectedAggrType();
	int dissimMeasIdx = m_gui->m_settings->dissimMeasIdx();

	m_gui->m_dwParamInfluence->setWindowTitle("Parameter Influence (by " + DistributionDifferenceMeasureNames()[measureIdx] + ")");

	auto& plot = m_gui->m_paramDetails;
	plot->clearGraphs();
	plot->addGraph();
	plot->graph(0)->setPen(QPen(Qt::blue));

	auto const& data = (
		(outType == outCharacteristic) ?  sensitivityField[selCharIdx][measureIdx][aggrType]
		  : (outType == outFiberCount) ? sensitivityFiberCount[aggrType]
	  /* (outType == outDissimilarity)*/: sensDissimField[dissimMeasIdx][aggrType])[paramIdx];
	QVector<double> x(data.size()), y(data.size());
	for (int i = 0; i < data.size(); ++i)
	{
		x[i] = paramSetValues[i][m_variedParams[paramIdx]];
		y[i] = data[i];
	}
	// configure right and top axis to show ticks but no labels:
	// (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
	plot->xAxis2->setVisible(true);
	plot->xAxis2->setTickLabels(false);
	plot->yAxis2->setVisible(true);
	plot->yAxis2->setTickLabels(false);
	plot->xAxis->setLabel(m_paramNames[m_variedParams[paramIdx]]);
	plot->yAxis->setLabel( ((outType == outCharacteristic) ?
		"Sensitivity " + (charactName(selCharIdx) + " (" + DistributionDifferenceMeasureNames()[measureIdx]+") ") :
		"Fiber Count ") + AggregationNames()[aggrType]  );
	// make left and bottom axes always transfer their ranges to right and top axes:
	connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
	connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));
	// pass data points to graphs:
	plot->graph(0)->setData(x, y);
	// let the ranges scale themselves so graph 0 fits perfectly in the visible area:
	plot->graph(0)->rescaleAxes();
	plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
	plot->replot();

	//m_gui->m_paramInfluenceView->showDifferenceDistribution(outputIdx, selCharIdx, aggrType);
}

void iASensitivityInfo::outputChanged(int outType, int outIdx)
{
	QSignalBlocker block1(m_gui->m_settings->cmbboxOutput);
	m_gui->m_settings->cmbboxOutput->setCurrentIndex(outType);
	if (outType == outCharacteristic)
	{
		QSignalBlocker block2(m_gui->m_settings->cmbboxCharacteristic);
		m_gui->m_settings->cmbboxCharacteristic->setCurrentIndex(outIdx);
	}
	else if (outType == outDissimilarity)
	{
		QSignalBlocker block2(m_gui->m_settings->cmbboxDissimilarity);
		m_gui->m_settings->cmbboxDissimilarity->setCurrentIndex(outIdx);
	}
	paramChanged();
}

void iASensitivityInfo::characteristicChanged(int charIdx)
{
	assert(m_gui->m_settings->cmbboxOutput->currentIndex() == outCharacteristic);
	m_gui->m_paramInfluenceView->selectStackedBar(outCharacteristic, charIdx);
}

void iASensitivityInfo::outputBarAdded(int outType, int outIdx)
{
	if (outType == outCharacteristic)
	{
		m_gui->m_algoInfo->addShownOut(outIdx);
		m_gui->m_algoInfo->update();
	}
}
void iASensitivityInfo::outputBarRemoved(int outType, int outIdx)
{
	if (outType == outCharacteristic)
	{
		m_gui->m_algoInfo->removeShownOut(outIdx);
		m_gui->m_algoInfo->update();
	}
}

void iASensitivityInfo::updateOutputControls()
{
	bool characteristics = m_gui->m_settings->cmbboxOutput->currentIndex() == 0;
	m_gui->m_settings->lbCharacteristic->setEnabled(characteristics);
	m_gui->m_settings->cmbboxCharacteristic->setEnabled(characteristics);
	m_gui->m_settings->lbMeasure->setEnabled(characteristics);
	m_gui->m_settings->cmbboxMeasure->setEnabled(characteristics);
}

void iASensitivityInfo::updateDissimilarity()
{
	int dissimIdx = m_gui->m_settings->cmbboxDissimilarity->currentIndex();
	iAMatrixType distMatrix(m_data->result.size(), std::vector<double>(m_data->result.size()));
	//LOG(lvlDebug, "Distance Matrix:");
	for (int r1 = 0; r1 < distMatrix.size(); ++r1)
	{
		QString line;
		for (int r2 = 0; r2 < distMatrix.size(); ++r2)
		{
			distMatrix[r1][r2] = m_resultDissimMatrix[r1][r2].avgDissim[dissimIdx];
			line += " " + QString::number(distMatrix[r1][r2], 'f', 2).rightJustified(5);
		}
		//LOG(lvlDebug, QString("%1:%2").arg(r1).arg(line));
	}
	auto mds = computeMDS(distMatrix, 2, 100);
	//LOG(lvlDebug, "MDS:");
	for (int i = 0; i < mds.size(); ++i)
	{
		for (int c = 0; c < mds[0].size(); ++c)
		{
			m_gui->m_mdsData->data()[m_gui->m_mdsData->numParams() - 3 + c][i] = mds[i][c];
		}
		//LOG(lvlDebug, QString("%1: %2, %3").arg(i).arg(mds[i][0]).arg(mds[i][1]));
	}
	m_gui->m_mdsData->updateRanges();
}

void iASensitivityInfo::spPointHighlighted(size_t resultIdx, bool state)
{
	int paramID = -1;
	if (state && resultIdx % m_starGroupSize != 0)
	{	// specific parameter "branch" selected:
		paramID = ((resultIdx % m_starGroupSize) - 1) / m_numOfSTARSteps;
	}
	m_gui->m_paramInfluenceView->setSelectedParam(paramID);
	emit resultSelected(resultIdx, state);
}

void iASensitivityInfo::spHighlightChanged()
{
	m_gui->updateScatterPlotLUT(m_starGroupSize, m_numOfSTARSteps, m_data->result.size(), m_variedParams);
}