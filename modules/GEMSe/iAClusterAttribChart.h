/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <iAValueType.h>

#include <QWidget>

class iAFilterChart;
class iANameMapper;
class iAParamHistogramData;
class iAPlot;

class QCheckBox;
class QLabel;

class iAClusterAttribChart: public QWidget
{
	Q_OBJECT
public:
	iAClusterAttribChart(QString const & caption, int id, QSharedPointer<iAParamHistogramData> data,
		QSharedPointer<iANameMapper> nameMapper);
	void SetFilteredData(QSharedPointer<iAParamHistogramData> data);
	void SetFilteredClusterData(QSharedPointer<iAParamHistogramData> data);
	void RemoveFilterData();
	void AddClusterData(QSharedPointer<iAParamHistogramData> data);
	void ClearClusterData();
	void SetMarker(double xPos);
	void RemoveMarker();
	size_t GetNumBin() const;
	int GetID() const;
	iAValueType GetRangeType() const;
	double GetMaxYValue() const;
	void SetMaxYAxisValue(double val);
	
	void SetSpanValues(double minValue, double maxValue);
	void ResetSpan();
	double mapValueToBin(double value) const;
	void SetBinColor(int bin, QColor const & color);
	void UpdateChart();
	void ResetMaxYAxisValue();
signals:
	void Toggled(bool);
	void FilterChanged(double min, double max);
	void ChartDblClicked();
private slots:
	void SelectionChanged();
private:
	void SetAdditionalDrawer(QSharedPointer<iAPlot>& drawer, QSharedPointer<iAPlot> newDrawer);
	QColor GetClusterColor(int nr) const;
	
	iAFilterChart*  m_charts;
	QCheckBox*     m_checkbox;
	int    m_ID;
	QVector<QSharedPointer<iAPlot> > m_clusterDrawer;
	QSharedPointer<iAPlot> m_filteredDrawer;
	QSharedPointer<iAPlot> m_filteredClusterDrawer;
	int	           m_oldMin;
	int	           m_oldMax;
	
	QSharedPointer<iAParamHistogramData> m_filteredClusterData;
};
