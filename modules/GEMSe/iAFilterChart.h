// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAChartWidget.h>

#include <iAValueType.h>

class iAParamHistogramData;
class iANameMapper;

// TODO: Merge with FeatureAnalyzer: iARangeSliderDiagramWidget
class iAFilterChart: public iAChartWidget
{
	Q_OBJECT
public:
	iAFilterChart(QWidget* parent,
		QString const & caption,
		QSharedPointer<iAParamHistogramData> data,
		QSharedPointer<iANameMapper> nameMapper,
		bool showCaption = false);
	double mapBinToValue(double bin) const;
	double mapValueToBin(double value) const;
	QSharedPointer<iAPlot> GetDrawer(QSharedPointer<iAParamHistogramData> data, QColor color);
	void RemoveMarker();
	void SetMarker(double value);
	virtual iAValueType GetRangeType() const;
	double GetMinVisibleBin() const;
	double GetMaxVisibleBin() const;
	void SetBinColor(int bin, QColor const & color);
	double GetMinSliderPos();
	double GetMaxSliderPos();
	void SetMinMaxSlider(double min, double max);
signals:
	void selectionChanged();
protected:
	void drawAxes(QPainter& painter) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mousePressEvent( QMouseEvent *event ) override;
	void mouseReleaseEvent( QMouseEvent *event ) override;
	void mouseMoveEvent( QMouseEvent *event ) override;
private:
	QString xAxisTickMarkLabel(double value, double stepWidth) override;
	void drawMarker(QPainter & painter, double markerLocation, QPen const & pen, QBrush const & brush);

	QSharedPointer<iAParamHistogramData> m_data;
	QSharedPointer<iANameMapper> m_nameMapper;
	double m_markedLocation;
	std::vector<QColor> m_binColors;
	double m_minSliderPos, m_maxSliderPos;
	int m_selectedHandle;
	int m_selectionOffset;
};
