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
#include <iAJobListView.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iARunAsync.h>
#include <iAStringHelper.h>

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
#include "iARefDistCompute.h"
#include "iASensitivityDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QSpinBox>
#include <QTableView>
#include <QtConcurrent>
#include <QTextStream>
#include <QVBoxLayout>

#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariant.h>

namespace
{
	const int LayoutMargin = 4;
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
	iAJobListView* jobListView, int histogramBins)
{
	QString fileName = QFileDialog::getOpenFileName(child,
		"Sensitivity: Parameter Sets file", data->folder,
		"Comma-Separated Values (*.csv);;");
	if (fileName.isEmpty())
	{
		return QSharedPointer<iASensitivityInfo>();
	}
	iACsvVectorTableCreator tblCreator;
	if (!readParameterCSV(fileName, "UTF-8", ",", tblCreator, data->result.size()))
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
		new iASensitivityInfo(data, fileName, paramNames, paramValues, child, nextToDW));

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
			sensitivity->variedParams.push_back(p + 1); // +1 because valueMin/valueMax don't contain ID
		}
	}
	if (sensitivity->variedParams.size() == 0)
	{
		LOG(lvlError, "Invalid sampling: No parameter was varied!");
		return QSharedPointer<iASensitivityInfo>();
	}
	//LOG(lvlInfo, QString("Found the following parameters to vary (number: %1): %2")
	//	.arg(sensitivity->variedParams.size())
	//	.arg(joinAsString(sensitivity->variedParams, ",", [&paramNames](int const& i) { return paramNames[i]; })));

	// find out how many additional parameter sets were added per STAR:
	//   - go to first value row; take value of first varied parameter as v
	//   - go down rows, as long as either
	//        first varied parameter has same value as v
	//        or distance of current value of first varied parameter is a multiple
	//        of the distance between its first row value and second row value
	double checkValue0 = paramValues[sensitivity->variedParams[0]][0];
	const double RemainderCheckEpsilon = 1e-12;
	double curCheckValue = paramValues[sensitivity->variedParams[0]][1];
	double diffCheck = std::abs(curCheckValue - checkValue0);
	//LOG(lvlDebug, QString("checkValue0=%1, curCheckValue=%2, diffCheck=%3").arg(checkValue0).arg(curCheckValue).arg(diffCheck));
	double remainder = 0;
	int row = 2;
	while (row < paramValues[sensitivity->variedParams[0]].size() &&
		(remainder < RemainderCheckEpsilon || 	// "approximately a multiple" is not so easy with double
			(std::abs(diffCheck - remainder) < RemainderCheckEpsilon) || // remainder could also be close to but smaller than diffCheck
			(dblApproxEqual(curCheckValue, checkValue0))))
	{
		curCheckValue = paramValues[sensitivity->variedParams[0]][row];
		remainder = std::abs(std::fmod(std::abs(curCheckValue - checkValue0), diffCheck));
		//LOG(lvlDebug, QString("Row %1: curCheckValue=%2, checkValue0=%3, remainder=%4")
		//	.arg(row).arg(curCheckValue).arg(checkValue0).arg(remainder));
		++row;
	}
	sensitivity->m_starGroupSize = row - 1;
	sensitivity->numOfSTARSteps = (sensitivity->m_starGroupSize - 1) / sensitivity->variedParams.size();
	//LOG(lvlInfo,QString("Determined that there are groups of size: %1; number of STAR points per parameter: %2")
	//	.arg(sensitivity->m_starGroupSize)
	//	.arg(sensitivity->numOfSTARSteps)
	//);

	// select output features to compute sensitivity for:
	// - the loaded and computed ones (length, orientation, ...)
	// - dissimilarity measure(s)
	iASensitivityDialog dlg(data);
	if (dlg.exec() != QDialog::Accepted)
	{
		return QSharedPointer<iASensitivityInfo>();
	}
	sensitivity->charactIndex = dlg.selectedCharacteristics();
	sensitivity->charDiffMeasure = dlg.selectedDiffMeasures();
	sensitivity->dissimMeasure = dlg.selectedMeasures();
	sensitivity->m_histogramBins = histogramBins;
	if (sensitivity->charactIndex.size() == 0 || sensitivity->charDiffMeasure.size() == 0)
	{
		QMessageBox::warning(child, "Sensitivity", "You have to select at least one characteristic and at least one measure!", QMessageBox::Ok);
		return QSharedPointer<iASensitivityInfo>();
	}
	iAMeasureSelectionDlg selectMeasure;
	if (selectMeasure.exec() != QDialog::Accepted)
	{
		return QSharedPointer<iASensitivityInfo>();
	}

	sensitivity->m_resultDissimMeasures = selectMeasure.measures();
	sensitivity->m_resultDissimOptimMeasureIdx = selectMeasure.optimizeMeasureIdx();
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
	charHistograms.resize(static_cast<int>(m_data->result.size()));
	for (int rIdx = 0; rIdx < m_data->result.size(); ++rIdx)
	{
		m_progress.emitProgress(static_cast<int>(100 * rIdx / m_data->result.size()));
		auto const& r = m_data->result[rIdx];
		int numCharact = static_cast<int>(m_data->spmData->numParams());
		// TODO: skip some columns? like ID...
		charHistograms[rIdx].reserve(numCharact);
		for (int c = 0; c < numCharact; ++c)
		{
			// make sure of all histograms for the same characteristic have the same range
			double rangeMin = m_data->spmData->paramRange(c)[0];
			double rangeMax = m_data->spmData->paramRange(c)[1];
			std::vector<double> fiberData(r.fiberCount);
			for (size_t fiberID = 0; fiberID < r.fiberCount; ++fiberID)
			{
				fiberData[fiberID] = r.table->GetValue(fiberID, c).ToDouble();
			}
			auto histogram = createHistogram(
				fiberData, m_histogramBins, rangeMin, rangeMax);
			charHistograms[rIdx].push_back(histogram);
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

	paramStep.fill(0.0, variedParams.size());
	sensitivityField.resize(charactIndex.size());
	aggregatedSensitivities.resize(charactIndex.size());
	for (int charIdx = 0; charIdx < charactIndex.size() && !m_aborted; ++charIdx)
	{
		m_progress.emitProgress(100 * charIdx / charactIndex.size());
		//int charactID = charactIndex[charIdx];
		//auto charactName = m_data->spmData->parameterName(charactID);
		//LOG(lvlDebug, QString("Characteristic %1 (%2):").arg(charIdx).arg(charactName));
		sensitivityField[charIdx].resize(charDiffMeasure.size());
		aggregatedSensitivities[charIdx].resize(charDiffMeasure.size());
		for (int diffMeasure = 0; diffMeasure < charDiffMeasure.size(); ++diffMeasure)
		{
			//LOG(lvlDebug, QString("    Difference Measure %1 (%2)").arg(diffMeasure).arg(DistributionDifferenceMeasureNames()[diffMeasure]));
			auto& field = sensitivityField[charIdx][diffMeasure];
			field.resize(NumOfVarianceAggregation);
			auto& agg = aggregatedSensitivities[charIdx][diffMeasure];
			agg.resize(NumOfVarianceAggregation);
			for (int i = 0; i < NumOfVarianceAggregation; ++i)
			{
				field[i].resize(variedParams.size());
				agg[i].fill(0.0, variedParams.size());
			}
			for (int paramIdx = 0; paramIdx < variedParams.size(); ++paramIdx)
			{
				for (int i = 0; i < NumOfVarianceAggregation; ++i)
				{
					field[i][paramIdx].resize(paramSetValues.size());
				}
				// TODO: unify with other loops over STARs
				//QString paramName(m_paramNames[variedParams[paramIdx]]);
				//LOG(lvlDebug, QString("  Parameter %1 (%2):").arg(paramIdx).arg(paramName));
				int origParamColIdx = variedParams[paramIdx];
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
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * numOfSTARSteps;

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
						leftVar = distributionDifference(
							charHistograms[resultIdxGroupStart][charIdx],
							charHistograms[resultIdxParamStart][charIdx],
							diffMeasure);
						//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
						++numLeftRight;
						++numAllLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < numOfSTARSteps)
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
							charHistograms[resultIdxGroupStart][charIdx],
							charHistograms[firstPosStepIdx][charIdx],
							diffMeasure);
						//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
						++numLeftRight;
						++numAllRight;
					}
					double sumTotal = 0;
					bool wasSmaller = true;
					for (int i = 0; i < numOfSTARSteps; ++i)
					{
						int compareIdx = (i==0)? resultIdxGroupStart : (resultIdxParamStart + i - 1);
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
						if (paramVal > paramStartParamVal && wasSmaller)
						{
							wasSmaller = false;
							compareIdx = resultIdxGroupStart;
						}
						double difference = distributionDifference(
							charHistograms[compareIdx][charIdx],
							charHistograms[resultIdxParamStart + i][charIdx],
							diffMeasure);
						sumTotal += difference;
					}
					numAllLeftRight += numLeftRight;
					numAllTotal += numOfSTARSteps;
					double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
					double meanTotal = sumTotal / numOfSTARSteps;
					//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
					//LOG(lvlDebug, QString("        (sum total var = %1) / (numOfSTARSteps = %2)  = %3")
					//	.arg(sumTotal).arg(numOfSTARSteps).arg(meanTotal));
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
		sensitivityFiberCount[i].resize(variedParams.size());
		aggregatedSensitivitiesFiberCount[i].fill(0.0, variedParams.size());
	}
	for (int paramIdx = 0; paramIdx < variedParams.size() && !m_aborted; ++paramIdx)
	{
		m_progress.emitProgress(100 * paramIdx / variedParams.size());
		int origParamColIdx = variedParams[paramIdx];
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
			int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * numOfSTARSteps;

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
			while (paramDiff > 0 && k < numOfSTARSteps)
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
			for (int i = 0; i < numOfSTARSteps; ++i)
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
			numAllTotal += numOfSTARSteps;
			double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
			double meanTotal = sumTotal / numOfSTARSteps;
			//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
			//LOG(lvlDebug, QString("        (sum total var = %1) / (numOfSTARSteps = %2)  = %3")
			//	.arg(sumTotal).arg(numOfSTARSteps).arg(meanTotal));
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
	//charHistHist.resize(charactIndex.size());
	charHistVar.resize(charactIndex.size());
	charHistVarAgg.resize(charactIndex.size());
	for (int charIdx = 0; charIdx < charactIndex.size() && !m_aborted; ++charIdx)
	{
		m_progress.emitProgress(100 * charIdx / charactIndex.size());
		//charHistHist[charIdx].resize(NumOfVarianceAggregation);
		charHistVar[charIdx].resize(NumOfVarianceAggregation);
		charHistVarAgg[charIdx].resize(NumOfVarianceAggregation);
		for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
		{
			//charHistHist[charIdx][aggIdx].resize(variedParams.size());
			charHistVar[charIdx][aggIdx].resize(variedParams.size());
			charHistVarAgg[charIdx][aggIdx].resize(variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < variedParams.size() && !m_aborted; ++paramIdx)
		{
			for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
			{
				//charHistHist[charIdx][aggIdx][paramIdx].resize(m_histogramBins);
				charHistVar[charIdx][aggIdx][paramIdx].resize(m_histogramBins);
				charHistVarAgg[charIdx][aggIdx][paramIdx].fill(0.0, m_histogramBins);
			}
			// TODO: unify with other loops over STARs?
			int origParamColIdx = variedParams[paramIdx];

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
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * numOfSTARSteps;
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
						charHistHist[charIdx][agg][paramIdx][bin][paramSetIdx].push_back(charHistograms[resultIdxGroupStart][charIdx][bin]);
					}
					*/
					charHistVar[charIdx][0][paramIdx][bin][paramSetIdx] = 0;
					if (paramDiff > 0)
					{
						// left-only:
						//charHistHist[charIdx][0][paramIdx][bin][paramSetIdx].push_back(charHistograms[resultIdxParamStart][charIdx][bin]);
						// left+right:
						//charHistHist[charIdx][2][paramIdx][bin][paramSetIdx].push_back(charHistograms[resultIdxGroupStart][charIdx][bin]);
						charHistVar[charIdx][0][paramIdx][bin][paramSetIdx] =
							std::abs(charHistograms[resultIdxGroupStart][charIdx][bin] - charHistograms[resultIdxParamStart][charIdx][bin]);
						charHistVarAgg[charIdx][0][paramIdx][bin] += charHistVar[charIdx][0][paramIdx][bin][paramSetIdx];
						charHistVarAgg[charIdx][2][paramIdx][bin] += charHistVar[charIdx][0][paramIdx][bin][paramSetIdx];
						//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
						++numLeftRight;
						++numAllLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < numOfSTARSteps)
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
						//charHistHist[charIdx][1][paramIdx][bin][paramSetIdx].push_back(charHistograms[firstPosStepIdx][charIdx][bin]);
						// left+right:
						//charHistHist[charIdx][2][paramIdx][bin][paramSetIdx].push_back(charHistograms[firstPosStepIdx][charIdx][bin]);
						charHistVar[charIdx][1][paramIdx][bin][paramSetIdx] =
							std::abs(charHistograms[resultIdxGroupStart][charIdx][bin] - charHistograms[firstPosStepIdx][charIdx][bin]);
						charHistVarAgg[charIdx][1][paramIdx][bin] += charHistVar[charIdx][1][paramIdx][bin][paramSetIdx];
						charHistVarAgg[charIdx][2][paramIdx][bin] += charHistVar[charIdx][1][paramIdx][bin][paramSetIdx];
						//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
						++numLeftRight;
						++numAllRight;
					}
					charHistVar[charIdx][2][paramIdx][bin][paramSetIdx] /= numLeftRight;
					bool wasSmaller = true;
					charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] = 0;
					numAllTotal += numOfSTARSteps;
					for (int i = 0; i < numOfSTARSteps; ++i)
					{
						//charHistHist[charIdx][3][paramIdx][bin][paramSetIdx].push_back(charHistograms[resultIdxParamStart + i][charIdx][bin]);
						int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
						if (paramVal > paramStartParamVal && wasSmaller)
						{
							wasSmaller = false;
							compareIdx = resultIdxGroupStart;
						}
						charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] +=
							std::abs(charHistograms[compareIdx][charIdx][bin] - charHistograms[resultIdxParamStart + i][charIdx][bin]);
					}
					charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] /= numOfSTARSteps;//charHistHist[charIdx][3][paramIdx][bin][paramSetIdx].size();
					charHistVarAgg[charIdx][3][paramIdx][bin] += charHistVar[charIdx][3][paramIdx][bin][paramSetIdx];
				}
				assert(numAllTotal == paramSetValues.size() * numOfSTARSteps);
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

	if (!readDissimilarityMatrixCache(measures))
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
		LOG(lvlError, QString("FIAKER cache file '%1': Unknown cache file format - found identifier %2 does not match expected identifier %3.")
			.arg(cacheFile.fileName())
			.arg(identifier).arg(DissimilarityMatrixCacheFileIdentifier));
		return false;
	}
	quint32 version;
	in >> version;
	if (version > DissimilarityMatrixCacheFileVersion)
	{
		LOG(lvlError, QString("FIAKER cache file '%1': Invalid or too high version number (%2), expected %3 or less.")
			.arg(cacheFile.fileName())
			.arg(version).arg(DissimilarityMatrixCacheFileVersion));
		return false;
	}
	in >> measures;
	in >> m_resultDissimMatrix;
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

#include <charts/iAChartWidget.h>
#include <iAColorTheme.h>
#include <iAStackedBarChart.h>
#include <qthelper/iADockWidgetWrapper.h>
#include <qthelper/iAQTtoUIConnector.h>

#include "ui_SensitivitySettings.h"

#include <QMainWindow>
#include <QMenu>
#include <QScrollArea>

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
		for (int charIdx = 0; charIdx < sensInf->charactIndex.size(); ++charIdx)
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


#include "iAParameterInfluenceView.h"

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

	QSharedPointer<iASPLOMData> m_mdsData;
};

QString iASensitivityInfo::charactName(int charIdx) const
{
	return m_data->spmData->parameterName(charactIndex[charIdx]);
}

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
	auto dwParamInfluence = new iADockWidgetWrapper(m_gui->m_paramInfluenceView, "Parameter Influence", "foeParamInfluence");
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::parameterChanged, this, &iASensitivityInfo::paramChanged);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::characteristicSelected, this, &iASensitivityInfo::charactChanged);
	connect(m_gui->m_settings->cmbboxCharacteristic, QOverload<int>::of(&QComboBox::currentIndexChanged),
		m_gui->m_paramInfluenceView, &iAParameterInfluenceView::selectStackedBar);
	m_child->splitDockWidget(dwSettings, dwParamInfluence, Qt::Vertical);

	m_gui->m_paramDetails = new QCustomPlot(m_child);
	auto dwParamDetails = new iADockWidgetWrapper(m_gui->m_paramDetails, "Parameter Details", "foeParamDetails");
	m_child->splitDockWidget(dwParamInfluence, dwParamDetails, Qt::Vertical);


	m_gui->m_mdsData = QSharedPointer<iASPLOMData>(new iASPLOMData());
	std::vector<QString> paramNames;
	paramNames.push_back("X");
	paramNames.push_back("Y");
	m_gui->m_mdsData->setParameterNames(paramNames, m_data->result.size());
	m_gui->m_mdsData->data()[0].resize(m_data->result.size());
	m_gui->m_mdsData->data()[1].resize(m_data->result.size());
	m_gui->m_scatterPlot = new iAScatterPlotWidget(m_gui->m_mdsData);
	m_gui->m_scatterPlot->setPointRadius(5);
	auto dwScatterPlot = new iADockWidgetWrapper(m_gui->m_scatterPlot, "Results Overview", "foeScatterPlot");
	m_child->splitDockWidget(dwParamInfluence, dwScatterPlot, Qt::Vertical);

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
	int charIdx = m_gui->m_settings->charIdx();
	int measureIdx = m_gui->m_paramInfluenceView->selectedMeasure();
	int aggrType = m_gui->m_paramInfluenceView->selectedAggrType();

	auto& plot = m_gui->m_paramDetails;
	plot->clearGraphs();
	plot->addGraph();
	plot->graph(0)->setPen(QPen(Qt::blue));

	auto const& data = (outputIdx == outCharacteristic) ?
		sensitivityField[charIdx][measureIdx][paramIdx][aggrType]:
		sensitivityFiberCount[paramIdx][aggrType];
	QVector<double> x(data.size()), y(data.size());
	for (int i = 0; i < data.size(); ++i)
	{
		x[i] = paramSetValues[i][variedParams[paramIdx]];
		y[i] = data[i];
	}
	// configure right and top axis to show ticks but no labels:
	// (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
	plot->xAxis2->setVisible(true);
	plot->xAxis2->setTickLabels(false);
	plot->yAxis2->setVisible(true);
	plot->yAxis2->setTickLabels(false);
	plot->xAxis->setLabel(m_paramNames[variedParams[paramIdx]]);
	plot->yAxis->setLabel( ((outputIdx == outCharacteristic) ?
		"Sensitivity " + (charactName(charIdx) + " (" + DistributionDifferenceMeasureNames()[measureIdx]+") ") :
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

	m_gui->m_paramInfluenceView->showDifferenceDistribution(outputIdx, charIdx, aggrType);
}

void iASensitivityInfo::charactChanged(int charIdx)
{
	QSignalBlocker block1(m_gui->m_settings->cmbboxOutput);
	if (charIdx < charactIndex.size())
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
	for (int r1 = 0; r1 < distMatrix.size(); ++r1)
	{
		for (int r2 = 0; r2 < distMatrix.size(); ++r2)
		{
			distMatrix[r1][r2] = m_resultDissimMatrix[r1][r2].avgDissim[dissimIdx];
		}
	}
	auto mds = computeMDS(distMatrix, 2, 10);
	for (int i = 0; i < mds.size(); ++i)
	{
		for (int c = 0; c < mds[0].size(); ++c)
		{
			m_gui->m_mdsData->data()[c][i] = mds[i][c];
		}
		LOG(lvlInfo, QString("%1: %2, %3").arg(i).arg(mds[i][0]).arg(mds[i][1]));
	}
	m_gui->m_mdsData->updateRanges();
}
