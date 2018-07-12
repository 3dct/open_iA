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

#include "BarycentricTriangle.h"

#include "BCoord.h"

BarycentricTriangle::BarycentricTriangle(int xa, int ya, int xb, int yb, int xc, int yc) :
	m_xa(xa), m_ya(ya), m_xb(xb), m_yb(yb), m_xc(xc), m_yc(yc)
{
}

BCoord* BarycentricTriangle::getBarycentricCoordinates(double x, double y)
{
	return new BCoord(this, x, y);
}

BCoord* BarycentricTriangle::getBarycentricCoordinatesA()
{
	return new BCoord(1, 0);
}

BCoord* BarycentricTriangle::getBarycentricCoordinatesB()
{
	return new BCoord(0, 1);
}

BCoord* BarycentricTriangle::getBarycentricCoordinatesC()
{
	return new BCoord(0, 0);
}

QPoint* BarycentricTriangle::getCartesianCoordinates(const BCoord &bCoord)
{
	double alpha, beta, gamma;
	alpha = bCoord.getAlpha();
	beta = bCoord.getBeta();
	gamma = bCoord.getGamma();

	return new QPoint(
		(alpha * m_xa) + (beta * m_xb) + (gamma * m_xc),
		(alpha * m_ya) + (beta * m_yb) + (gamma * m_yc)
	);
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