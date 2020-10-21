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

#include <iAColorTheme.h>
#include <iAConsole.h>

#include "iASensitivityInfo.h"
#include "iAStackedBarChart.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>

namespace// merge with iASensitivityinfo!
{
	int LayoutSpacing = 4; int LayoutMargin = 4; 
	const QString DefaultStackedBarColorTheme("Brewer Accent (max. 8)");

	void addColumnAction(QString const & name, int charactIdx, iAParameterInfluenceView* view, iAStackedBarChart* stackedHeader)
	{
		auto charactShowAction = new QAction(name, nullptr);
		charactShowAction->setProperty("charactIdx", static_cast<unsigned long long>(charactIdx));
		charactShowAction->setCheckable(true);
		charactShowAction->setChecked(false);
		QObject::connect(charactShowAction, &QAction::triggered, view, &iAParameterInfluenceView::stackedColSelect);
		stackedHeader->contextMenu()->addAction(charactShowAction);
	}
}

iAParameterInfluenceView::iAParameterInfluenceView(iASensitivityInfo* sensInf) :
	m_sensInf(sensInf),
	m_measureIdx(0),
	m_aggrType(0),
	m_paramIdx(0)
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
	enum ColumnIndices { colParamName=0, colMin=1, colMax=2, colStep=3, colStackedBar=4 };
	paramListLayout->setColumnStretch(colParamName, 1);
	paramListLayout->setColumnStretch(colMin, 1);
	paramListLayout->setColumnStretch(colMax, 1);
	paramListLayout->setColumnStretch(colStep, 1);
	paramListLayout->setColumnStretch(colStackedBar, 3);

	auto colorTheme = iAColorThemeManager::instance().theme(DefaultStackedBarColorTheme);
	m_stackedHeader = new iAStackedBarChart(colorTheme, true);
	// TODO: Unify/Group stacked bar widgets here / in iAFIAKERController into a class
	// which encapsulates updating weights, showing columns, unified data interface (table?)
	// for all characteristics, add column to stacked bar charts

	connect(m_stackedHeader, &iAStackedBarChart::switchedStackMode, this, &iAParameterInfluenceView::switchStackMode);

	for (size_t charactIdx = 0; charactIdx < sensInf->charactIndex.size(); ++charactIdx)
	{
		addColumnAction(sensInf->charactName(charactIdx), charactIdx, this, m_stackedHeader);
	}
	addColumnAction("Fiber Count", sensInf->charactIndex.size() + 1, this, m_stackedHeader);
	DEBUG_LOG(QString("Adding lines for %1 characteristics").arg(sensInf->charactIndex.size()));
	addHeaderLabel(paramListLayout, colParamName, "Parameter");
	addHeaderLabel(paramListLayout, colMin, "Min");
	addHeaderLabel(paramListLayout, colMax, "Max");
	addHeaderLabel(paramListLayout, colStep, "Step");
	paramListLayout->addWidget(m_stackedHeader, 0, colStackedBar);
	for (int paramIdx = 0; paramIdx < sensInf->variedParams.size(); ++paramIdx)
	{
		m_stackedCharts.push_back(new iAStackedBarChart(colorTheme, false));
		connect(m_stackedHeader, &iAStackedBarChart::weightsChanged, m_stackedCharts[paramIdx], &iAStackedBarChart::setWeights);
		auto label = new QLabel(sensInf->paramNames[sensInf->variedParams[paramIdx]]);
		label->setProperty("paramIdx", paramIdx);
		m_stackedCharts[paramIdx]->setProperty("paramIdx", paramIdx);
		connect(m_stackedCharts[paramIdx], &iAStackedBarChart::doubleClicked, this, &iAParameterInfluenceView::paramChangedSlot);
		connect(m_stackedHeader, &iAStackedBarChart::normalizeModeChanged, m_stackedCharts[paramIdx], &iAStackedBarChart::setNormalizeMode);
		paramListLayout->addWidget(label, 1 + paramIdx, 0);
		auto const& paramVec = sensInf->allParamValues[sensInf->variedParams[paramIdx]];
		double minVal = *std::min_element(paramVec.begin(), paramVec.end()),
			maxVal = *std::max_element(paramVec.begin(), paramVec.end());
		paramListLayout->addWidget(new QLabel(QString::number(minVal)), 1 + paramIdx, colMin);
		paramListLayout->addWidget(new QLabel(QString::number(maxVal)), 1 + paramIdx, colMax);
		paramListLayout->addWidget(new QLabel(QString::number(sensInf->paramStep[paramIdx])), 1 + paramIdx, colStep);
		paramListLayout->addWidget(m_stackedCharts[paramIdx], 1 + paramIdx, colStackedBar);
	}
}

void iAParameterInfluenceView::changeMeasure(int newMeasure)
{
	m_measureIdx = newMeasure;
	updateStackedBars();
	emit parameterChanged();
}

void iAParameterInfluenceView::changeAggregation(int newAggregation)
{
	m_aggrType = newAggregation;
	updateStackedBars();
	emit parameterChanged();
}

int iAParameterInfluenceView::selectedMeasure() const { return m_measureIdx; }
int iAParameterInfluenceView::selectedAggrType() const { return m_aggrType; }
int iAParameterInfluenceView::selectedParam() const { return m_paramIdx; }


void iAParameterInfluenceView::setColorTheme(iAColorTheme const * colorTheme)
{
	m_stackedHeader->setColorTheme(colorTheme);
	for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
	{
		m_stackedCharts[paramIdx]->setColorTheme(colorTheme);
	}
}

void iAParameterInfluenceView::stackedColSelect()
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
void iAParameterInfluenceView::selectMeasure(int measureIdx)
{
	m_measureIdx = measureIdx;
	//updateStackedBars();
	emit parameterChanged();
}

void iAParameterInfluenceView::paramChangedSlot()
{
	auto source = qobject_cast<iAStackedBarChart*>(QObject::sender());
	m_paramIdx = source->property("paramIdx").toInt();
	for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
	{
		m_stackedCharts[paramIdx]->setBackgroundColor(paramIdx == m_paramIdx ? QColor(200, 200, 200) : QColor(255, 255, 255));
		m_stackedCharts[paramIdx]->update();
	}
	emit parameterChanged();
}

void iAParameterInfluenceView::switchStackMode(bool stack)
{
	for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
	{
		m_stackedCharts[paramIdx]->setDoStack(stack);
	}
}

void iAParameterInfluenceView::updateStackedBars()
{
	for (auto charactIdx : m_visibleCharacts)
	{
		// TODO: unify with addStackedBar
		auto title(m_sensInf->charactName(charactIdx));
		double maxValue = std::numeric_limits<double>::lowest();
		for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
		{
			double value = (charactIdx < m_sensInf->aggregatedSensitivities.size())?
				m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType] :
				m_sensInf->aggregatedSensitivitiesFiberCount[paramIdx][m_aggrType];
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
			double value = (charactIdx < m_sensInf->aggregatedSensitivities.size()) ?
				m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType] :
				m_sensInf->aggregatedSensitivitiesFiberCount[paramIdx][m_aggrType];
			m_stackedCharts[paramIdx]->updateBar(title, value, maxValue);
		}
	}
	for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
	{
		m_stackedCharts[paramIdx]->update();
	}
}

void iAParameterInfluenceView::addStackedBar(int charactIdx)
{
	m_visibleCharacts.push_back(charactIdx);
	auto title(charactIdx < m_sensInf->aggregatedSensitivities.size() ?
		m_sensInf->charactName(charactIdx): "Fiber Count");
	DEBUG_LOG(QString("Showing stacked bar for characteristic %1").arg(title));
	m_stackedHeader->addBar(title, 1, 1);

	double maxValue = std::numeric_limits<double>::lowest();
	for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
	{
		double value = (charactIdx < m_sensInf->aggregatedSensitivities.size()) ?
			m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType] :
			m_sensInf->aggregatedSensitivitiesFiberCount[paramIdx][m_aggrType];
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
		double value = (charactIdx < m_sensInf->aggregatedSensitivities.size()) ?
			m_sensInf->aggregatedSensitivities[charactIdx][paramIdx][m_measureIdx][m_aggrType] :
			m_sensInf->aggregatedSensitivitiesFiberCount[paramIdx][m_aggrType];
		m_stackedCharts[paramIdx]->addBar(title, value, maxValue);
	}
}

void iAParameterInfluenceView::removeStackedBar(int charactIdx)
{
	int visibleIdx = m_visibleCharacts.indexOf(charactIdx);
	if (visibleIdx == -1)
	{
		DEBUG_LOG(QString("Invalid state - called remove on non-added characteristic idx %1").arg(charactIdx));
	}
	m_visibleCharacts.remove(visibleIdx);
	auto title(m_sensInf->charactName(charactIdx));
	DEBUG_LOG(QString("Showing stacked bar for characteristic %1").arg(title));
	m_stackedHeader->removeBar(title);
	for (size_t paramIdx = 0; paramIdx < m_sensInf->variedParams.size(); ++paramIdx)
	{
		m_stackedCharts[paramIdx]->removeBar(title);
	}
}
