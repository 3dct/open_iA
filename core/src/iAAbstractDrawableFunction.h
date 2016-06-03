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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IAABSTRACTDRAWABLEFUNCTION_H
#define IAABSTRACTDRAWABLEFUNCTION_H

#include <QSharedPointer>

class QPainter;

class CoordinateConverter
{
public:
	virtual double Diagram2ScreenY(double y) const =0;
	virtual double Screen2DiagramY(double y) const =0;
	virtual void update(double yZoom, double yDataMax, double yMinValueBiggerThanZero, int height) =0;
	virtual bool equals(QSharedPointer<CoordinateConverter> other) const
	{
		return false;
	}
	virtual QSharedPointer<CoordinateConverter> clone() =0;
};

/**
 * \class	iAAbstractDrawableFunction
 *
 * \brief	Interface for a function which is drawable in a diagram
 *			encapsulates both the data of the function and the drawing method
 *
 */
class iAAbstractDrawableFunction
{
public:
	/**
	* \brief makes sure the next call to draw will completely redraw the function, without any caching
	*/
	virtual void update() =0;
	/**
	* \brief method which draws the function
	* it is allowed to cache the result; when the data has changed, update() needs to be called
	*/
	virtual void draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const =0;
};



#endif /* IAABSTRACTDRAWABLEFUNCTION_H */
