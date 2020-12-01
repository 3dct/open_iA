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
	const int RowsPerParam = 3;

	enum
	{
		RowStackedHeader = 0,
		RowStackedBar = 1,
		RowOutputChart = 2,
		RowParamChart = 3
	};
}

iAParameterInfluenceView::iAParameterInfluenceView(iASensitivityInfo* sensInf, QColor const & paramColor, QColor const & outputColor) :
	m_sensInf(sensInf),
	m_measureIdx(0),
	m_aggrType(0),
	m_selectedRow(-1),
	m_selectedCol(-1),
	m_paramListLayout(new QGridLayout()),
	m_stackedBarTheme(new iASingleColorTheme("OneOutputColorTheme", outputColor)),
	m_outHistoCharts(sensInf->m_variedParams.size())
{
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
		int rowIdx = 1 + RowsPerParam * paramIdx;
		auto header = new iAStackedBarChart(m_stackedBarTheme.data(), m_paramListLayout,
			rowIdx + RowStackedHeader, colStackedBar, true);
		header->setDoStack(false);
		header->setNormalizeMode(false);
		connect(header, &iAStackedBarChart::barDblClicked, this, &iAParameterInfluenceView::stackedBarDblClicked);
		QString paramName = sensInf->m_paramNames[sensInf->m_variedParams[paramIdx]];
		auto bars = new iAStackedBarChart(m_stackedBarTheme.data(), m_paramListLayout,
			rowIdx + RowStackedBar, colStackedBar, false,
			paramIdx == sensInf->m_variedParams.size() - 1);
		bars->setNormalizeMode(false);
		bars->setDoStack(false);
		m_stackedBars.push_back(qMakePair(header, bars));
		bars->setProperty("paramIdx", paramIdx);
		connect(bars, &iAStackedBarChart::clicked, this, &iAParameterInfluenceView::paramChangedSlot);
		connect(header, &iAStackedBarChart::weightsChanged      , this, &iAParameterInfluenceView::setBarWeights);
		connect(header, &iAStackedBarChart::normalizeModeChanged, this, &iAParameterInfluenceView::setBarNormalizeMode);
		connect(header, &iAStackedBarChart::switchedStackMode   , this, &iAParameterInfluenceView::setBarDoStack);
		auto const& paramVec = sensInf->m_paramValues[sensInf->m_variedParams[paramIdx]];
		double minVal = *std::min_element(paramVec.begin(), paramVec.end()),
			maxVal = *std::max_element(paramVec.begin(), paramVec.end());
		iAClickableLabel* labels[4];
		labels[colParamName] = new iAClickableLabel(paramName);
		labels[colParamName]->setStyleSheet("QLabel { background-color : " + paramColor.name() + "; }");
		labels[colMin] = new iAClickableLabel(QString::number(minVal, 'f', digitsAfterComma(sensInf->paramStep[paramIdx])));
		labels[colMax] = new iAClickableLabel(QString::number(maxVal, 'f', digitsAfterComma(sensInf->paramStep[paramIdx])));
		labels[colStep] = new iAClickableLabel(QString::number(sensInf->paramStep[paramIdx]));
		for (int i = colParamName; i <= colStep; ++i)
		{
			labels[i]->setProperty("paramIdx", paramIdx);
			m_paramListLayout->addWidget(labels[i], rowIdx, i, RowsPerParam, 1);
			connect(labels[i], &iAClickableLabel::clicked, this, &iAParameterInfluenceView::paramChangedSlot);
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
}

void iAParameterInfluenceView::addColumnAction(int objectType, int charactIdx, bool checked)
{
	auto charactShowAction = new QAction(columnName(objectType, charactIdx), nullptr);
	charactShowAction->setProperty("objectType", objectType);
	charactShowAction->setProperty("charactIdx", charactIdx);
	charactShowAction->setCheckable(true);
	charactShowAction->setChecked(checked);
	QObject::connect(charactShowAction, &QAction::triggered, this, &iAParameterInfluenceView::showStackedBar);
	for (auto bars : m_stackedBars)
	{
		bars.first->contextMenu()->addAction(charactShowAction);
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
int iAParameterInfluenceView::selectedRow() const { return m_selectedRow; }
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
	int barIdx = m_stackedBars[0].first->barIndex(columnName(outputType, outTypeIdx));
	m_selectedCol = barIdx;
	for (auto bars : m_stackedBars)
	{
		bars.first->setSelectedBar(m_selectedCol);
		bars.second->setSelectedBar(m_selectedCol);
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
	QString barName = m_stackedBars[0].first->barName(barIdx);
	QPair<int, int> selected;
	for (auto outTypeIdx : m_visibleCharacts)
	{
		if (columnName(outTypeIdx.first, outTypeIdx.second) == barName)
		{
			selected = outTypeIdx;
			break;
		}
	}
	selectStackedBar(selected.first, selected.second);
	emit outputSelected(selected.first, selected.second);
}

void iAParameterInfluenceView::paramChangedSlot()
{
	auto source = qobject_cast<QWidget*>(QObject::sender());
	setSelectedParam(source->property("paramIdx").toInt());
}

void iAParameterInfluenceView::setBarWeights(std::vector<double> const& weights)
{
	for (auto barPair: m_stackedBars)
	{
		barPair.first->setWeights(weights);
		barPair.second->setWeights(weights);
	}
}

void iAParameterInfluenceView::setBarNormalizeMode(bool normalizePerBar)
{
	for (auto barPair : m_stackedBars)
	{
		barPair.first->setNormalizeMode(normalizePerBar);
		barPair.second->setNormalizeMode(normalizePerBar);
	}
}

void iAParameterInfluenceView::setBarDoStack(bool doStack)
{
	for (auto barPair : m_stackedBars)
	{
		barPair.first->setDoStack(doStack);
		barPair.second->setDoStack(doStack);
	}
}


void iAParameterInfluenceView::setSelectedParam(int param)
{
	m_selectedRow = param;
	for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
	{
		// QPalette::Highlight / QPalette::HighlightedText
		QColor color = palette().color(paramIdx == m_selectedRow ? QPalette::Midlight : backgroundRole());
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
		m_stackedBars[paramIdx].first->setBackgroundColor(color);
		m_stackedBars[paramIdx].second->setBackgroundColor(color);
		for (int barIdx = 0; barIdx < m_outHistoCharts[paramIdx].size(); ++barIdx)
		{
			m_outHistoCharts[paramIdx][barIdx]->setBackgroundColor(color);
			m_outHistoCharts[paramIdx][barIdx]->update();
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
			m_stackedBars[paramIdx].second->updateBar(title, d[paramIdx], maxVal, minValDiff);
			updateStackedBarHistogram(title, paramIdx, col.first, col.second);
		}
	}
	for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
	{
		m_stackedBars[paramIdx].second->update();
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
	int barIdx = m_stackedBars[paramIdx].second->barIndex(barName);
	auto chart = m_outHistoCharts[paramIdx][barIdx];
	if (outType != outCharacteristic)
	{
		return;
	}
	chart->clearPlots();
	const int numBins = m_sensInf->m_histogramBins;
	auto const rng = m_sensInf->m_data->spmData->paramRange(m_sensInf->m_charSelected[outIdx]);
	auto plotData = iAHistogramData::create(barName, iAValueType::Continuous, rng[0], rng[1],
		m_sensInf->charHistVarAgg[outIdx][m_aggrType][paramIdx]);
	chart->addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(plotData, QColor(80, 80, 80, 128))));
	chart->resetYBounds();
	chart->update();
}

void iAParameterInfluenceView::updateChartY()
{
	double yMin = 0, yMax = std::numeric_limits<double>::lowest();
	for (auto chartRow : m_outHistoCharts)
	{
		for (auto chart : chartRow)
		{
			yMax = std::max(yMax, chart->yBounds()[1]);
		}
	}
	for (auto chartRow : m_outHistoCharts)
	{
		for (auto chart : chartRow)
		{
			chart->setYBounds(yMin, yMax);
			chart->update();
		}
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

	int curBarIdx = static_cast<int>(m_stackedBars[0].first->numberOfBars()); // not yet added to bars here so no -1
	auto params = m_sensInf->m_variedParams;
	for (int paramIdx = 0; paramIdx < params.size(); ++paramIdx)
	{
		auto paramName = m_sensInf->m_paramNames[paramIdx];
		auto chart = new iAChartWidget(this, "", (curBarIdx == 0) ? "Var. from " + paramName: "");
		chart->setShowXAxisLabel(false);
		chart->setEmptyText("");
		QColor color = palette().color(paramIdx == m_selectedRow ? QPalette::Midlight : backgroundRole());
		chart->setBackgroundColor(color);
		m_paramListLayout->addWidget(chart, 1+RowsPerParam * paramIdx + RowOutputChart, colStackedBar + curBarIdx);
		m_outHistoCharts[paramIdx].push_back(chart);
	}
	double maxVal, minValDiff;
	getParamMaxMinDiffVal(d, maxVal, minValDiff);
	for (int paramIdx = 0; paramIdx < m_sensInf->m_variedParams.size(); ++paramIdx)
	{
		m_stackedBars[paramIdx].first->addBar(title, 1, 1, 1);
		m_stackedBars[paramIdx].second->addBar(title, d[paramIdx], maxVal, minValDiff);
		updateStackedBarHistogram(title, paramIdx, outType, outIdx);
		m_stackedBars[paramIdx].first->setLeftMargin(m_outHistoCharts[paramIdx][0]->leftMargin());
		m_stackedBars[paramIdx].second->setLeftMargin(m_outHistoCharts[paramIdx][0]->leftMargin());
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
	for (int paramIdx = 0; paramIdx < m_stackedBars.size(); ++paramIdx)
	{
		m_stackedBars[paramIdx].first->removeBar(title);
		int barIdx = m_stackedBars[paramIdx].second->removeBar(title);
		auto w = m_outHistoCharts[paramIdx][barIdx];
		m_outHistoCharts[paramIdx].remove(barIdx);
		delete w;
		m_outHistoCharts[paramIdx][0]->setYCaption("Var. from " + m_sensInf->m_paramNames[paramIdx]); // to make sure if first chart is removed that new first gets caption
		int newNumBars = m_stackedBars[paramIdx].second->numberOfBars();
		for (int i = barIdx; i < newNumBars; ++i)
		{   // addWidget automatically removes it from position where it was before
			m_paramListLayout->addWidget(m_outHistoCharts[paramIdx][i],
				1+RowsPerParam * paramIdx + RowOutputChart, colStackedBar + i);
		}
		for (int b = 0; b < newNumBars; ++b)
		{
			m_outHistoCharts[paramIdx][b]->resetYBounds();
		}
	}
	updateChartY();
	emit barRemoved(outType, outIdx);
}
