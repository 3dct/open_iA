/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <QMap>
#include <QSet>
#include <QWidget>

class iAChartWidget;
class iAColorTheme;
class iAHistogramData;
class iAStackedBarChart;
class iASensitivityData;
class iASensitivityViewState;
class iASingleColorTheme;
class iAParTableRow;

class iAPlot;

class QGridLayout;

enum
{
	outCharacteristic = 0,
	outFiberCount = 1,
	outDissimilarity = 2
};

class iAParameterInfluenceView : public QWidget
{
	Q_OBJECT
public:
	iAParameterInfluenceView(QSharedPointer<iASensitivityData> data, QSharedPointer<iASensitivityViewState> viewState,
		QColor const& paramColor, QColor const& outputColor);
	void setDistributionMeasure(int newMeasure);
	void setCharDiffMeasure(int newMeasure);
	void setAggregation(int newAggregation);
	void setResultSelected(size_t resultIdx, bool state, QColor c);
	int selectedMeasure() const;
	int selectedAggrType() const;
	int selectedRow() const;
	int selectedCol() const;
	void setHistogramChartType(QString const& chartType);
	QVector<int> const& paramIndicesSorted() const;  //! return parameter indices in current sort order (by variation)
	void updateHighlightColors(std::vector<size_t> highlighted, iAColorTheme const* theme);
	void setHighlightedParams(QSet<int> hiParam);
	void setNormalizePerOutput(bool norm);
	void setInOutColor(QColor const& inColor, QColor const& outColor);

signals:
	void parameterChanged();
	void barAdded(int outType, int outIdx);
	void barRemoved(int outType, int outIdx);
	void resultSelected(size_t resultIdx, Qt::KeyboardModifiers modifiers);
	void orderChanged(QVector<int> const & sortOrder);

public slots:
	void showStackedBar();
	void selectStackedBar(int outputType, int idx);
	void sortListByBar(int barIdx);
	void setSelectedParam(int param);
	void toggleCharacteristic(int charactIdx);

private slots:
	void paramChangedSlot();
	void setBarWeights(std::vector<double> const& weights);
	void setBarNormalizeMode(bool normalizePerBar);
	void setBarDoStack(bool doStack);
	//void paramChartClicked(double x, Qt::KeyboardModifiers modifiers);
	void paramChartClicked(size_t idx, bool state);
	void paramChartAxisChanged();
	void charactChartAxisChanged();

private:
	void updateStackedBars();
	void addStackedBar(int outputType, int outIdx);
	void removeStackedBar(int outputType, int outIdx);
	void updateStackedBarHistogram(QString const& barName, int paramIdx, int outType, int outIdx);
	void addColumnAction(int outType, int outIdx, bool checked);
	QString columnName(int outputType, int outTypeIdx) const;
	void updateChartY();
	void toggleBar(bool show, int outType, int outIdx);
	void addTableWidgets();
	void setActionChecked(int outType, int outIdx, bool checked);
	QSharedPointer<iAPlot> createHistoPlot(QSharedPointer<iAHistogramData> data, QColor color);
	void addResultHistoPlot(size_t resultIdx, int charIdx, int paramIdx, QColor c);
	void setInColor(QColor const& inColor);
	void setInOutColorPrivate(QColor const& inColor, QColor const& outColor);

	// pair output type / index
	QVector<QPair<int,int>> m_visibleCharacts;
	//! sensitivity data
	QSharedPointer<iASensitivityData> m_data;
	//! view state:
	QSharedPointer<iASensitivityViewState> m_viewState;
	//! values from which distribution difference measure to show
	int m_measureIdx;
	//! whether to show values from distribution based (0) or pairwise characteristics difference measures
	int m_charDiffMeasureIdx;
	//! aggregation type
	int m_aggrType;
	int m_selectedParam, m_selectedCol;
	QGridLayout* m_paramListLayout;
	QSharedPointer<iASingleColorTheme> m_stackedBarTheme;
	QVector<QSharedPointer<iAParTableRow>> m_table;
	QVector<int> m_sort;
	int m_sortLastOut;
	bool m_sortLastDesc;
	QMap<std::tuple<size_t, int, int>, QSharedPointer<iAPlot>> m_selectedResultHistoPlots;
	QString m_histogramChartType;
	QSet<int> m_highlightedParams;
	bool m_normalizePerOutput;
};
