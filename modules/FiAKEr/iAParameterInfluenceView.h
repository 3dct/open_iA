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
class iAColorTheme;
class iAStackedBarChart;
class iASensitivityInfo;

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
	iAParameterInfluenceView(iASensitivityInfo* sensInf);
	void changeMeasure(int newMeasure);
	void changeAggregation(int newAggregation);
	int selectedMeasure() const;
	int selectedAggrType() const;
	int selectedRow() const;
	int selectedCol() const;
	void setColorTheme(iAColorTheme const * colorTheme);
	void setSelectedParam(int param);
public slots:
	void showStackedBar();
	void selectStackedBar(int idx);
	void stackedBarDblClicked(int idx);
private slots:
	void selectMeasure(int measureIdx);
	void paramChangedSlot();
private:
	void updateStackedBars();
	void addStackedBar(int charactIdx);
	void removeStackedBar(int charactIdx);
	void updateStackedBarHistogram(QString const & barName, int paramIdx, int charactIdx);

	QString columnName(int charactIdx) const;

	QVector<int> m_visibleCharacts;
	//! stacked bar charts (one per parameter)
	QVector<iAStackedBarChart*> m_stackedCharts;
	//QVector<iAChartWidget*> m_diffChart;
	iAStackedBarChart* m_stackedHeader;
	//! sensitivity information
	iASensitivityInfo* m_sensInf;
	//! measure to use 
	int m_measureIdx;
	//! aggregation type
	int m_aggrType;
	int m_selectedRow, m_selectedCol;
	QGridLayout* m_paramListLayout;
signals:
	void parameterChanged();
	void characteristicSelected(int charIdx);
};