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
#include "BarycentricTriangle.h"

#include "BCoord.h"

BarycentricTriangle::BarycentricTriangle(int xa, int ya, int xb, int yb, int xc, int yc) :
	m_xa(xa), m_ya(ya), m_xb(xb), m_yb(yb), m_xc(xc), m_yc(yc)
{
}

BarycentricTriangle::BarycentricTriangle() :
	m_xa(0), m_ya(0), m_xb(0), m_yb(0), m_xc(0), m_yc(0)
{
}

BCoord BarycentricTriangle::getBarycentricCoordinates(double x, double y)
{
	return BCoord(*this, x, y);
}

BCoord BarycentricTriangle::getBarycentricCoordinatesA()
{
	return BCoord(1, 0);
}

BCoord BarycentricTriangle::getBarycentricCoordinatesB()
{
	return BCoord(0, 1);
}

BCoord BarycentricTriangle::getBarycentricCoordinatesC()
{
	return BCoord(0, 0);
}

QPoint BarycentricTriangle::getCartesianCoordinates(const BCoord &bCoord)
{
	QPoint ret = QPoint();
	updateCartesianCoordinates(ret, bCoord);
	return ret;
}

QPoint BarycentricTriangle::getCartesianCoordinates(double alpha, double beta)
{
	QPoint ret = QPoint();
	updateCartesianCoordinates(ret, alpha, beta);
	return ret;
}

void BarycentricTriangle::updateCartesianCoordinates(QPoint &qPoint, const BCoord &bCoord)
{
	updateCartesianCoordinates(qPoint, bCoord.getAlpha(), bCoord.getBeta(), bCoord.getGamma());
}

void BarycentricTriangle::updateCartesianCoordinates(QPoint &qPoint, double alpha, double beta)
{
	updateCartesianCoordinates(qPoint, alpha, beta, 1.0 - alpha - beta);
}

void BarycentricTriangle::updateCartesianCoordinates(QPoint &qPoint, double alpha, double beta, double gamma)
{
	qPoint.setX((alpha * m_xa) + (beta * m_xb) + (gamma * m_xc));
	qPoint.setY((alpha * m_ya) + (beta * m_yb) + (gamma * m_yc));
}


int BarycentricTriangle::getXa()
{
	return m_xa;
}

int BarycentricTriangle::getYa()
{
	return m_ya;
}

int BarycentricTriangle::getXb()
{
	return m_xb;
}

int BarycentricTriangle::getYb()
{
	return m_yb;
}

int BarycentricTriangle::getXc()
{
	return m_xc;
}

int BarycentricTriangle::getYc()
{
	return m_yc;
}

void BarycentricTriangle::setXa(int xa)
{
	m_xa = xa;
}

void BarycentricTriangle::setYa(int ya)
{
	m_ya = ya;
}

void BarycentricTriangle::setXb(int xb)
{
	m_xb = xb;
}

void BarycentricTriangle::setYb(int yb)
{
	m_yb = yb;
}

void BarycentricTriangle::setXc(int xc)
{
	m_xc = xc;
}

void BarycentricTriangle::setYc(int yc)
{
	m_yc = yc;
}

void BarycentricTriangle::set(int xa, int ya, int xb, int yb, int xc, int yc)
{
	m_xa = xa;
	m_ya = ya;
	m_xb = xb;
	m_yb = yb;
	m_xc = xc;
	m_yc = yc;
}

bool BarycentricTriangle::contains(double x, double y)
{
	return getBarycentricCoordinates(x, y).isInside();
}

QRect BarycentricTriangle::getBounds()
{
	int minx = qMin(m_xa, qMin(m_xb, m_xc));
	int maxx = qMax(m_xa, qMax(m_xb, m_xc));
	int miny = qMin(m_ya, qMin(m_yb, m_yc));
	int maxy = qMax(m_ya, qMax(m_yb, m_yc));
	return QRect(minx, miny, maxx - minx, maxy - miny);
}
