/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <QPoint>
#include <QRect>

class BCoord;

class BarycentricTriangle
{
	public: // TODO: int/double or references?
		BarycentricTriangle(int xa, int ya, int xb, int yb, int xc, int yc);
		BarycentricTriangle();

		BarycentricTriangle operator- (QPoint p) {
			return BarycentricTriangle(m_xa - p.x(), m_ya - p.y(), m_xb - p.x(), m_yb - p.y(), m_xc - p.x(), m_yc - p.y());
		}

		int getXa();
		int getYa();
		int getXb();
		int getYb();
		int getXc();
		int getYc();

		void set(int xa, int ya, int xb, int yb, int xc, int yc);
		void setXa(int xa);
		void setYa(int ya);
		void setXb(int xb);
		void setYb(int yb);
		void setXc(int xc);
		void setYc(int yc);

		BCoord getBarycentricCoordinates(double x, double y);
		BCoord getBarycentricCoordinatesA();
		BCoord getBarycentricCoordinatesB();
		BCoord getBarycentricCoordinatesC();

		bool contains(double x, double y);

		QPoint getCartesianCoordinates(const BCoord &bCoord);
		QPoint getCartesianCoordinates(double alpha, double beta);
		void updateCartesianCoordinates(QPoint &qPoint, const BCoord &bCoord);
		void updateCartesianCoordinates(QPoint &qPoint, double alpha, double beta);
		void updateCartesianCoordinates(QPoint &qPoint, double alpha, double beta, double gamma);

		QRect getBounds();

	private:
		int m_xa;
		int m_ya;

		int m_xb;
		int m_yb;

		int m_xc;
		int m_yc;


};