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

#include "iAAbstractDrawableFunction.h"
#include "open_iA_Core_export.h"

#include <QVector>
#include <QSharedPointer>

class iAAbstractDiagramData;

class QPolygon;

class open_iA_Core_API iASelectedBinDrawer : public iAAbstractDrawableFunction
{
public:
	iASelectedBinDrawer( int position = 0, QColor const & color = Qt::red );
	void setPosition( int position );
	void draw( QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter ) const;
	void update(){}
private:
	int m_position;
};

class open_iA_Core_API iAPolygonBasedFunctionDrawer: public iAAbstractDrawableFunction
{
public:
	iAPolygonBasedFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color);
	void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const;
	void update();
private:
	virtual bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const =0;
	virtual void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const = 0;
protected:
	QSharedPointer<iAAbstractDiagramData> m_data;
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
	iALineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color);
private:
	virtual bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const;
	void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const;
};


class open_iA_Core_API iAStepFunctionDrawer : public iAPolygonBasedFunctionDrawer
{
public:
	iAStepFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color);
private:
	virtual bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const;
	void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const;
	virtual QColor getFillColor() const;
};

class open_iA_Core_API iAFilledLineFunctionDrawer : public iAPolygonBasedFunctionDrawer
{
public:
	iAFilledLineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color);
private:
	virtual bool computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const;
	void drawPoly(QPainter& painter, QSharedPointer<QPolygon> m_poly) const;
	virtual QColor getFillColor() const;
};


class open_iA_Core_API iABarGraphDrawer: public iAAbstractDrawableFunction
{
public:
	iABarGraphDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color, int margin=0);
	void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const;
	void update();
private:
	QSharedPointer<iAAbstractDiagramData> m_data;
	int m_margin;
};


class open_iA_Core_API iAMultipleFunctionDrawer: public iAAbstractDrawableFunction
{
public:
	iAMultipleFunctionDrawer();
	void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const;
	void add (QSharedPointer<iAAbstractDrawableFunction> drawer);
	void update();
	void clear();
	void setColor(QColor const & color);
private:
	QVector<QSharedPointer<iAAbstractDrawableFunction> > m_drawers;
};
