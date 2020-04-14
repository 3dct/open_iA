/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAPlot.h"
#include "open_iA_Core_export.h"

#include <QVector>
#include <QSharedPointer>

class iALookupTable;
class iAMapper;
class iAPlotData;

class QPolygon;

//! Plot highlighting a single bin in a histogram plot.
class open_iA_Core_API iASelectedBinPlot : public iAPlot
{
public:
	iASelectedBinPlot(QSharedPointer<iAPlotData> proxyData, int position = 0, QColor const & color = Qt::red );
	void setPosition( int position );
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
private:
	int m_position;
};

//! Plots the given data as a line.
//! Well-suited for (pseudo-)continuous data.
//! For filling the area under the curve, see the iAFilledLinePlot.
class open_iA_Core_API iALinePlot: public iAPlot
{
public:
	iALinePlot(QSharedPointer<iAPlotData> data, QColor const & color);
	void setLineWidth(int width);
private:
	int m_lineWidth;
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
};

//! Plots each data point as a rectangular bar, all the bars are horizontally connected via a line.
//! Well suited for binned, e.g. histogram data.
class open_iA_Core_API iAStepFunctionPlot : public iAPlot
{
public:
	iAStepFunctionPlot(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
	QColor getFillColor() const;
};

//! Plots the given data as a line and fills the area below the line.
//! Well-suited for (pseudo-)continuous data.
//! For a plot without filling the area under the curve, see the iALinePlot.
class open_iA_Core_API iAFilledLinePlot : public iAPlot
{
public:
	iAFilledLinePlot(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
	QColor getFillColor() const;
};

//! Plots the given data points as single bars, horizontally separated by the given margin.
//! Well-suited for discrete or binned data.
class open_iA_Core_API iABarGraphPlot: public iAPlot
{
public:
	iABarGraphPlot(QSharedPointer<iAPlotData> data, QColor const & color, int margin=0);
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
	void setLookupTable(QSharedPointer<iALookupTable> lut);
private:
	QSharedPointer<iALookupTable> m_lut;
	int m_margin;
};

//! Collects multiple plots and makes them act as a single plot.
class open_iA_Core_API iAPlotCollection: public iAPlot
{
public:
	iAPlotCollection();
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
	void add (QSharedPointer<iAPlot> plot);
	void clear();
	void setColor(QColor const & color) override;
	QSharedPointer<iAPlotData> data() override;
private:
	QVector<QSharedPointer<iAPlot> > m_drawers;
};
