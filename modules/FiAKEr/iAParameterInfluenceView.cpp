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
#include "iAParameterInfluenceView.h"

#include "iAFiberCharData.h"
#include "iAFiberData.h" // for getAvailableDissimilarityMeasureNames
#include "iASensitivityInfo.h"
#include "iAStackedBarChart.h"
#include "qthelper/iAClickableLabel.h"

#include <charts/iAChartWidget.h>
#include <charts/iAHistogramData.h>
#include <charts/iAPlotTypes.h>
#include <charts/iASPLOMData.h>
#include <charts/iAXYPlotData.h>
#include <iAColorTheme.h>
#include <iALog.h>

#include <QAction>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>

namespace
{
	enum ColumnIndices { colParamName = 0, colMin = 1, colMax = 2, colStep = 3, colStackedBar = 4
		//, colHistogram = 5 
	};
	const int GridSpacing = 2;
	const int LayoutMargin = 4;
	const int RowsPerParam = 4;

	enum
	{
		RowStackedHeader = 0,
		RowStackedBar = 1,
		RowOutputChart = 2,
		RowParamChart = 3
	};
	const int LabelCount = 4;
}

class iAParTableRow
{
public:
	iAStackedBarChart* head;
	iAStackedBarChart* bars;
	QVector<iAChartWidget*> out;
	QVector<iAChartWidget*> par;
	iAClickableLabel* labels[LabelCount];
};

iAParameterInfluenceView::iAParameterInfluenceView(iASensitivityInfo* sensInf, QColor const & paramColor, QColor const & outputColor) :
	m_sensInf(sensInf),
	m_measureIdx(0),
	m_aggrType(0),
	m_selectedParam(-1),
	m_selectedCol(-1),
	m_paramListLayout(new QGridLayout()),
	m_stackedBarTheme(new iASingleColorTheme("OneOutputColorTheme", outputColor)),
	m_table(sensInf->m_variedParams.size()),
	m_sort(sensInf->m_variedParams.size())
{
	for (int i=0; i<m_sort.size(); ++i)
	{
		m_sort[i] = i;
	}
	setLayout(new QHBoxLayout);
	layout()->setContentsMargins(1, 0, 1, 0);
	auto paramScrollArea = new QScrollArea();
	paramScrollArea->setWidgetResizable(true);
	auto paramList = new QWidget();
	paramScrollArea->setWidget(paramList);
	paramScrollArea->setContentsMargins(0, 0, 0, 0);
	layout()->addWidget(paramScrollArea);

	paramList->setLayout(m_paramListLayout);
	m_paramListLayout->setSpacing(GridSpacing);
	m_paramListLayout->setContentsMargins(LayoutMargin, LayoutMargin, LayoutMargin, LayoutMargin);
	m_paramListLayout->setColumnStretch(colParamName, 1);
	m_paramListLayout->setColumnStretch(colMin, 1);
	m_paramListLayout->setColumnStretch(colMax, 1);
	m_paramListLayout->setColumnStretch(colStep, 1);
	m_paramListLayout->setColumnStretch(colStackedBar, 10);
	//m_paramListLayout->setColumnStretch(colHistogram, 10);

	// TODO: Unify/Group stacked bar widgets here / in iAFIAKERController into a class
	// which encapsulates updating weights, showing columns, unified data interface (table?)
	// for all characteristics, add column to stacked bar charts


	//LOG(lvlDebug, QString("Adding lines for %1 characteristics").arg(sensInf->m_charSelected.size()));

	// headers:
	addHeaderLabel(m_paramListLayout, colParamName, "Parameter");
	addHeaderLabel(m_paramListLayout, colMin, "Min");
	addHeaderLabel(m_paramListLayout, colMax, "Max");
	addHeaderLabel(m_paramListLayout, colStep, "Step");
	//m_paramListLayout->addWidget(m_stackedHeader, 0, colStackedBar);
	//addHeaderLabel(m_paramListLayout, colHistogram, "Difference Distribution");

	for (int paramIdx = 0; paramIdx < sensInf->m_variedParams.size(); ++paramIdx)
	{
		iAParTableRow & row = m_table[paramIdx];
		int rowIdx = 1 + RowsPerParam * paramIdx;
		row.head = new iAStackedBarChart(m_stackedBarTheme.data(), m_paramListLayout,
			rowIdx + RowStackedHeader, colStackedBar, true);
		row.head->setDoStack(false);
		row.head->setNormalizeMode(false);
		connect(row.head, &iAStackedBarChart::barDblClicked, this, &iAParameterInfluenceView::stackedBarDblClicked);
		QString paramName = sensInf->m_paramNames[sensInf->m_variedParams[paramIdx]];
		row.bars = new iAStackedBarChart(m_stackedBarTheme.data(), m_paramListLayout,
			rowIdx + RowStackedBar, colStackedBar, false,
			paramIdx == sensInf->m_variedParams.size() - 1);
		row.bars->setNormalizeMode(false);
		row.bars->setDoStack(false);
		row.bars->setProperty("paramIdx", paramIdx);
		connect(row.bars, &iAStackedBarChart::clicked, this, &iAParameterInfluenceView::paramChangedSlot);
		connect(row.head, &iAStackedBarChart::weightsChanged      , this, &iAParameterInfluenceView::setBarWeights);
		connect(row.head, &iAStackedBarChart::normalizeModeChanged, this, &iAParameterInfluenceView::setBarNormalizeMode);
		connect(row.head, &iAStackedBarChart::switchedStackMode   , this, &iAParameterInfluenceView::setBarDoStack);
		auto const& paramVec = sensInf->m_paramValues[sensInf->m_variedParams[paramIdx]];
		double minVal = *std::min_element(paramVec.begin(), paramVec.end()),
			maxVal = *std::max_element(paramVec.begin(), paramVec.end());
		row.labels[colParamName] = new iAClickableLabel(paramName);
		row.labels[colParamName]->setStyleSheet("QLabel { background-color : " + paramColor.name() + "; }");
		row.labels[colMin] = new iAClickableLabel(QString::number(minVal, 'f', digitsAfterComma(sensInf->paramStep[paramIdx])));
		row.labels[colMax] = new iAClickableLabel(QString::number(maxVal, 'f', digitsAfterComma(sensInf->paramStep[paramIdx])));
		row.labels[colStep] = new iAClickableLabel(QString::number(sensInf->paramStep[paramIdx]));
		for (int i = colParamName; i <= colStep; ++i)
		{
			row.labels[i]->setProperty("paramIdx", paramIdx);
			connect(row.labels[i], &iAClickableLabel::clicked, this, &iAParameterInfluenceView::paramChangedSlot);
		}
		//m_paramListLayout->addWidget(m_stackedCharts[paramIdx], rowIdx, colStackedBar);

		//m_diffChart.push_back(new iAChartWidget(this, "Characteristics distribution", ));
		//m_paramListLayout->addWidget(m_diffChart[paramIdx], 1 + paramIdx, colHistogram);
	}
	for (int charactIdx = 0; charactIdx < sensInf->m_charSelected.size(); ++charactIdx)
	{
		addColumnAction(outCharacteristic, charactIdx, charactIdx == 0);
	}
	addColumnAction(outFiberCount, -1, false);
	for (int dissimMeasIdx = 0; dissimMeasIdx < sensInf->m_resultDissimMeasures.size(); ++dissimMeasIdx)
	{
		addColumnAction(outDissimilarity, dissimMeasIdx, false);
	}

	addStackedBar(outCharacteristic, 0);  // reflected in iAAlgorithmInfo construction in iASensitivity -> put in common place?

	updateTableOrder();
}

void iAParameterInfluenceView::updateTableOrder()
{
	for (int paramRow = 0; paramRow < m_table.size(); ++paramRow)
	{
		int paramIdx = m_sort[paramRow];
		int rowIdx = 1 + RowsPerParam * paramRow;
		for (int i=0; i<LabelCount; ++i)
		{
			m_paramListLayout->addWidget(m_table[paramIdx].labels[i], rowIdx, i, RowsPerParam, 1);
		}
		m_table[paramIdx].head->setPos(rowIdx+RowStackedHeader, colStackedBar);
		m_table[paramIdx].bars->setPos(rowIdx+RowStackedBar, colStackedBar);
		for (int c=0; c<m_table[paramIdx].out.size(); ++c)
		{
			m_paramListLayout->addWidget(m_table[paramIdx].out[c], rowIdx + RowOutputChart, colStackedBar + c);
			m_paramListLayout->addWidget(m_table[paramIdx].par[c], rowIdx + RowParamChart, colStackedBar + c);
		}
	}
}

void iAParameterInfluenceView::addColumnAction(int objectType, int charactIdx, bool checked)
{
	auto charactShowAction = new QAction(columnName(objectType, charactIdx), nullptr);
	charactShowAction->setProperty("objectType", objectType);
	charactShowAction->setProperty("charactIdx", charactIdx);
	charactShowAction->setCheckable(true);
	charactShowAction->setChecked(checked);
	QObject::connect(charactShowAction, &QAction::triggered, this, &iAParameterInfluenceView::showStackedBar);
	for (auto row : m_table)
	{
		row.head->contextMenu()->addAction(charactShowAction);
	}
}

void iAParameterInfluenceView::setMeasure(int newMeasure)
{
	m_measureIdx = newMeasure;
	updateStackedBars();
	emit parameterChanged();
}

void iAParameterInfluenceView::setAggregation(int newAggregation)
{
	m_aggrType = newAggregation;
	updateStackedBars();
	emit parameterChanged();
}

int iAParameterInfluenceView::selectedMeasure() const { return m_measureIdx; }
int iAParameterInfluenceView::selectedAggrType() const { return m_aggrType; }
int iAParameterInfluenceView::selectedRow() const { return m_selectedParam; }
int iAParameterInfluenceView::selectedCol() const { return m_selectedCol; }

void iAParameterInfluenceView::showStackedBar()
{
	auto source = qobject_cast<QAction*>(QObject::sender());
	int objectType = source->property("objectType").toInt();
	int charactIdx = source->property("charactIdx").toInt();
	toggleBar(source->isChecked(), objectType, charactIdx);
}

void iAParameterInfluenceView::selectStackedBar(int outputType, int outTypeIdx)
{
	int barIdx = m_table[0].head->barIndex(columnName(outputType, outTypeIdx));
	m_selectedCol = barIdx;
	for (auto row : m_table)
	{
		row.head->setSelectedBar(m_selectedCol);
		row.bars->setSelectedBar(m_selectedCol);
	}
}

void iAParameterInfluenceView::toggleCharacteristic(int charactIdx)
{
	bool shown = m_visibleCharacts.indexOf(qMakePair(outCharacteristic, charactIdx)) != -1;
	toggleBar(!shown, outCharacteristic, charactIdx);
}

void iAParameterInfluenceView::toggleBar(bool show, int outType, int outIdx)
{
	if (show)
	{
		addStackedBar(outType, outIdx);
	}
	else
	{
		removeStackedBar(outType, outIdx);
	}
}

void iAParameterInfluenceView::stackedBarDblClicked(int barIdx)
{
	std::sort(m_sort.begin(), m_sort.end(), [this, barIdx](int r1, int r2)
	{
		return m_table[r1].bars->barValue(barIdx) < m_table[r2].bars->barValue(barIdx);
	});
	LOG(lvlDebug, joinNumbersAsString(m_sort, ","));
	updateTableOrder();
}

void iAParameterInfluenceView::paramChangedSlot()
{
	auto source = qobject_cast<QWidget*>(QObject::sender());
	setSelectedParam(source->property("paramIdx").toInt());
}

void iAParameterInfluenceView::setBarWeights(std::vector<double> const& weights)
{
	for (auto row: m_table)
	{
		row.head->setWeights(weights);
		row.bars->setWeights(weights);
	}
}

void iAParameterInfluenceView::setBarNormalizeMode(bool normalizePerBar)
{
	for (auto row : m_table)
	{
		row.head->setNormalizeMode(normalizePerBar);
		row.bars->setNormalizeMode(normalizePerBar);
	}
}

void iAParameterInfluenceView::setBarDoStack(bool doStack)
{
	for (auto row : m_table)
	{
		row.head->setDoStack(doStack);
		row.bars->setDoStack(doStack);
	}
}


void iAParameterInfluenceView::setSelectedParam(int param)
{
	m_selectedParam = param;
	for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
	{
		// QPalette::Highlight / QPalette::HighlightedText
		QColor color = palette().color(paramIdx == m_selectedParam ? QPalette::Midlight : backgroundRole());
		//QColor textColor = palette().color(paramIdx == m_selectedRow ?  : QPalette::Text);
		for (int col = colMin; col <= colStep; ++col)
		{
			auto item = m_paramListLayout->itemAtPosition(1+RowsPerParam*paramIdx, col);
			if (!item)
			{
				LOG(lvlWarn, "Invalid - empty item!");
				continue;
			}
			item->widget()->setStyleSheet("QLabel { background-color : " + color.name() + "; }");
		}
		m_table[paramIdx].head->setBackgroundColor(color);
		m_table[paramIdx].bars->setBackgroundColor(color);
		for (int barIdx = 0; barIdx < m_table[paramIdx].out.size(); ++barIdx)
		{
			m_table[paramIdx].out[barIdx]->setBackgroundColor(color);
			m_table[paramIdx].out[barIdx]->update();
			m_table[paramIdx].par[barIdx]->setBackgroundColor(color);
			m_table[paramIdx].par[barIdx]->update();
		}
	}
	emit parameterChanged();
}

void getParamMaxMinDiffVal(QVector<double> const & d, double & maxVal, double & minValDiff)
{
	maxVal = std::numeric_limits<double>::lowest();
	minValDiff = std::numeric_limits<double>::max();
	for (int paramIdx = 0; paramIdx < d.size(); ++paramIdx)
	{
		if (d[paramIdx] > maxVal)
		{
			maxVal = d[paramIdx];
		}
		for (int p2 = paramIdx + 1; p2 < d.size(); ++p2)
		{
			auto curDiff = std::abs(d[paramIdx] - d[p2]);
			if (curDiff < minValDiff)
			{
				minValDiff = curDiff;
			}
		}
	}
}

void iAParameterInfluenceView::updateStackedBars()
{
	for (auto col : m_visibleCharacts)
	{
		auto const& d =
		 ((col.first == outCharacteristic)  ? m_sensInf->aggregatedSensitivities[col.second][m_measureIdx] :
		     ((col.first == outFiberCount)  ? m_sensInf->aggregatedSensitivitiesFiberCount
		 /*(col.first == outDissimilarity)*/: m_sensInf->aggregatedSensDissim[col.second]))[m_aggrType];
		// TODO: unify with addStackedBar
		auto title(columnName(col.first, col.second));
		double maxVal, minValDiff;
		getParamMaxMinDiffVal(d, maxVal, minValDiff);
		for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
		{
			m_table[paramIdx].bars->updateBar(title, d[paramIdx], maxVal, minValDiff);
			updateStackedBarHistogram(title, paramIdx, col.first, col.second);
		}
	}
	for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
	{
		m_table[paramIdx].bars->update();
	}
	updateChartY();
}

QString iAParameterInfluenceView::columnName(int outType, int outIdx) const
{
	return  +
		(outType == outCharacteristic) ? QString("Variation ") + m_sensInf->charactName(outIdx) :
		((outType == outFiberCount) ? "Variation Fiber Count"
		/*(outType == outDissimilarity)*/	:
			getAvailableDissimilarityMeasureNames()[m_sensInf->m_resultDissimMeasures[outIdx].first]);
}

void iAParameterInfluenceView::updateStackedBarHistogram(QString const & barName, int paramIdx, int outType, int outIdx)
{
	int barIdx = m_table[paramIdx].bars->barIndex(barName);
	if (outType != outCharacteristic)
	{
		return;
	}
	auto outChart = m_table[paramIdx].out[barIdx];
	outChart->clearPlots();
	const int numBins = m_sensInf->m_histogramBins;
	auto const rng = m_sensInf->m_data->spmData->paramRange(m_sensInf->m_charSelected[outIdx]);
	auto histData = iAHistogramData::create(barName, iAValueType::Continuous, rng[0], rng[1],
		m_sensInf->charHistVarAgg[outIdx][m_aggrType][paramIdx]);
	outChart->resetYBounds();
	outChart->addPlot(QSharedPointer<iABarGraphPlot>::create(histData, QColor(80, 80, 80, 128)));
	outChart->update();

	auto parChart = m_table[paramIdx].par[barIdx];
	parChart->clearPlots();
	auto const& d = ((outType == outCharacteristic) ? m_sensInf->sensitivityField[outIdx][m_measureIdx][m_aggrType]
			: (outType == outFiberCount) ? m_sensInf->sensitivityFiberCount[m_aggrType]
										/* (outputIdx == outDissimilarity)*/
										 : m_sensInf->sensDissimField[outIdx][m_aggrType])[paramIdx];
	auto plotData = iAXYPlotData::create("Sensitivity " + columnName(outType, outIdx), iAValueType::Continuous, d.size());
	for (int i = 0; i < d.size(); ++i)
	{
		plotData->addValue(m_sensInf->paramSetValues[i][m_sensInf->m_variedParams[paramIdx]], d[i]);
	}
	parChart->resetYBounds();
	parChart->addPlot(QSharedPointer<iALinePlot>::create(plotData, QColor(80, 80, 80, 255)));
	parChart->update();
}

void iAParameterInfluenceView::updateChartY()
{
	auto getChartMax = [](QVector<iAChartWidget*> const& c, double yMax) {
		for (auto chart : c)
		{
			yMax = std::max(yMax, chart->yBounds()[1]);
		}
		return yMax;
	};
	auto setChartYBounds = [](QVector<iAChartWidget*> const& c, double yMax) {
		for (auto chart : c)
		{
			chart->setYBounds(0, yMax);
			chart->update();
		}
	};
	double yMaxOut = std::numeric_limits<double>::lowest(),
	       yMaxPar = std::numeric_limits<double>::lowest();
	for (auto chartRow : m_table)
	{
		yMaxOut = getChartMax(chartRow.out, yMaxOut);
		yMaxPar = getChartMax(chartRow.par, yMaxPar);
	}
	for (auto chartRow : m_table)
	{
		setChartYBounds(chartRow.out, yMaxOut);
		setChartYBounds(chartRow.par, yMaxPar);
	}
}

void iAParameterInfluenceView::addStackedBar(int outType, int outIdx)
{
	m_visibleCharacts.push_back(qMakePair(outType, outIdx));
	auto title(columnName(outType, outIdx));
	LOG(lvlDebug, QString("Showing stacked bar for characteristic %1").arg(title));
	auto const& d = (
		   (outType == outCharacteristic)  ? m_sensInf->aggregatedSensitivities[outIdx][m_measureIdx]:
		      ((outType == outFiberCount)  ? m_sensInf->aggregatedSensitivitiesFiberCount
		/*(col.first == outDissimilarity)*/: m_sensInf->aggregatedSensDissim[outIdx]))[m_aggrType];

	int curBarIdx = static_cast<int>(m_table[0].head->numberOfBars());  // not yet added to bars here so no -1
	auto params = m_sensInf->m_variedParams;
	for (int paramIdx = 0; paramIdx < params.size(); ++paramIdx)
	{
		auto paramName = m_sensInf->m_paramNames[paramIdx];
		QColor color = palette().color(paramIdx == m_selectedParam ? QPalette::Midlight : backgroundRole());

		auto outChart = new iAChartWidget(this, "", (curBarIdx == 0) ? "Var. from " + paramName: "");
		outChart->setShowXAxisLabel(false);
		outChart->setEmptyText("");
		outChart->setBackgroundColor(color);
		m_table[paramIdx].out.push_back(outChart);

		auto parChart = new iAChartWidget(this, paramName, (curBarIdx == 0) ? "Sensitivity " + title : "");
		parChart->setEmptyText("");
		parChart->setBackgroundColor(color);
		m_table[paramIdx].par.push_back(parChart);
	}
	double maxVal, minValDiff;
	getParamMaxMinDiffVal(d, maxVal, minValDiff);
	for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
	{
		m_table[paramIdx].head->addBar(title, 1, 1, 1);
		m_table[paramIdx].bars->addBar(title, d[paramIdx], maxVal, minValDiff);
		updateStackedBarHistogram(title, paramIdx, outType, outIdx);
		m_table[paramIdx].head->setLeftMargin(m_table[paramIdx].out[0]->leftMargin());
		m_table[paramIdx].bars->setLeftMargin(m_table[paramIdx].out[0]->leftMargin());
	}
	updateChartY();
	emit barAdded(outType, outIdx);
}

void iAParameterInfluenceView::removeStackedBar(int outType, int outIdx)
{
	int visibleIdx = m_visibleCharacts.indexOf(qMakePair(outType, outIdx));
	if (visibleIdx == -1)
	{
		LOG(lvlError, QString("Invalid state - called remove on non-added bar (outType=%1, outIdx=%2").arg(outType).arg(outIdx));
	}
	m_visibleCharacts.remove(visibleIdx);
	auto title(columnName(outType, outIdx));
	LOG(lvlDebug, QString("Removing stacked bar for characteristic %1").arg(title));
	for (int rowIdx = 0; rowIdx < m_table.size(); ++rowIdx)
	{
		m_table[rowIdx].head->removeBar(title);
		int barIdx = m_table[rowIdx].bars->removeBar(title);
		auto ow = m_table[rowIdx].out[barIdx];
		m_table[rowIdx].out.remove(barIdx);
		delete ow;
		auto pw = m_table[rowIdx].par[barIdx];
		m_table[rowIdx].par.remove(barIdx);
		delete pw;
		m_table[rowIdx].out[0]->setYCaption("Var. from " + m_sensInf->m_paramNames[m_sort[rowIdx]]); // to make sure if first chart is removed that new first gets caption
		int newNumBars = m_table[rowIdx].bars->numberOfBars();
		for (int i = barIdx; i < newNumBars; ++i)
		{   // addWidget automatically removes it from position where it was before
			m_paramListLayout->addWidget(m_table[rowIdx].out[i],
				1+RowsPerParam * rowIdx + RowOutputChart, colStackedBar + i);
			m_paramListLayout->addWidget(m_table[rowIdx].par[i],
				1+RowsPerParam * rowIdx + RowParamChart, colStackedBar + i);
		}
		for (int b = 0; b < newNumBars; ++b)
		{
			m_table[rowIdx].out[b]->resetYBounds();
		}
	}
	updateChartY();
	emit barRemoved(outType, outIdx);
}
