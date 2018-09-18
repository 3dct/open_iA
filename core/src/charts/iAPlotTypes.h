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

#include "iAPlot.h"
#include "open_iA_Core_export.h"

#include <QVector>
#include <QSharedPointer>

class iAMapper;
class iAPlotData;

class QPolygon;

class open_iA_Core_API iASelectedBinPlot : public iAPlot
{
public:
	iASelectedBinPlot(QSharedPointer<iAPlotData> proxyData, int position = 0, QColor const & color = Qt::red );
	void setPosition( int position );
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
private:
	int m_position;
};

class open_iA_Core_API iALinePlot: public iAPlot
{
public:
	iALinePlot(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
};

class open_iA_Core_API iAStepFunctionPlot : public iAPlot
{
public:
	iAStepFunctionPlot(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
	QColor getFillColor() const;
};

class open_iA_Core_API iAFilledLinePlot : public iAPlot
{
public:
	iAFilledLinePlot(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	void draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const override;
	QColor getFillColor() const;
};


class iALookupTable;

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
