// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
