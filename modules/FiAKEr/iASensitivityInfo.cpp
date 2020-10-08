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
#include <iAConsole.h>
#include <iAMathUtility.h>
#include <iAStringHelper.h>

// FeatureScout
#include "iACsvVectorTableCreator.h"

// Segmentation
#include "iAVectorTypeImpl.h"
#include "iAVectorDistanceImpl.h"

// FIAKER
#include "iAFiberCharData.h"
#include "iAFiberData.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableView>
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
	QStringList const & MeasureNames()
	{
		static QStringList Names = QStringList() << "L2 Difference" << "Jensen-Shannon divergence";
		return Names;
	}
	QStringList const& AggregationNames()
	{
		static QStringList Names = QStringList() << "Mean left+right" << "Left only" << "Right only";
		return Names;
	}
}

// Factor out as generic CSV reading class also used by iACsvIO?
bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator, iACsvTableCreator& tblCreator, size_t resultCount)
{
	if (!QFile::exists(fileName))
	{
		DEBUG_LOG("Error loading csv file, file does not exist.");
		return false;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Unable to open file '%1': %2").arg(fileName).arg(file.errorString()));
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
		DEBUG_LOG("Found additional rows at end...");
		return false;
	}
	return true;
}

void addCheckItem(QStandardItemModel* model, int i, QString const& title)
{
	model->setItem(i, 0, new QStandardItem(title));
	auto checkItem = new QStandardItem();
	checkItem->setCheckable(true);
	checkItem->setCheckState(Qt::Unchecked);
	model->setItem(i, 1, checkItem);
}

QVector<int> selectedIndices(QStandardItemModel* model)
{
	QVector<int> result;
	for (int row = 0; row < model->rowCount(); ++row)
	{
		if (model->item(row, 1)->checkState() == Qt::Checked)
		{
			result.push_back(row);
		}
	}
	return result;
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
		DEBUG_LOG(QString("invalid diffType %1").arg(diffType));
		return 0;
	}
}

QSharedPointer<iASensitivityInfo> iASensitivityInfo::create(QString const& parameterFileName,
	QSharedPointer<iAFiberResultsCollection> data)
{
	iACsvVectorTableCreator tblCreator;
	// csv assumed to contain header line (names of parameters), and one row per parameter set;
	// parameter set contains an ID as first column and a filename as last row
	if (!readParameterCSV(parameterFileName, "UTF-8", ",", tblCreator, data->result.size()))
	{
		return QSharedPointer<iASensitivityInfo>();
	}
	QSharedPointer<iASensitivityInfo> sensitivityInfo(new iASensitivityInfo(data));
	auto paramNames = tblCreator.header();
	sensitivityInfo->paramNames = paramNames;
	sensitivityInfo->allParamValues = tblCreator.table();
	auto& paramValues = sensitivityInfo->allParamValues;

	if (paramValues.size() <= 2 || paramValues[0].size() <= 3)
	{
		DEBUG_LOG(QString("Invalid parameter set file: expected at least 2 data rows (actual: %1) "
			"and at least 3 columns (ID, filename, and one parameter; actual: %2")
			.arg(paramValues.size() > 0 ? paramValues[0].size() : -1)
			.arg(paramValues.size())
		);
		return QSharedPointer<iASensitivityInfo>();
	}
	// data in paramValues is indexed [col(=parameter index)][row(=parameter set index)]

	// find min/max, for all columns except ID and filename
	QVector<double> valueMin(static_cast<int>(paramValues.size() - 2), std::numeric_limits<double>::max());
	QVector<double> valueMax(static_cast<int>(paramValues.size() - 2), std::numeric_limits<double>::lowest());
	DEBUG_LOG(QString("Parameter values size: %1x%2").arg(paramValues.size()).arg(paramValues[0].size()));
	// TODO: common min/max calculator for table? (/ SPM data)
	for (int p = 1; p < paramValues.size() - 1; ++p)
	{
		for (int row = 0; row < paramValues[0].size(); ++row)
		{
			double value = paramValues[p][row];
			if (value < valueMin[p - 1])    // -1 because of skipping ID
			{
				valueMin[p - 1] = value;
			}
			if (value > valueMax[p - 1])    // -1 because of skipping ID
			{
				valueMax[p - 1] = value;
			}
		}
	}

	// countOfVariedParams = number of parameters for which min != max:
	for (int p = 0; p < valueMin.size(); ++p)
	{
		if (valueMin[p] != valueMax[p])
		{
			sensitivityInfo->variedParams.push_back(p + 1); // +1 because valueMin/valueMax don't contain ID
		}
	}
	if (sensitivityInfo->variedParams.size() == 0)
	{
		DEBUG_LOG("Invalid sampling: No parameter was varied!");
		return QSharedPointer<iASensitivityInfo>();
	}
	DEBUG_LOG(QString("Found the following parameters to vary (number: %1): %2")
		.arg(sensitivityInfo->variedParams.size())
		.arg(joinAsString(sensitivityInfo->variedParams, ",", [&paramNames](int const& i) { return paramNames[i + 1]; })));

	// find out how many additional parameter sets were added per STAR:
	//   - go to first value row; take value of first varied parameter as v
	//   - go down rows, as long as either
	//        first varied parameter has same value as v
	//        or distance of current value of first varied parameter is a multiple
	//        of the distance between its first row value and second row value
	double checkValue0 = paramValues[sensitivityInfo->variedParams[0]][0];
	const double RemainderCheckEpsilon = 1e-14;
	double curCheckValue = paramValues[sensitivityInfo->variedParams[0]][1];
	double diffCheck = curCheckValue - checkValue0;
	//DEBUG_LOG(QString("checkValue0=%1, diffCheck=%2").arg(checkValue0).arg(diffCheck));
	double remainder = 0;
	int row = 2;
	while (row < paramValues[sensitivityInfo->variedParams[0]].size() &&
		(remainder < RemainderCheckEpsilon || (dblApproxEqual(curCheckValue, checkValue0))))
	{
		curCheckValue = paramValues[sensitivityInfo->variedParams[0]][row];
		remainder = std::abs(std::fmod(std::abs(curCheckValue - checkValue0), diffCheck));
		//DEBUG_LOG(QString("curCheckValue=%1, remainder=%2, row=%3").arg(curCheckValue).arg(remainder).arg(row));
		++row;
	}
	int starGroupSize = row - 1;

	sensitivityInfo->numOfSTARSteps = (starGroupSize - 1) / sensitivityInfo->variedParams.size();
	DEBUG_LOG(QString("Determined that there are groups of size: %1; number of STAR points per parameter: %2")
		.arg(starGroupSize)
		.arg(sensitivityInfo->numOfSTARSteps)
	);

	// select output features to compute sensitivity for:
	// - the loaded and computed ones (length, orientation, ...)
	// - dissimilarity measure(s)

	auto buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

	auto tvCharacteristic(new QTableView);
	auto characteristicsModel(new QStandardItemModel());
	characteristicsModel->setHorizontalHeaderLabels(QStringList() << "Characteristic" << "Select");
	for (int i = 0; i < static_cast<int>(data->m_resultIDColumn); ++i)
	{
		addCheckItem(characteristicsModel, i, data->spmData->parameterName(i));
	}
	tvCharacteristic->setModel(characteristicsModel);

	auto tvDiffMeasures(new QTableView);
	auto diffMeasuresModel(new QStandardItemModel());
	diffMeasuresModel->setHorizontalHeaderLabels(QStringList() << "Difference" << "Select");
	// Difference Between Mean, Min, Max ? other single measures?
	for (int i = 0; i < MeasureNames().size(); ++i)
	{
		addCheckItem(diffMeasuresModel, i, MeasureNames()[i]);
	}
	//addCheckItem(diffMeasuresModel, 2, "Mutual information");
	// ... some other measures from iAVectorDistance...?

	tvDiffMeasures->setModel(diffMeasuresModel);

	auto tvMeasures(new QTableView);
	auto measuresModel(new QStandardItemModel());
	measuresModel->setHorizontalHeaderLabels(QStringList() << "Measure" << "Select");
	auto measureNames = getAvailableDissimilarityMeasureNames();
	for (size_t i = 0; i < measureNames.size(); ++i)
	{
		addCheckItem(measuresModel, i, measureNames[i]);
	}
	tvMeasures->setModel(measuresModel);

	QDialog dlg;
	QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	dlg.setLayout(new QVBoxLayout);
	dlg.setWindowTitle("Characteristic/Measure/Difference selection");
	dlg.layout()->setContentsMargins(LayoutMargin, LayoutMargin, LayoutMargin, LayoutMargin);
	dlg.layout()->setSpacing(LayoutSpacing);
	dlg.layout()->addWidget(new QLabel("Characteristic for which to compute sensitivity:"));
	dlg.layout()->addWidget(tvCharacteristic);
	dlg.layout()->addWidget(new QLabel("Measure difference between two feature distributions:"));
	dlg.layout()->addWidget(tvDiffMeasures);
	dlg.layout()->addWidget(new QLabel("Measures for which to compute sensitivity:"));
	dlg.layout()->addWidget(tvMeasures);
	dlg.layout()->addWidget(new QLabel("Number of Histogram Bins:"));
	QSpinBox* spHistogramBins(new QSpinBox());
	spHistogramBins->setMinimum(2);
	spHistogramBins->setMaximum(9999);
	spHistogramBins->setValue(100);
	dlg.layout()->addWidget(spHistogramBins);
	dlg.layout()->addWidget(buttonBox);

	if (dlg.exec() != QDialog::Accepted)
	{
		return QSharedPointer<iASensitivityInfo>();
	}
	sensitivityInfo->charactIndex = selectedIndices(characteristicsModel);
	sensitivityInfo->charDiffMeasure = selectedIndices(diffMeasuresModel);
	sensitivityInfo->dissimMeasure = selectedIndices(measuresModel);
	const size_t HistogramBins = spHistogramBins->value();

	// store parameter set values
	for (int p = 0; p < paramValues[0].size(); p += starGroupSize)
	{
		QVector<double> parameterSet;
		for (int v = 0; v < paramValues.size(); ++v)
		{
			parameterSet.push_back(paramValues[v][p]);
		}
		sensitivityInfo->paramSetValues.push_back(parameterSet);
	}

	// TODO: make computation asynchronous

	// sensitivityInfo->paramStep.fill(0.0, sensitivityInfo->variedParams.size());

	// compute characteristics distribution (histogram) for all results:

	sensitivityInfo->resultCharacteristicHistograms.resize(data->result.size());
	for (int rIdx = 0; rIdx < data->result.size(); ++rIdx)
	{
		auto const& r = data->result[rIdx];
		int numCharact = data->spmData->numParams();
		// TODO: skip some columns? like ID...
		sensitivityInfo->resultCharacteristicHistograms[rIdx].reserve(numCharact);
		for (int c = 0; c < numCharact; ++c)
		{
			// make sure of all histograms for the same characteristic have the same range
			double rangeMin = data->spmData->paramRange(c)[0];
			double rangeMax = data->spmData->paramRange(c)[1];
			std::vector<double> fiberData(r.fiberCount);
			for (size_t fiberID = 0; fiberID < r.fiberCount; ++fiberID)
			{
				fiberData[fiberID] = r.table->GetValue(fiberID, c).ToDouble();
			}
			auto histogram = createHistogram<std::vector<double>, double, size_t, double>(
				fiberData, HistogramBins, rangeMin, rangeMax);
			sensitivityInfo->resultCharacteristicHistograms[rIdx].push_back(histogram);
		}
	}

	// for each characteristic
	//     for each varied parameter
	//         for each selected characteristics difference measure
	//             for each "aggregation type" - left only, right only, average/? over full range
	//                 for each point in parameter space (of base sampling method)
	//                     compute local change by that difference measure

	const int NumOfVarianceAggregation = 4;

	sensitivityInfo->sensitivityField.resize(sensitivityInfo->charactIndex.size());
	sensitivityInfo->aggregatedSensitivities.resize(sensitivityInfo->charactIndex.size());
	for (int charactIdx = 0; charactIdx < sensitivityInfo->charactIndex.size(); ++charactIdx)
	{
		sensitivityInfo->sensitivityField[charactIdx].resize(sensitivityInfo->variedParams.size());
		sensitivityInfo->aggregatedSensitivities[charactIdx].resize(sensitivityInfo->variedParams.size());
		int charactID = sensitivityInfo->charactIndex[charactIdx];
		auto charactName = data->spmData->parameterName(charactID);
		for (int paramIdx = 0; paramIdx < sensitivityInfo->variedParams.size(); ++paramIdx)
		{
			int origParamColIdx = sensitivityInfo->variedParams[paramIdx];
			sensitivityInfo->sensitivityField[charactIdx][paramIdx].resize(sensitivityInfo->charDiffMeasure.size());
			sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx].resize(sensitivityInfo->charDiffMeasure.size());
			for (int diffMeasure = 0; diffMeasure < sensitivityInfo->charDiffMeasure.size(); ++diffMeasure)
			{
				// for now:
				//     - one step average, left only, right only
				//      future: overall (weighted) average, ...=
				sensitivityInfo->sensitivityField[charactIdx][paramIdx][diffMeasure].resize(NumOfVarianceAggregation);
				sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure].fill(0.0, NumOfVarianceAggregation);
				for (int i = 0; i < NumOfVarianceAggregation; ++i)
				{
					sensitivityInfo->sensitivityField[charactIdx][paramIdx][diffMeasure][i].resize(sensitivityInfo->paramSetValues.size());
				}
				int numLeft = 0,
					numRight = 0,
					numTotal = 0;
				for (int paramSetIdx = 0; paramSetIdx < sensitivityInfo->paramSetValues.size(); ++paramSetIdx)
				{
					int resultIdxGroupStart = starGroupSize * paramSetIdx;
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * sensitivityInfo->numOfSTARSteps;

					// first - then + steps (both skipped if value +/- step exceeds bounds
					double groupStartParamVal = sensitivityInfo->allParamValues[origParamColIdx][resultIdxGroupStart];
					double paramStartParamVal = sensitivityInfo->allParamValues[origParamColIdx][resultIdxParamStart];
					double paramDiff = paramStartParamVal - groupStartParamVal;

					double leftVar = 0;
					int numVals = 0;
					if (paramDiff > 0)
					{
						leftVar = distributionDifference(
							sensitivityInfo->resultCharacteristicHistograms[resultIdxGroupStart][charactIdx],
							sensitivityInfo->resultCharacteristicHistograms[resultIdxParamStart][charactIdx],
							diffMeasure);
						++numVals;
						++numLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < sensitivityInfo->numOfSTARSteps)
					{
						double paramVal = sensitivityInfo->allParamValues[origParamColIdx][resultIdxParamStart + k];
						paramDiff = paramStartParamVal - paramVal;
						++k;
					}
					double rightVar = 0;
					if (paramDiff < 0)
					{
						int firstPosStepIdx = resultIdxParamStart + (k - 1);
						rightVar = distributionDifference(
							sensitivityInfo->resultCharacteristicHistograms[resultIdxGroupStart][charactIdx],
							sensitivityInfo->resultCharacteristicHistograms[firstPosStepIdx][charactIdx],
							diffMeasure);
						++numVals;
						++numRight;
					}
					numTotal += numVals;
					double meanVar = (leftVar + rightVar) / numVals;
					sensitivityInfo->sensitivityField[charactIdx][paramIdx][diffMeasure][0][paramSetIdx] = meanVar;
					sensitivityInfo->sensitivityField[charactIdx][paramIdx][diffMeasure][1][paramSetIdx] = leftVar;
					sensitivityInfo->sensitivityField[charactIdx][paramIdx][diffMeasure][2][paramSetIdx] = rightVar;

					sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure][0] += meanVar;
					sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure][1] += leftVar;
					sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure][2] += rightVar;
				}
				sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure][0] /= numTotal;
				sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure][1] /= numLeft;
				sensitivityInfo->aggregatedSensitivities[charactIdx][paramIdx][diffMeasure][2] /= numRight;
			}
		}
	}

	return sensitivityInfo;
}

iASensitivityInfo::iASensitivityInfo(QSharedPointer<iAFiberResultsCollection> data):
	m_data(data)
{}

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

		cmbboxMeasure->addItems(MeasureNames());
		cmbboxAggregation->addItems(AggregationNames());
		connect(cmbboxMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::changeMeasure);
		connect(cmbboxAggregation, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::changeAggregation);
	}
};

class iAParameterInfluenceView: public QWidget
{
private:
	//! stacked bar charts (one per parameter)
	QVector<iAStackedBarChart*> m_stackedCharts;
	iAStackedBarChart* m_stackedHeader;
	//! sensitivity information
	iASensitivityInfo* m_sensInf;
	//! measure to use 
	int m_measureIdx;
	//! aggregation type
	int m_aggrType;
public:
	iAParameterInfluenceView(iASensitivityInfo* sensInf):
		m_sensInf(sensInf),
		m_measureIdx(0),
		m_aggrType(0)
	{
		setLayout(new QHBoxLayout);
		layout()->setContentsMargins(0, 0, 0, 0);
		auto paramScrollArea = new QScrollArea();
		paramScrollArea->setWidgetResizable(true);
		auto paramList = new QWidget();
		paramScrollArea->setWidget(paramList);
		paramScrollArea->setContentsMargins(0, 0, 0, 0);
		layout()->addWidget(paramScrollArea);

		auto paramListLayout = new QGridLayout();
		paramList->setLayout(paramListLayout);
		paramListLayout->setSpacing(LayoutSpacing);
		paramListLayout->setContentsMargins(LayoutMargin, LayoutMargin, LayoutMargin, LayoutMargin);
		paramListLayout->setColumnStretch(0, 1); // param name
		paramListLayout->setColumnStretch(1, 2); // stacked bar chart
		//paramListLayout->setColumnStretch(2, 2); // histogram
		
		auto colorTheme = iAColorThemeManager::instance().theme(DefaultStackedBarColorTheme);
		m_stackedHeader = new iAStackedBarChart(colorTheme, true);
		// TODO: Unify/Group stacked bar widgets here / in iAFIAKERController into a class
		// which encapsulates updating weights, showing columns, unified data interface (table?)
		// for all characteristics, add column to stacked bar charts
		for (size_t charactIdx = 0; charactIdx < sensInf->charactIndex.size(); ++charactIdx)
		{
			auto charactShowAction = new QAction(charactName(charactIdx), nullptr);
			charactShowAction->setProperty("charactIdx", static_cast<unsigned long long>(charactIdx));
			charactShowAction->setCheckable(true);
			charactShowAction->setChecked(false);
			connect(charactShowAction, &QAction::triggered, this, &iAParameterInfluenceView::stackedColSelect);
			m_stackedHeader->contextMenu()->addAction(charactShowAction);
		}

		DEBUG_LOG(QString("Adding lines for %1 characteristics").arg(sensInf->charactIndex.size()));
		paramListLayout->addWidget(new QLabel("Parameter"), 0, 0);
		paramListLayout->addWidget(m_stackedHeader, 0, 1);
		for (int paramIdx = 0; paramIdx < sensInf->variedParams.size(); ++paramIdx)
		{
			m_stackedCharts.push_back(new iAStackedBarChart(colorTheme, false));
			connect(m_stackedHeader, &iAStackedBarChart::weightsChanged, m_stackedCharts[paramIdx], &iAStackedBarChart::setWeights);
			paramListLayout->addWidget(new QLabel(sensInf->paramNames[sensInf->variedParams[paramIdx]]), 1 + paramIdx, 0);
			paramListLayout->addWidget(m_stackedCharts[paramIdx], 1 + paramIdx, 1);
		}
	}

	void changeMeasure(int newMeasure)
	{
		m_measureIdx = newMeasure;
		updateStackedBars();
	}

	void changeAggregation(int newAggregation)
	{
		m_aggrType = newAggregation;
		updateStackedBars();
	}
private slots:
	void stackedColSelect()
	{
		auto source = qobject_cast<QAction*>(QObject::sender());
		size_t charactIdx = source->property("charactIdx").toULongLong();
		if (source->isChecked())
		{
			addStackedBar(charactIdx);
		}
		else
		{
			removeStackedBar(charactIdx);
		}
	}
	void selectMeasure(int measureIdx)
	{
		m_measureIdx = measureIdx;
		//updateStackedBars();
	}

private:
	QVector<int> m_visibleCharacts;

	void updateStackedBars()
	{
		for (auto charactIdx : m_visibleCharacts)
		{
			// TODO: unify with addStackedBar
			auto title(charactName(charactIdx));
			double maxValue = std::numeric_limits<double>::lowest();
			for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
			{
				double value = m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType];
				if (value > maxValue)
				{
					maxValue = value;
				}
			}
			for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
			{
				// characteristis
				// parameter index
				// difference measure
				// variation aggregation
				double value = m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType];
				m_stackedCharts[paramIdx]->updateBar(title, value, maxValue);
			}
		}
		for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
		{
			m_stackedCharts[paramIdx]->update();
		}
	}

	QString charactName(int charactIdx)
	{
		return m_sensInf->m_data->spmData->parameterName(m_sensInf->charactIndex[charactIdx]);
	}

	void addStackedBar(int charactIdx)
	{
		m_visibleCharacts.push_back(charactIdx);
		auto title(charactName(charactIdx));
		DEBUG_LOG(QString("Showing stacked bar for characteristic %1").arg(title));
		m_stackedHeader->addBar(title, 1, 1);

		double maxValue = std::numeric_limits<double>::lowest();
		for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
		{
			double value = m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType];
			if (value > maxValue)
			{
				maxValue = value;
			}
		}
		for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
		{
			// characteristis
			// parameter index
			// difference measure
			// variation aggregation
			double value = m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType];
			m_stackedCharts[paramIdx]->addBar(title, value, maxValue);
		}
	}

	void removeStackedBar(int charactIdx)
	{
		int visibleIdx = m_visibleCharacts.indexOf(charactIdx);
		if (visibleIdx == -1)
		{
			DEBUG_LOG(QString("Invalid state - called remove on non-added characteristic idx %1").arg(charactIdx));
		}
		m_visibleCharacts.remove(visibleIdx);
		auto title(charactName(charactIdx));
		DEBUG_LOG(QString("Showing stacked bar for characteristic %1").arg(title));
		m_stackedHeader->removeBar(title);
		for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
		{
			m_stackedCharts[paramIdx]->removeBar(title);
		}
	}

};

class iASensitivityGUI
{
public:

	//! @{ Param Influence List
	iAParameterInfluenceView* m_paramInfluenceView;
	//! @}

	//! Overall settings
	iASensitivitySettingsView* m_settings;
};

void iASensitivityInfo::createGUI(QMainWindow* child, QDockWidget* nextToDW)
{
	m_gui.reset(new iASensitivityGUI);

	m_gui->m_settings = new iASensitivitySettingsView(this);
	auto dwSettings = new iADockWidgetWrapper(m_gui->m_settings, "Sensitivity Settings", "foeSensitivitySettings");
	child->splitDockWidget(nextToDW, dwSettings, Qt::Horizontal);

	m_gui->m_paramInfluenceView = new iAParameterInfluenceView(this);
	auto dwParamInfluence = new iADockWidgetWrapper(m_gui->m_paramInfluenceView, "Parameter Influence", "foeParamInfluence");
	child->splitDockWidget(dwSettings, dwParamInfluence, Qt::Vertical);
}

void iASensitivityInfo::changeMeasure(int newMeasure)
{
	m_gui->m_paramInfluenceView->changeMeasure(newMeasure);
}

void iASensitivityInfo::changeAggregation(int newAggregation)
{
	m_gui->m_paramInfluenceView->changeAggregation(newAggregation);
}