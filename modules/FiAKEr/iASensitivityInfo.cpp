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
#include <qthelper/iADockWidgetWrapper.h>
#include <qthelper/iAQTtoUIConnector.h>

// FeatureScout
#include "iACsvVectorTableCreator.h"

// Segmentation
#include "iAVectorTypeImpl.h"
#include "iAVectorDistanceImpl.h"

// FIAKER
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
}

// Factor out as generic CSV reading class also used by iACsvIO?
bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator, iACsvTableCreator& tblCreator, size_t resultCount)
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
		auto values = line.split(columnSeparator);
		tblCreator.addRow(row, values);
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

void iASensitivityInfo::abort()
{
	m_aborted = true;
}

QSharedPointer<iASensitivityInfo> iASensitivityInfo::create(QMainWindow* child,
	QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW,
	iAJobListView* jobListView, int histogramBins,
	QString parameterSetFileName, QVector<int> const & charSelected,
	QVector<int> const & charDiffMeasure)
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
	if (!readParameterCSV(parameterSetFileName, "UTF-8", ",", tblCreator, data->result.size()))
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
	jobListView->addJob("Sensitivity computation", &sensitivity->m_progress, futureWatcher, sensitivity.data());
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

	m_progress.setStatus("Computing dissimilarity between all result pairs.");
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
		int measureCount = static_cast<int>(m_resultDissimMeasures.size());
		int resultCount = static_cast<int>(m_data->result.size());
		m_resultDissimMatrix = iADissimilarityMatrixType(resultCount,
			QVector<iAResultPairInfo>(resultCount, iAResultPairInfo(measureCount)));

		for (size_t m = 0; m < m_resultDissimMeasures.size(); ++m)
		{
			measures.push_back(m_resultDissimMeasures[m].first);
		}
		// fill "upper" half
		for (int r1 = 0; r1 < resultCount - 1 && !m_aborted; ++r1)
		{
			m_progress.emitProgress(100 * r1 / resultCount);
			auto& res1 = m_data->result[r1];
			auto const& mapping = *res1.mapping.data();
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
				auto& res2 = m_data->result[r2];
				qint64 const fiberCount = res2.table->GetNumberOfRows();
				auto& dissimilarities = mat.fiberDissim;
				dissimilarities.resize(fiberCount);
#pragma omp parallel for
				for (int fiberID = 0; fiberID < static_cast<int>(fiberCount); ++fiberID)
				{
					auto it = res2.curveInfo.find(fiberID);
					// find the best-matching fibers in reference & compute difference:
					iAFiberData fiber(res2.table, fiberID, mapping, (it != res2.curveInfo.end()) ? it->second : std::vector<iAVec3f>());
					getBestMatches(fiber, mapping, res1.table, dissimilarities[fiberID], res1.curveInfo,
						diagonalLength, maxLength, m_resultDissimMeasures, m_resultDissimOptimMeasureIdx);
					for (int m = 0; m < measureCount; ++m)
					{
						mat.avgDissim[m] += dissimilarities[fiberID][m][0].dissimilarity;
					}
				}
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] /= res2.fiberCount;
				}
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
		cmbboxStackedBarChartColors->addItems(iAColorThemeManager::instance().availableThemes());
		cmbboxStackedBarChartColors->setCurrentText(DefaultStackedBarColorTheme);

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
		connect(cmbboxStackedBarChartColors, QOverload<int>::of(&QComboBox::currentIndexChanged),
			sensInf, &iASensitivityInfo::changeStackedBarColors);

		connect(cmbboxAggregation, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxCharacteristic, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxOutput, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::paramChanged);
		connect(cmbboxOutput, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::updateOutputControls);

		connect(cmbboxDissimilarity, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::updateDissimilarity);
	}
	int charIdx() const { return cmbboxCharacteristic->currentIndex(); }
	int outputIdx() const { return cmbboxOutput->currentIndex(); }
};



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

	void updateScatterPlotLUT(int starGroupSize, int numOfSTARSteps, size_t resultCount)
	{
		//LOG(lvlDebug, "\nNEW LUT:");
		std::set<int> hiGrp;
		std::set<std::pair<int, int> > hiGrpParam;
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
		m_scatterPlot->setLookupTable(m_lut, m_mdsData->numParams()-1);
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

QString iASensitivityInfo::charactName(int charIdx) const
{
	return m_data->spmData->parameterName(m_charSelected[charIdx])
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


namespace
{
	// there should be a function like this already somewhere?
	QVector<int> stringToIntVector(QString const& listAsString)
	{
		QStringList strList = listAsString.split(",");
		QVector<int> result(strList.size());
		for (auto i=0; i<strList.size(); ++i)
		{
			bool ok;
			result[i] = strList[i].toInt(&ok);
			if (!ok)
			{
				LOG(lvlWarn, QString("Invalid value %1 in stringToInt conversion!").arg(strList[i]));
			}
		}
		return result;
	}
}

QSharedPointer<iASensitivityInfo> iASensitivityInfo::load(QMainWindow* child,
	QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW,
	iAJobListView* jobListView, int histogramBins, iASettings const & projectFile,
	QString const& projectFileName)
{
	QString parameterSetFileName = MakeAbsolute(QFileInfo(projectFileName).absolutePath(), projectFile.value(ProjectParameterFile).toString());
	QVector<int> charsSelected = stringToIntVector(projectFile.value(ProjectCharacteristics).toString());
	QVector<int> charDiffMeasure = stringToIntVector(projectFile.value(ProjectCharDiffMeasures).toString());
	return iASensitivityInfo::create(child, data, nextToDW, jobListView, histogramBins, parameterSetFileName,
		charsSelected, charDiffMeasure);
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

	m_gui->m_paramInfluenceView = new iAParameterInfluenceView(this);
	m_gui->m_dwParamInfluence = new iADockWidgetWrapper(m_gui->m_paramInfluenceView, "Parameter Influence", "foeParamInfluence");
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::parameterChanged, this, &iASensitivityInfo::paramChanged);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::characteristicSelected, this, &iASensitivityInfo::charactChanged);
	connect(m_gui->m_settings->cmbboxCharacteristic, QOverload<int>::of(&QComboBox::currentIndexChanged),
		m_gui->m_paramInfluenceView, &iAParameterInfluenceView::selectStackedBar);
	m_child->splitDockWidget(dwSettings, m_gui->m_dwParamInfluence, Qt::Vertical);

	m_gui->m_paramDetails = new QCustomPlot(m_child);
	auto dwParamDetails = new iADockWidgetWrapper(m_gui->m_paramDetails, "Parameter Details", "foeParamDetails");
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwParamDetails, Qt::Vertical);

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
	m_gui->updateScatterPlotLUT(m_starGroupSize, m_numOfSTARSteps, m_data->result.size());
	m_gui->m_scatterPlot->setPointInfo(QSharedPointer<iAScatterPlotPointInfo>(new iASPParamPointInfo(*this, *m_data.data())));
	auto dwScatterPlot = new iADockWidgetWrapper(m_gui->m_scatterPlot, "Results Overview", "foeScatterPlot");
	connect(m_gui->m_scatterPlot, &iAScatterPlotWidget::pointHighlighted, this, &iASensitivityInfo::resultSelected);
	connect(m_gui->m_scatterPlot, &iAScatterPlotWidget::highlightChanged, this, &iASensitivityInfo::spHighlightChanged);
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwScatterPlot, Qt::Vertical);

	updateDissimilarity();
}

void iASensitivityInfo::changeMeasure(int newMeasure)
{
	m_gui->m_paramInfluenceView->changeMeasure(newMeasure);
}

void iASensitivityInfo::changeAggregation(int newAggregation)
{
	m_gui->m_paramInfluenceView->changeAggregation(newAggregation);
}

void iASensitivityInfo::changeStackedBarColors()
{
	iAColorTheme const* theme = iAColorThemeManager::instance().theme(m_gui->m_settings->cmbboxStackedBarChartColors->currentText());
	m_gui->m_paramInfluenceView->setColorTheme(theme);
}

void iASensitivityInfo::paramChanged()
{
	int outputIdx = m_gui->m_settings->outputIdx();
	int paramIdx = m_gui->m_paramInfluenceView->selectedRow();
	int selCharIdx = m_gui->m_settings->charIdx();
	int measureIdx = m_gui->m_paramInfluenceView->selectedMeasure();
	int aggrType = m_gui->m_paramInfluenceView->selectedAggrType();

	m_gui->m_dwParamInfluence->setWindowTitle("Parameter Influence (by " + DistributionDifferenceMeasureNames()[measureIdx] + ")");

	auto& plot = m_gui->m_paramDetails;
	plot->clearGraphs();
	plot->addGraph();
	plot->graph(0)->setPen(QPen(Qt::blue));

	auto const& data = (outputIdx == outCharacteristic) ?
		sensitivityField[selCharIdx][measureIdx][paramIdx][aggrType]:
		sensitivityFiberCount[paramIdx][aggrType];
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
	plot->yAxis->setLabel( ((outputIdx == outCharacteristic) ?
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

	m_gui->m_paramInfluenceView->showDifferenceDistribution(outputIdx, selCharIdx, aggrType);
}

void iASensitivityInfo::charactChanged(int charIdx)
{
	QSignalBlocker block1(m_gui->m_settings->cmbboxOutput);
	if (charIdx < m_charSelected.size())
	{
		m_gui->m_settings->cmbboxOutput->setCurrentIndex(0);
		QSignalBlocker block2(m_gui->m_settings->cmbboxCharacteristic);
		m_gui->m_settings->cmbboxCharacteristic->setCurrentIndex(charIdx);
	}
	else
	{
		m_gui->m_settings->cmbboxOutput->setCurrentIndex(1);
	}
	paramChanged();
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
	LOG(lvlDebug, "Distance Matrix:");
	for (int r1 = 0; r1 < distMatrix.size(); ++r1)
	{
		QString line;
		for (int r2 = 0; r2 < distMatrix.size(); ++r2)
		{
			distMatrix[r1][r2] = m_resultDissimMatrix[r1][r2].avgDissim[dissimIdx];
			line += " " + QString::number(distMatrix[r1][r2], 'f', 2).rightJustified(5);
		}
		LOG(lvlDebug, QString("%1:%2").arg(r1).arg(line));
	}
	auto mds = computeMDS(distMatrix, 2, 100);
	LOG(lvlDebug, "MDS:");
	for (int i = 0; i < mds.size(); ++i)
	{
		for (int c = 0; c < mds[0].size(); ++c)
		{
			m_gui->m_mdsData->data()[m_gui->m_mdsData->numParams() - 3 + c][i] = mds[i][c];
		}
		LOG(lvlDebug, QString("%1: %2, %3").arg(i).arg(mds[i][0]).arg(mds[i][1]));
	}
	m_gui->m_mdsData->updateRanges();
}

void iASensitivityInfo::spHighlightChanged()
{
	m_gui->updateScatterPlotLUT(m_starGroupSize, m_numOfSTARSteps, m_data->result.size());
}
