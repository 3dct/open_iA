// Copyright (c) open_iA contributors
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
		std::shared_ptr<iAParamHistogramData> data,
		std::shared_ptr<iANameMapper> nameMapper,
		bool showCaption = false);
	double mapBinToValue(double bin) const;
	double mapValueToBin(double value) const;
	std::shared_ptr<iAPlot> GetDrawer(std::shared_ptr<iAParamHistogramData> data, QColor color);
	void RemoveMarker();
	void SetMarker(double value);
	virtual iAValueType GetRangeType() const;
	double GetMinVisibleBin() const;
	double GetMaxVisibleBin() const;
	void SetBinColor(size_t bin, QColor const & color);
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

	std::shared_ptr<iAParamHistogramData> m_data;
	std::shared_ptr<iANameMapper> m_nameMapper;
	double m_markedLocation;
	std::vector<QColor> m_binColors;
	double m_minSliderPos, m_maxSliderPos;
	int m_selectedHandle;
	int m_selectionOffset;
};
