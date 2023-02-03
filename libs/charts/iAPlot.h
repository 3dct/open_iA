// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAcharts_export.h"

#include <QColor>
#include <QSharedPointer>

class iAPlotData;
class iAMapper;

class QPainter;

//! Interface for a function which is drawable in a chart.
//! Encapsulates both the data of the function and the drawing method
class iAcharts_API iAPlot
{
public:
	iAPlot(QSharedPointer<iAPlotData> data, QColor const & color);
	virtual ~iAPlot();
	//! Draws the plot
	virtual void draw(QPainter& painter, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const = 0;
	//! Draws a legend item for the plot
	virtual void drawLegendItem(QPainter& painter, QRect const& rect);
	//! Retrieves the data used for drawing
	virtual QSharedPointer<iAPlotData> data();
	//! Whether the plot is currently being drawn
	virtual bool visible() const;
	//! Sets whether the plot should be currently drawn or not
	virtual void setVisible(bool visible);
	//! Set plot color
	virtual void setColor(QColor const& color);
	//! Get plot color
	QColor const color() const;
protected:
	QSharedPointer<iAPlotData> m_data;
	bool m_visible;
private:
	QColor m_color;
};
