// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABarycentricTriangle.h"

#include "iABCoord.h"

iABarycentricTriangle::iABarycentricTriangle(int xa, int ya, int xb, int yb, int xc, int yc) :
	m_xa(xa), m_ya(ya), m_xb(xb), m_yb(yb), m_xc(xc), m_yc(yc)
{
}

iABarycentricTriangle::iABarycentricTriangle() :
	m_xa(0), m_ya(0), m_xb(0), m_yb(0), m_xc(0), m_yc(0)
{
}

iABCoord iABarycentricTriangle::getBarycentricCoordinates(double x, double y)
{
	return iABCoord(*this, x, y);
}

iABCoord iABarycentricTriangle::getBarycentricCoordinatesA()
{
	return iABCoord(1, 0);
}

iABCoord iABarycentricTriangle::getBarycentricCoordinatesB()
{
	return iABCoord(0, 1);
}

iABCoord iABarycentricTriangle::getBarycentricCoordinatesC()
{
	return iABCoord(0, 0);
}

QPoint iABarycentricTriangle::getCartesianCoordinates(const iABCoord &bCoord)
{
	QPoint ret = QPoint();
	updateCartesianCoordinates(ret, bCoord);
	return ret;
}

QPoint iABarycentricTriangle::getCartesianCoordinates(double alpha, double beta)
{
	QPoint ret = QPoint();
	updateCartesianCoordinates(ret, alpha, beta);
	return ret;
}

void iABarycentricTriangle::updateCartesianCoordinates(QPoint &qPoint, const iABCoord &bCoord)
{
	updateCartesianCoordinates(qPoint, bCoord.getAlpha(), bCoord.getBeta(), bCoord.getGamma());
}

void iABarycentricTriangle::updateCartesianCoordinates(QPoint &qPoint, double alpha, double beta)
{
	updateCartesianCoordinates(qPoint, alpha, beta, 1.0 - alpha - beta);
}

void iABarycentricTriangle::updateCartesianCoordinates(QPoint &qPoint, double alpha, double beta, double gamma)
{
	qPoint.setX((alpha * m_xa) + (beta * m_xb) + (gamma * m_xc));
	qPoint.setY((alpha * m_ya) + (beta * m_yb) + (gamma * m_yc));
}


int iABarycentricTriangle::getXa()
{
	return m_xa;
}

int iABarycentricTriangle::getYa()
{
	return m_ya;
}

int iABarycentricTriangle::getXb()
{
	return m_xb;
}

int iABarycentricTriangle::getYb()
{
	return m_yb;
}

int iABarycentricTriangle::getXc()
{
	return m_xc;
}

int iABarycentricTriangle::getYc()
{
	return m_yc;
}

void iABarycentricTriangle::setXa(int xa)
{
	m_xa = xa;
}

void iABarycentricTriangle::setYa(int ya)
{
	m_ya = ya;
}

void iABarycentricTriangle::setXb(int xb)
{
	m_xb = xb;
}

void iABarycentricTriangle::setYb(int yb)
{
	m_yb = yb;
}

void iABarycentricTriangle::setXc(int xc)
{
	m_xc = xc;
}

void iABarycentricTriangle::setYc(int yc)
{
	m_yc = yc;
}

void iABarycentricTriangle::set(int xa, int ya, int xb, int yb, int xc, int yc)
{
	m_xa = xa;
	m_ya = ya;
	m_xb = xb;
	m_yb = yb;
	m_xc = xc;
	m_yc = yc;
}

bool iABarycentricTriangle::contains(double x, double y)
{
	return getBarycentricCoordinates(x, y).isInside();
}

QRect iABarycentricTriangle::getBounds()
{
	int minx = qMin(m_xa, qMin(m_xb, m_xc));
	int maxx = qMax(m_xa, qMax(m_xb, m_xc));
	int miny = qMin(m_ya, qMin(m_yb, m_yc));
	int maxy = qMax(m_ya, qMax(m_yb, m_yc));
	return QRect(minx, miny, maxx - minx, maxy - miny);
}
