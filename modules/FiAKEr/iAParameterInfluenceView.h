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
#pragma once

#include <QWidget>

class iAChartWidget;
class iAStackedBarChart;
class iASensitivityInfo;
class iASingleColorTheme;
class iAParTableRow;

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
	iAParameterInfluenceView(iASensitivityInfo* sensInf, QColor const& paramColor, QColor const& outputColor);
	void setMeasure(int newMeasure);
	void setAggregation(int newAggregation);
	int selectedMeasure() const;
	int selectedAggrType() const;
	int selectedRow() const;
	int selectedCol() const;
public slots:
	void showStackedBar();
	void selectStackedBar(int outputType, int idx);
	void stackedBarDblClicked(int idx);
	void setSelectedParam(int param);
	void toggleCharacteristic(int charactIdx);
private slots:
	void paramChangedSlot();
	void setBarWeights(std::vector<double> const& weights);
	void setBarNormalizeMode(bool normalizePerBar);
	void setBarDoStack(bool doStack);

private:
	void updateStackedBars();
	void addStackedBar(int outputType, int outIdx);
	void removeStackedBar(int outputType, int outIdx);
	void updateStackedBarHistogram(QString const& barName, int paramIdx, int outType, int outIdx);
	void addColumnAction(int outType, int outIdx, bool checked);
	QString columnName(int outputType, int outTypeIdx) const;
	void updateChartY();
	void toggleBar(bool show, int outType, int outIdx);
	void updateTableOrder();

	// pair output type / index
	QVector<QPair<int,int>> m_visibleCharacts;
	//! sensitivity information
	iASensitivityInfo* m_sensInf;
	//! measure to use 
	int m_measureIdx;
	//! aggregation type
	int m_aggrType;
	int m_selectedParam, m_selectedCol;
	QGridLayout* m_paramListLayout;
	QSharedPointer<iASingleColorTheme> m_stackedBarTheme;
	QVector<iAParTableRow> m_table;
	QVector<int> m_sort;
signals:
	void parameterChanged();
	void outputSelected(int outType, int outTypeIdx);
	void barAdded(int outType, int outIdx);
	void barRemoved(int outType, int outIdx);
};