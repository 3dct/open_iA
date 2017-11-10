/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

class iAPlotData;

class QPolygon;

class open_iA_Core_API iASelectedBinDrawer : public iAPlot
{
public:
	iASelectedBinDrawer( int position = 0, QColor const & color = Qt::red );
	void setPosition( int position );
	void draw( QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter ) const override;
private:
	int m_position;
};

class open_iA_Core_API iAPolygonBasedFunctionDrawer: public iAPlot
{
public:
	iAPolygonBasedFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color);
	void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const override;
	void update() override;
private:
	virtual bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const =0;
	virtual void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const = 0;
	QSharedPointer<iAPlotData> GetData() override;
protected:
	QSharedPointer<iAPlotData> m_data;
	//! @{
	//! just for caching:
	mutable QSharedPointer<QPolygon> m_poly;
	mutable double m_cachedBinWidth;
	mutable QSharedPointer<CoordinateConverter> m_cachedCoordConv;
	//! @}
};


class open_iA_Core_API iALineFunctionDrawer: public iAPolygonBasedFunctionDrawer
{
public:
	iALineFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const override;
	void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const override;
};


class open_iA_Core_API iAStepFunctionDrawer : public iAPolygonBasedFunctionDrawer
{
public:
	iAStepFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const override;
	void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const override;
	QColor getFillColor() const;
};

class open_iA_Core_API iAFilledLineFunctionDrawer : public iAPolygonBasedFunctionDrawer
{
public:
	iAFilledLineFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color);
private:
	bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const override;
	void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const override;
	QColor getFillColor() const;
};


class open_iA_Core_API iABarGraphDrawer: public iAPlot
{
public:
	iABarGraphDrawer(QSharedPointer<iAPlotData> data, QColor const & color, int margin=0);
	void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const override;
	QSharedPointer<iAPlotData> GetData() override;
private:
	QSharedPointer<iAPlotData> m_data;
	int m_margin;
};


class open_iA_Core_API iAMultipleFunctionDrawer: public iAPlot
{
public:
	iAMultipleFunctionDrawer();
	void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const override;
	void add (QSharedPointer<iAPlot> drawer);
	void clear();
	void setColor(QColor const & color) override;
private:
	QVector<QSharedPointer<iAPlot> > m_drawers;
};
