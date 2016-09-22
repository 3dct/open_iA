/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAValueType.h"

#include <vtkSmartPointer.h>

#include <QWidget>

class iAAbstractDrawableFunction;
class iANameMapper;
class iAParamChart;
class iAParamHistogramData;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class QCheckBox;
class QLabel;

class iAChartSpanSlider: public QWidget
{
	Q_OBJECT
public:
	iAChartSpanSlider(QString const & caption, int id, QSharedPointer<iAParamHistogramData> data,
		QSharedPointer<iANameMapper> nameMapper);
	void SetFilteredData(QSharedPointer<iAParamHistogramData> data);
	void SetFilteredClusterData(QSharedPointer<iAParamHistogramData> data);
	void RemoveFilterData();
	void AddClusterData(QSharedPointer<iAParamHistogramData> data);
	void ClearClusterData();
	void SetMarker(double xPos);
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
signals:
	void Toggled(bool);
	void FilterChanged(double min, double max);
	void ChartDblClicked();
private slots:
	void SelectionChanged();
private:
	void SetAdditionalDrawer(QSharedPointer<iAAbstractDrawableFunction>& drawer, QSharedPointer<iAAbstractDrawableFunction> newDrawer);
	QColor GetClusterColor(int nr) const;
	
	iAParamChart*  m_charts;
	QCheckBox*     m_checkbox;
	int    m_ID;
	QVector<QSharedPointer<iAAbstractDrawableFunction> > m_clusterDrawer;
	QSharedPointer<iAAbstractDrawableFunction> m_filteredDrawer;
	QSharedPointer<iAAbstractDrawableFunction> m_filteredClusterDrawer;
	int	           m_oldMin;
	int	           m_oldMax;
	
	QSharedPointer<iAParamHistogramData> m_filteredClusterData;
};
