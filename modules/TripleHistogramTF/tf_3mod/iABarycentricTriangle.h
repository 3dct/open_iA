// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QPoint>
#include <QRect>

class iABCoord;

class iABarycentricTriangle
{
	public: // TODO: int/double or references?
		iABarycentricTriangle(int xa, int ya, int xb, int yb, int xc, int yc);
		iABarycentricTriangle();

		iABarycentricTriangle operator- (QPoint p)
		{
			return iABarycentricTriangle(m_xa - p.x(), m_ya - p.y(), m_xb - p.x(), m_yb - p.y(), m_xc - p.x(), m_yc - p.y());
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

		iABCoord getBarycentricCoordinates(double x, double y);
		iABCoord getBarycentricCoordinatesA();
		iABCoord getBarycentricCoordinatesB();
		iABCoord getBarycentricCoordinatesC();

		bool contains(double x, double y);

		QPoint getCartesianCoordinates(const iABCoord &bCoord);
		QPoint getCartesianCoordinates(double alpha, double beta);
		void updateCartesianCoordinates(QPoint &qPoint, const iABCoord &bCoord);
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
