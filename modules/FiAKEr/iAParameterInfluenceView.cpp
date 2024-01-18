// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAParameterInfluenceView.h"

#include "iAFiberResult.h"
#include "iAFiberData.h" // for getAvailableDissimilarityMeasureNames
#include "iASensitivityData.h"
#include "iAStackedBarChart.h"
#include "iAClickableLabel.h"

#include <iAColorTheme.h>
#include <iALog.h>

// charts:
#include "iALUT.h"

#include <iAChartWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>
#include <iAScatterPlotWidget.h>
#include <iASPLOMData.h>

#include <QAction>
#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>

namespace
{
	enum ColumnIndices
	{
		colParamName = 0,
		colMin = 1,
		colMax = 2,
		colStep = 3,
		colStackedBar = 4
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

	const QColor VariationHistogramColor(50, 50, 50, 255);
	const QColor AverageHistogramColor(150, 150, 150, 255);

	QColor barGraphColor(QColor const& color)
	{
		QColor c(color);
		c.setAlpha(64);  // we need a transparent color for bars
		return c;
	}
}

class iAParTableRow
{
public:
	iAStackedBarChart* head;
	iAStackedBarChart* bars;
	QVector<iAChartWidget*> out;
	QVector<iAScatterPlotWidget*> par;
	iAClickableLabel* labels[LabelCount];
};

iAParameterInfluenceView::iAParameterInfluenceView(std::shared_ptr<iASensitivityData> data,
	std::shared_ptr<iASensitivityViewState> viewState, QColor const& inColor, QColor const& outColor) :
	m_data(data),
	m_viewState(viewState),
	m_measureIdx(0),
	m_charDiffMeasureIdx(0),
	m_aggrType(0),
	m_selectedParam(-1),
	m_selectedCol(-1),
	m_paramListLayout(new QGridLayout()),
	m_stackedBarTheme(std::make_shared<iASingleColorTheme>("OneOutputColorTheme", outColor)),
	m_table(data->m_variedParams.size()),
	m_sort(data->m_variedParams.size()),
	m_sortLastOut(-1),
	m_sortLastDesc(true),
	m_histogramChartType("Bars"),    // needs to match value from radio buttons in SensitivitySettings.ui
	m_normalizePerOutput(false),
	m_sortParamLUT(std::make_shared<iALookupTable>(QColor(64, 64, 64))),
	m_spColorMapName("Matplotlib: Plasma")
{
	for (int i=0; i<m_sort.size(); ++i)
	{
		m_sort[i] = i;
	}
	for (int j=0; j<data->m_variedParams.size(); ++j)
	{
		m_table[j] = std::make_shared<iAParTableRow>();
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
	// TODO: Unify/Group stacked bar widgets here / in iAFIAKERController into a class
	// which encapsulates updating weights, showing columns, unified data interface (table?)
	// for all characteristics, add column to stacked bar charts

	//LOG(lvlDebug, QString("Adding lines for %1 characteristics").arg(sensInf->m_charSelected.size()));

	// headers:
	addHeaderLabel(m_paramListLayout, colParamName, "Param.", QSizePolicy::Fixed);
	addHeaderLabel(m_paramListLayout, colMin, "Min", QSizePolicy::Fixed);
	addHeaderLabel(m_paramListLayout, colMax, "Max", QSizePolicy::Fixed);
	addHeaderLabel(m_paramListLayout, colStep, "Step", QSizePolicy::Fixed);

	for (int paramIdx = 0; paramIdx < data->m_variedParams.size(); ++paramIdx)
	{
		iAParTableRow * row = m_table[paramIdx].get();
		int rowIdx = 1 + RowsPerParam * paramIdx;
		row->head = new iAStackedBarChart(m_stackedBarTheme.get(), m_paramListLayout,
			rowIdx + RowStackedHeader, colStackedBar, true);
		row->head->setDoStack(false);
		row->head->setNormalizeMode(false);
		connect(row->head, &iAStackedBarChart::barDblClicked, this, &iAParameterInfluenceView::sortListByBar);
		QString paramName = data->m_paramNames[data->m_variedParams[paramIdx]];
		row->bars = new iAStackedBarChart(m_stackedBarTheme.get(), m_paramListLayout,
			rowIdx + RowStackedBar, colStackedBar, false,
			paramIdx == data->m_variedParams.size() - 1);
		row->bars->setNormalizeMode(false);
		row->bars->setDoStack(false);
		row->bars->setProperty("paramIdx", paramIdx);
		connect(row->bars, &iAStackedBarChart::clicked, this, &iAParameterInfluenceView::paramChangedSlot);
		connect(row->head, &iAStackedBarChart::weightsChanged      , this, &iAParameterInfluenceView::setBarWeights);
		connect(row->head, &iAStackedBarChart::normalizeModeChanged, this, &iAParameterInfluenceView::setBarNormalizeMode);
		connect(row->head, &iAStackedBarChart::switchedStackMode   , this, &iAParameterInfluenceView::setBarDoStack);
		auto const& paramVec = data->m_paramValues[data->m_variedParams[paramIdx]];
		double minVal = *std::min_element(paramVec.begin(), paramVec.end()),
			maxVal = *std::max_element(paramVec.begin(), paramVec.end());
		row->labels[colParamName] = new iAClickableLabel(paramName, true);
		row->labels[colMin] = new iAClickableLabel(QString::number(minVal, 'f', digitsAfterComma(data->paramStep[paramIdx])), false);
		row->labels[colMax] = new iAClickableLabel(QString::number(maxVal, 'f', digitsAfterComma(data->paramStep[paramIdx])), false);
		row->labels[colStep] = new iAClickableLabel(QString::number(data->paramStep[paramIdx]), false);
		for (int i = colParamName; i <= colStep; ++i)
		{
			row->labels[i]->setProperty("paramIdx", paramIdx);
			row->labels[i]->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
			connect(row->labels[i], &iAClickableLabel::clicked, this, &iAParameterInfluenceView::paramChangedSlot);
		}
		//m_paramListLayout->addWidget(m_stackedCharts[paramIdx], rowIdx, colStackedBar);

		//m_diffChart.push_back(new iAChartWidget(this, "Characteristics distribution", ));
		//m_paramListLayout->addWidget(m_diffChart[paramIdx], 1 + paramIdx, colHistogram);
	}
	setInColor(inColor);
	for (int charactIdx = 0; charactIdx < data->m_charSelected.size(); ++charactIdx)
	{
		addColumnAction(outCharacteristic, charactIdx, charactIdx == 0);
	}
	addColumnAction(outFiberCount, -1, false);
	for (size_t dissimMeasIdx = 0; dissimMeasIdx < data->m_resultDissimMeasures.size(); ++dissimMeasIdx)
	{
		addColumnAction(outDissimilarity, static_cast<int>(dissimMeasIdx), false);
	}

	addStackedBar(outCharacteristic, 0);  // reflected in iAAlgorithmInfo construction in iASensitivity -> put in common place?

	sortListByBar(0);
}

void iAParameterInfluenceView::addTableWidgets()
{
	for (int paramRow = 0; paramRow < m_table.size(); ++paramRow)
	{
		int paramIdx = m_sort[paramRow];
		int rowIdx = 1 + RowsPerParam * paramRow;
		for (int i=0; i<LabelCount; ++i)
		{
			m_paramListLayout->addWidget(m_table[paramIdx]->labels[i], rowIdx, i, RowsPerParam, 1);
		}
		m_table[paramIdx]->head->setPos(rowIdx+RowStackedHeader, colStackedBar);
		m_table[paramIdx]->bars->setPos(rowIdx+RowStackedBar, colStackedBar);
		for (int c=0; c<m_table[paramIdx]->out.size(); ++c)
		{
			bool outVisible = m_highlightedParams.contains(paramIdx);
			m_paramListLayout->addWidget(m_table[paramIdx]->out[c], rowIdx + RowOutputChart, colStackedBar + c);
			m_paramListLayout->setRowStretch(rowIdx + RowOutputChart, outVisible ? 4 : 0);
			m_table[paramIdx]->out[c]->setMinimumHeight(outVisible ? 80 : 0);
			m_table[paramIdx]->out[c]->setMaximumHeight(outVisible ? 500 : 0);
			m_table[paramIdx]->out[c]->setVisible(outVisible);

			bool parVisible = paramRow < 2 || paramIdx == m_selectedParam;
			m_paramListLayout->addWidget(m_table[paramIdx]->par[c], rowIdx + RowParamChart, colStackedBar + c);
			m_paramListLayout->setRowStretch(rowIdx + RowParamChart, parVisible ? 4 : 0);
			m_table[paramIdx]->par[c]->setMinimumHeight(parVisible ? 80 : 0);
			m_table[paramIdx]->par[c]->setMaximumHeight(parVisible ? 500 : 0);
			m_table[paramIdx]->par[c]->setVisible(parVisible);
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
		row->head->contextMenu()->addAction(charactShowAction);
	}
}

void iAParameterInfluenceView::setDistributionMeasure(int newMeasure)
{
	m_measureIdx = newMeasure;
	updateStackedBars();
	emit parameterChanged();		// not sure why this is emitted here - selected input parameter should not have changed?
}

void iAParameterInfluenceView::setCharDiffMeasure(int newMeasure)
{
	m_charDiffMeasureIdx = newMeasure;
	updateStackedBars();
	emit parameterChanged();  // not sure why this is emitted here - selected input parameter should not have changed?
}

void iAParameterInfluenceView::setAggregation(int newAggregation)
{
	m_aggrType = newAggregation;
	updateStackedBars();
	emit parameterChanged();  // not sure why this is emitted here - selected input parameter should not have changed?
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
	int barIdx = m_table[0]->head->barIndex(columnName(outputType, outTypeIdx));
	m_selectedCol = barIdx;
	for (auto row : m_table)
	{
		row->head->setSelectedBar(m_selectedCol);
		row->bars->setSelectedBar(m_selectedCol);
	}
}

void iAParameterInfluenceView::toggleCharacteristic(int charactIdx)
{
	bool shown = m_visibleCharacts.indexOf(qMakePair(static_cast<int>(outCharacteristic), charactIdx)) != -1;
	toggleBar(!shown, outCharacteristic, charactIdx);
}

void iAParameterInfluenceView::setSPParameterColorMap(QString const& colorMapName)
{
	m_spColorMapName = colorMapName;
	updateSPColoring();
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

void iAParameterInfluenceView::sortListByBar(int barIdx)
{
	if (m_sortLastOut == barIdx)
	{
		m_sortLastDesc = !m_sortLastDesc;
	}
	m_sortLastOut = barIdx;
	std::sort(m_sort.begin(), m_sort.end(), [this, barIdx](int r1, int r2)
	{
		double v1 = m_table[r1]->bars->barValue(barIdx),
			v2 = m_table[r2]->bars->barValue(barIdx);
		return m_sortLastDesc ? v1 > v2 : v1 < v2;
	});
	//LOG(lvlDebug, joinNumbersAsString(m_sort, ","));
	addTableWidgets();

	updateSPColoring();

	//parChart->setLookupTable( iALUT::Build());
	emit orderChanged(m_sort);
}

void iAParameterInfluenceView::updateSPColoring()
{
	// update coloring of scatter plots by highest-sorteed parameter
	auto firstParam = m_data->m_variedParams[m_sort[0]];
	auto const& pset = m_data->paramSetValues;
	double maxRange[2] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
	for (int i = 0; i < pset.size(); ++i)
	{
		auto val = pset[i][firstParam];
		if (val < maxRange[0])
		{
			maxRange[0] = val;
		}
		if (val > maxRange[1])
		{
			maxRange[1] = val;
		}
	}
	*m_sortParamLUT = iALUT::Build(maxRange, m_spColorMapName, 5, 1.0);

	for (int rowIdx = 0; rowIdx < m_table.size(); ++rowIdx)
	{
		for (int c = 0; c < m_table[rowIdx]->par.size(); ++c)
		{
			auto spData = m_table[rowIdx]->par[c]->data();
			for (int i = 0; i < pset.size(); ++i)
			{
				spData->data()[2][i] = pset[i][firstParam];
			}
			spData->updateRange(2);
			m_table[rowIdx]->par[c]->update();
		}
	}
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
		row->head->setWeights(weights);
		row->bars->setWeights(weights);
	}
}

void iAParameterInfluenceView::setBarNormalizeMode(bool normalizePerBar)
{
	for (auto row : m_table)
	{
		row->head->setNormalizeMode(normalizePerBar);
		row->bars->setNormalizeMode(normalizePerBar);
	}
}

void iAParameterInfluenceView::setBarDoStack(bool doStack)
{
	for (auto row : m_table)
	{
		row->head->setDoStack(doStack);
		row->bars->setDoStack(doStack);
	}
}

void iAParameterInfluenceView::addResultHistoPlot(size_t resultIdx, int paramIdx, int barIdx, QColor c)
{
	if (m_visibleCharacts[barIdx].first != outCharacteristic)
	{
		return;
	}
	int charIdx = m_visibleCharacts[barIdx].second;
	auto const rng = m_data->m_data->spmData->paramRange(m_data->m_charSelected[charIdx]);
	auto histData = iAHistogramData::create(QString("Result %1").arg(resultIdx), iAValueType::Continuous, rng[0],
		rng[1], m_data->m_charHistograms[resultIdx][charIdx]);
	auto plotKey = std::make_tuple(resultIdx, paramIdx, charIdx);
	m_selectedResultHistoPlots.insert(plotKey, createHistoPlot(histData, c));
	m_table[paramIdx]->out[barIdx]->addPlot(m_selectedResultHistoPlots[plotKey]);
}

void iAParameterInfluenceView::setResultSelected(size_t resultIdx, bool state, QColor c)
{
	size_t inGroupIdx = resultIdx % m_data->m_starGroupSize;
	int branchIdx = (inGroupIdx != 0) ? (inGroupIdx - 1) % m_data->m_numOfSTARSteps : 0;
	size_t startResult = resultIdx - branchIdx;
	size_t starCenter = (resultIdx / m_data->m_starGroupSize) * m_data->m_starGroupSize;
	for (int paramIdx = 0; paramIdx < m_data->m_variedParams.size(); ++paramIdx)
	{
		//double paramValue = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][resultIdx];
		for (int barIdx = 0; barIdx < m_table[paramIdx]->out.size(); ++barIdx)
		{
			if (m_visibleCharacts[barIdx].first == outCharacteristic)
			{
				int charIdx = m_visibleCharacts[barIdx].second;
				auto plotKey = std::make_tuple(resultIdx, paramIdx, charIdx);
				if (state)
				{
					if (inGroupIdx != 0)
					{
						for (size_t r = 0; static_cast<int>(r) < m_data->m_numOfSTARSteps; ++r)
						{
							auto rIdx = startResult + r;
							double pv = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][rIdx];
							m_table[paramIdx]->par[barIdx]->setXMarker(
								pv, (rIdx == resultIdx) ? c : QColor(192, 192, 192), Qt::DashLine);
						}
					}
					double pv = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][starCenter];
					m_table[paramIdx]->par[barIdx]->setXMarker(pv, inGroupIdx != 0 ? QColor(192, 192, 192) : c, Qt::DashLine);
					if (m_selectedResultHistoPlots.contains(plotKey))
					{
						LOG(lvlWarn, QString("Plot to be added already exists!"));
					}
					else
					{
						addResultHistoPlot(resultIdx, paramIdx, barIdx, c);
					}
				}
				else
				{
					if (inGroupIdx != 0)
					{
						for (size_t r = 0; static_cast<int>(r) < m_data->m_numOfSTARSteps; ++r)
						{
							auto rIdx = startResult + r;
							double pv = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][rIdx];
							m_table[paramIdx]->par[barIdx]->removeXMarker(pv);
						}
					}
					double pv = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][starCenter];
					m_table[paramIdx]->par[barIdx]->removeXMarker(pv);
					if (!m_selectedResultHistoPlots.contains(plotKey))
					{
						LOG(lvlWarn, QString("Plot to be removed does not exist!"));
					}
					else
					{
						m_table[paramIdx]->out[barIdx]->removePlot(m_selectedResultHistoPlots[plotKey]);
						m_selectedResultHistoPlots.remove(plotKey);
					}
				}
			}
		}
	}
}

void iAParameterInfluenceView::updateHighlightColors(std::vector<size_t> highlighted, iAColorTheme const * theme)
{
	for (size_t i=0; i<highlighted.size(); ++i)
	{
		size_t resultIdx = highlighted[i];
		for (int paramIdx = 0; paramIdx < m_data->m_variedParams.size(); ++paramIdx)
		{
			double paramValue = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][resultIdx];
			for (int barIdx = 0; barIdx < m_table[paramIdx]->out.size(); ++barIdx)
			{
				if (m_visibleCharacts[barIdx].first == outCharacteristic)
				{
					int charIdx = m_visibleCharacts[barIdx].second;
					m_table[paramIdx]->par[barIdx]->setXMarker(paramValue, theme->color(i), Qt::DashLine);
					auto plotKey = std::make_tuple(resultIdx, paramIdx, charIdx);
					m_selectedResultHistoPlots[plotKey]->setColor(
						m_histogramChartType == "Bars" ? barGraphColor(theme->color(i)) : theme->color(i));
				}

			}
		}
	}
}

void iAParameterInfluenceView::setHighlightedParams(QSet<int> hiParam)
{
	if (hiParam.size() == 0)
	{
		return;
	}
	m_highlightedParams = hiParam;
	addTableWidgets();
}

void iAParameterInfluenceView::setNormalizePerOutput(bool norm)
{
	m_normalizePerOutput = norm;
	updateStackedBars();
}

void iAParameterInfluenceView::paramChartClicked(double x, double /* y */, Qt::KeyboardModifiers modifiers)
{
	// search for parameter value "closest" to clicked x;
	auto chart = qobject_cast<iAScatterPlotWidget*>(QObject::sender());
	int variedParamIdx = chart->property("paramIdx").toInt();
	auto& paramValues = m_data->m_paramValues[m_data->m_variedParams[variedParamIdx]];
	auto minDistElem = std::min_element(paramValues.begin(), paramValues.end(), [x](double a, double b) {
		return std::abs(a - x) < std::abs(b - x);
	});
	// select the result that is closest to the currently selected one in the other parameters!
	int resultIdx = minDistElem - paramValues.begin();
	emit resultSelected(resultIdx, modifiers);
}

void iAParameterInfluenceView::setSelectedParam(int param)
{
	m_selectedParam = param;
	for (int paramIdx = 0; paramIdx < m_data->m_variedParams.size(); ++paramIdx)
	{
		//QColor color = (paramIdx == m_selectedParam) ? ParamRowSelectedBGColor : ParamRowUnselectedBGColor;
		QPalette::ColorRole bgRole = (paramIdx == m_selectedParam) ? QPalette::AlternateBase : QPalette::Window;
		for (int col = colMin; col <= colStep; ++col)
		{
			m_table[paramIdx]->labels[col]->setAutoFillBackground(true);
			m_table[paramIdx]->labels[col]->setPalette(QApplication::palette());
			m_table[paramIdx]->labels[col]->setBackgroundRole(bgRole);
		}
		m_table[paramIdx]->head->setBackgroundRole(bgRole);
		m_table[paramIdx]->bars->setBackgroundRole(bgRole);
		for (int barIdx = 0; barIdx < m_table[paramIdx]->out.size(); ++barIdx)
		{
			m_table[paramIdx]->out[barIdx]->setBackgroundRole(bgRole);
			m_table[paramIdx]->par[barIdx]->setBackgroundRole(bgRole);
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
		 ((col.first == outCharacteristic)  ?
			 (m_charDiffMeasureIdx == 0 ?
				 m_data->aggregatedSensitivities[col.second][m_measureIdx] :
				 m_data->aggregatedSensitivitiesPWDiff[col.second]) :
		     ((col.first == outFiberCount)  ? m_data->aggregatedSensitivitiesFiberCount
		 /*(col.first == outDissimilarity)*/: m_data->aggregatedSensDissim[col.second]))[m_aggrType];
		// TODO: unify with addStackedBar
		auto title(columnName(col.first, col.second));
		double maxVal, minValDiff;
		getParamMaxMinDiffVal(d, maxVal, minValDiff);
		for (int paramIdx = 0; paramIdx < m_data->m_variedParams.size(); ++paramIdx)
		{
			m_table[paramIdx]->bars->updateBar(title, d[paramIdx], maxVal, minValDiff);
			updateStackedBarHistogram(title, paramIdx, col.first, col.second);
		}
	}
	for (int paramIdx = 0; paramIdx < m_data->m_variedParams.size(); ++paramIdx)
	{
		m_table[paramIdx]->bars->update();
	}
	updateChartY();
}

QString iAParameterInfluenceView::columnName(int outType, int outIdx) const
{
	return  +
		(outType == outCharacteristic) ? QString("Variation ") + m_data->charactName(outIdx) :
		((outType == outFiberCount) ? "Variation Fiber Count"
		/*(outType == outDissimilarity)*/	:
			getAvailableDissimilarityMeasureNames()[m_data->m_resultDissimMeasures[outIdx].first]);
}

std::shared_ptr<iAPlot> iAParameterInfluenceView::createHistoPlot(std::shared_ptr<iAHistogramData> histoData, QColor color)
{	// m_histogramChartType values need to match values from SensitivitySettings.ui file
	if (m_histogramChartType == "Bars")
	{
		return std::make_shared<iABarGraphPlot>(histoData, barGraphColor(color));
	}
	else if (m_histogramChartType == "Lines")
	{
		return std::make_shared<iALinePlot>(histoData, color);
	}
	else
	{
		LOG(lvlWarn, QString("Unknown chart type '%1'!").arg(m_histogramChartType));
		return std::shared_ptr<iAPlot>();
	}
}

void iAParameterInfluenceView::updateStackedBarHistogram(QString const & barName, int paramIdx, int outType, int outIdx)
{
	int barIdx = m_table[paramIdx]->bars->barIndex(barName);
	auto outChart = m_table[paramIdx]->out[barIdx];
	outChart->clearPlots();
	outChart->resetYBounds();
	std::array<double, 2> rng;
	if (outType == outCharacteristic)
	{
		auto r = m_data->m_data->spmData->paramRange(m_data->m_charSelected[outIdx]);
		rng[0] = r[0];
		rng[1] = r[1];
	}
	else if (outType == outFiberCount)
	{
		rng[0] = m_data->m_fiberCountRange[0];
		rng[1] = m_data->m_fiberCountRange[1];
	}
	else /* outType == outDissimilarity */
	{
		rng[0] = m_data->m_dissimRanges[outIdx].first;
		rng[1] = m_data->m_dissimRanges[outIdx].second;
	}
	if (outType == outCharacteristic)
	{
		auto varHistData = iAHistogramData::create(barName, iAValueType::Continuous, rng[0], rng[1],
			m_data->charHistVarAgg[outIdx][m_aggrType][paramIdx]);
		outChart->addPlot(createHistoPlot(varHistData, VariationHistogramColor));
	}
	auto avgHistData = iAHistogramData::create("Average", iAValueType::Continuous, rng[0], rng[1],
		(outType == outCharacteristic)
			? m_data->charHistAvg[outIdx]
			: (outType == outFiberCount)
				? m_data->fiberCountHistogram
				: /* outType == outDissimilarity */ m_data->m_dissimHistograms[outIdx]);
	outChart->addPlot(createHistoPlot(avgHistData, AverageHistogramColor));
	auto selectedResults = m_viewState->selectedResults();
	for (size_t i=0; i<selectedResults.size(); ++i)
	{
		auto resultIdx = selectedResults[i];
		addResultHistoPlot(resultIdx, paramIdx, barIdx, m_viewState->selectedResultColorTheme()->color(i));
	}
	outChart->update();

	auto parChart = m_table[paramIdx]->par[barIdx];
	auto spData = std::make_shared<iASPLOMData>();
	auto const& d = ((outType == outCharacteristic) ?
		(m_charDiffMeasureIdx == 0 ?
				 m_data->sensitivityField[outIdx][m_measureIdx][m_aggrType] :
				 m_data->sensitivityFieldPWDiff[outIdx][m_aggrType]) :
		(outType == outFiberCount)
			? m_data->sensitivityFiberCount[m_aggrType]
			: /* (outType == outDissimilarity)*/ m_data->sensDissimField[outIdx][m_aggrType])[paramIdx];
	std::vector<QString> paramNames;
	paramNames.push_back(m_data->m_paramNames[m_data->m_variedParams[paramIdx]]);
	paramNames.push_back("Sensitivity");
	paramNames.push_back("Highest");
	spData->setParameterNames(paramNames);

	double normalizeFactor = 1.0;
	if (m_normalizePerOutput)
	{	// maybe do in computation already / merge with max determination in iAAlgorithmInfo ?
		double maxEl = *std::max_element(d.begin(), d.end());
		normalizeFactor = (maxEl != 0) ? (1 / maxEl) : 1.0;
	}
	for (int i = 0; i < m_data->paramSetValues.size(); ++i)
	{
		spData->data()[0].push_back(m_data->paramSetValues[i][m_data->m_variedParams[paramIdx]]);
		spData->data()[1].push_back(d[i] * normalizeFactor);
		spData->data()[2].push_back(m_data->paramSetValues[i][m_data->m_variedParams[m_sort[0]]]);
	}
	parChart->setData(spData);
	parChart->resetYBounds();
	parChart->setDrawGridLines(false);
	parChart->setPointRadius(3);
	parChart->setPickedPointFactor(1.0);  // disable picking / make invisible
	parChart->setSelectionColor(QColor(0,0,0));
	parChart->setSelectionEnabled(false);
	parChart->setLookupTable(m_sortParamLUT, 2);
	parChart->update();
}

void iAParameterInfluenceView::setHistogramChartType(QString const & chartType)
{
	m_histogramChartType = chartType;
	updateStackedBars();
}

QVector<int> const& iAParameterInfluenceView::paramIndicesSorted() const
{
	return m_sort;
}

template <typename ChartType>
void setChartYBounds(QVector<ChartType*> const& c, double yMax)
{
	for (auto chart : c)
	{
		chart->setYBounds(0, yMax);
		chart->update();
	}
}

template <typename ChartType>
double getChartYMax(QVector<ChartType*> const& c, double yMax)
{
	for (auto chart : c)
	{
		yMax = std::max(yMax, chart->yBounds()[1]);
	}
	return yMax;
}

void iAParameterInfluenceView::updateChartY()
{
	double yMaxOut = std::numeric_limits<double>::lowest();
	double yMaxPar = std::numeric_limits<double>::lowest();
	for (auto chartRow : m_table)
	{
		yMaxOut = getChartYMax(chartRow->out, yMaxOut);
		yMaxPar = getChartYMax(chartRow->par, yMaxPar);
	}
	for (auto chartRow : m_table)
	{
		setChartYBounds(chartRow->out, yMaxOut);
		setChartYBounds(chartRow->par, yMaxPar);
	}
}

void iAParameterInfluenceView::setActionChecked(int outType, int outIdx, bool checked)
{
	for (auto action : m_table[0]->head->contextMenu()->actions())
	{
		if (action->property("objectType").toInt() == outType && action->property("charactIdx").toInt() == outIdx)
		{
			QSignalBlocker b(action);
			action->setChecked(checked);
		}
	}
}

void iAParameterInfluenceView::addStackedBar(int outType, int outIdx)
{
	setActionChecked(outType, outIdx, true);
	m_visibleCharacts.push_back(qMakePair(outType, outIdx));
	auto title(columnName(outType, outIdx));
	LOG(lvlDebug, QString("Showing stacked bar for characteristic %1").arg(title));
	auto const& d = (
		   (outType == outCharacteristic)  ?
				(m_charDiffMeasureIdx == 0 ?
					m_data->aggregatedSensitivities[outIdx][m_measureIdx] :
					m_data->aggregatedSensitivitiesPWDiff[outIdx]) :
		      ((outType == outFiberCount)  ? m_data->aggregatedSensitivitiesFiberCount
		/*(col.first == outDissimilarity)*/: m_data->aggregatedSensDissim[outIdx]))[m_aggrType];

	//int curBarIdx = static_cast<int>(m_table[0]->head->numberOfBars());  // not yet added to bars here so no -1
	auto params = m_data->m_variedParams;

	// add the two charts that can be shown inside the matrix cells:
	auto selectedResults = m_viewState->selectedResults();
	for (int paramIdx = 0; paramIdx < params.size(); ++paramIdx)
	{
		int varParIdx = m_data->m_variedParams[paramIdx];
		auto paramName = m_data->m_paramNames[varParIdx];
		//QColor color = (paramIdx == m_selectedParam) ? ParamRowSelectedBGColor : ParamRowUnselectedBGColor;
		QPalette::ColorRole bgRole = (paramIdx == m_selectedParam) ? QPalette::AlternateBase : QPalette::Window;

		auto outChart = new iAChartWidget(this, "", /*(curBarIdx == 0) ? */ "Var. from " + paramName /*: ""*/);
		outChart->showLegend(true);
		outChart->showXAxisLabel(false);
		outChart->setEmptyText("");
		outChart->setBackgroundRole(bgRole);
		//outChart->setMinimumHeight(80);
		m_table[paramIdx]->out.push_back(outChart);
		//connect(outChart, &iAChartWidget::clicked, this, &iAParameterInfluenceView::);
		connect(outChart, &iAChartWidget::axisChanged, this, &iAParameterInfluenceView::charactChartAxisChanged);

		//auto parChart = new iAChartWidget(this, paramName, (curBarIdx == 0) ? "Sensitivity" : "");

		auto parChart = new iAScatterPlotWidget();
		//parChart->setEmptyText("");
		parChart->setBackgroundRole(bgRole);
		parChart->setShowToolTips(false);
		parChart->setProperty("paramIdx", paramIdx);
		m_table[paramIdx]->par.push_back(parChart);
		//double parMin = m_data->m_paramMin[varParIdx],
		//	parMax = m_data->m_paramMax[varParIdx],
		//	double parPad = (parMax - parMin) / 100.0; // add 1% of range on both sides to make sure all markers will be visible
		//parChart->setXBounds(parMin - parPad, parMax + parPad);
		//connect(parChart, &iAChartWidget::axisChanged, this, &iAParameterInfluenceView::paramChartAxisChanged);
		connect(parChart, &iAScatterPlotWidget::chartPress, this, &iAParameterInfluenceView::paramChartClicked);
		//parChart->setMinimumHeight(80);
		for (size_t i=0; i<selectedResults.size(); ++i)
		{
			auto resultIdx = selectedResults[i];
			double paramValue = m_data->m_paramValues[m_data->m_variedParams[paramIdx]][resultIdx];
			parChart->setXMarker(paramValue, m_viewState->selectedResultColorTheme()->color(i), Qt::DashLine);
		}
	}
	addTableWidgets();
	double maxVal, minValDiff;
	getParamMaxMinDiffVal(d, maxVal, minValDiff);
	for (int paramIdx = 0; paramIdx < m_data->m_variedParams.size(); ++paramIdx)
	{
		m_table[paramIdx]->head->addBar(title, 1, 1, 1);
		m_table[paramIdx]->bars->addBar(title, d[paramIdx], maxVal, minValDiff);
		updateStackedBarHistogram(title, paramIdx, outType, outIdx);
		m_table[paramIdx]->head->setLeftMargin(m_table[paramIdx]->out[0]->leftMargin());
		m_table[paramIdx]->bars->setLeftMargin(m_table[paramIdx]->out[0]->leftMargin());
	}
	updateChartY();
	emit barAdded(outType, outIdx);
}

// TODO: avoid duplication:

void iAParameterInfluenceView::paramChartAxisChanged()
{
	/*
	auto inChart = qobject_cast<iAChartWidget*>(QObject::sender());
	for (int rowIdx = 0; rowIdx < m_table.size(); ++rowIdx)
	{
		for (int i = 0; i < m_table[rowIdx]->par.size(); ++i)
		{
			auto chart = m_table[rowIdx]->par[i];
			if (chart == inChart)
			{
				continue;
			}
			//chart->setXZoom(inChart->xZoom());
			//chart->setXShift(inChart->xShift());
			//chart->setYZoom(inChart->yZoom());
			chart->update();
		}
	}
	*/
}

void iAParameterInfluenceView::charactChartAxisChanged()
{
	auto inChart = qobject_cast<iAChartWidget*>(QObject::sender());
	for (int rowIdx = 0; rowIdx < m_table.size(); ++rowIdx)
	{
		for (int i = 0; i < m_table[rowIdx]->out.size(); ++i)
		{
			auto chart = m_table[rowIdx]->out[i];
			if (chart == inChart)
			{
				continue;
			}
			chart->setXZoom(inChart->xZoom());
			chart->setXShift(inChart->xShift());
			chart->setYZoom(inChart->yZoom());
			chart->update();
		}
	}
}


void iAParameterInfluenceView::removeStackedBar(int outType, int outIdx)
{
	setActionChecked(outType, outIdx, false);
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
		m_table[rowIdx]->head->removeBar(title);
		int barIdx = m_table[rowIdx]->bars->removeBar(title);
		auto ow = m_table[rowIdx]->out[barIdx];
		m_table[rowIdx]->out.remove(barIdx);
		delete ow;
		auto pw = m_table[rowIdx]->par[barIdx];
		m_table[rowIdx]->par.remove(barIdx);
		delete pw;
		auto paramName = m_data->m_paramNames[m_data->m_variedParams[m_sort[rowIdx]]];
		int newNumBars = m_table[rowIdx]->bars->numberOfBars();
		if (newNumBars > 0)
		{
			//m_table[rowIdx]->out[0]->setYCaption("Var. from " + paramName);  // to make sure if first chart is removed that new first gets caption
			//m_table[rowIdx]->par[0]->setYCaption("Sensitivity"/*" + m_table[rowIdx]->bars->barName(0)*/);
			for (int i = barIdx; i < newNumBars; ++i)
			{  // addWidget automatically removes it from position where it was before
				m_paramListLayout->addWidget(
					m_table[rowIdx]->out[i], 1 + RowsPerParam * rowIdx + RowOutputChart, colStackedBar + i);
				m_paramListLayout->addWidget(
					m_table[rowIdx]->par[i], 1 + RowsPerParam * rowIdx + RowParamChart, colStackedBar + i);
			}
			for (int b = 0; b < newNumBars; ++b)
			{
				m_table[rowIdx]->out[b]->resetYBounds();
				m_table[rowIdx]->par[b]->resetYBounds();
			}
		}
	}
	updateChartY();
	emit barRemoved(outType, outIdx);
}

void iAParameterInfluenceView::setInColor(QColor const& inColor)
{
	for (int paramIdx = 0; paramIdx < m_table.size(); ++paramIdx)
	{
		m_table[paramIdx].get()->labels[colParamName]->setStyleSheet(
			"QLabel { background-color : " + inColor.name() + "; }");
	}
}

void iAParameterInfluenceView::setInOutColorPrivate(QColor const& inColor, QColor const& outColor)
{
	setInColor(inColor);
	auto newColorTheme = std::make_shared<iASingleColorTheme>("OneOutputColorTheme", outColor);
	for (int r = 0; r < m_table.size(); ++r)
	{
		auto row = m_table[r].get();
		row->head->setColorTheme(newColorTheme.get());
		row->bars->setColorTheme(newColorTheme.get());
	}
	m_stackedBarTheme = newColorTheme;
}

void iAParameterInfluenceView::setInOutColor(QColor const& inColor, QColor const& outColor)
{
	setInOutColorPrivate(inColor, outColor);
	update();
}
