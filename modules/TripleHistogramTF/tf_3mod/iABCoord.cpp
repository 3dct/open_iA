// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABCoord.h"

#include "iABarycentricTriangle.h"

iABCoord::iABCoord(double alpha, double beta) :
	m_alpha(alpha), m_beta(beta)
{
}

iABCoord::iABCoord(iABarycentricTriangle triangle, double x, double y)
{
	double x1, y1, x2, y2, x3, y3;
	x1 = triangle.getXa();
	y1 = triangle.getYa();
	x2 = triangle.getXb();
	y2 = triangle.getYb();
	x3 = triangle.getXc();
	y3 = triangle.getYc();

	double x_x3 = x - x3;
	double y_y3 = y - y3;
	double det = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);

	m_alpha = ((y2 - y3) * x_x3 + (x3 - x2) * y_y3) / det;
	m_beta = ((y3 - y1) * x_x3 + (x1 - x3) * y_y3) / det;
}

double iABCoord::getAlpha() const
{
	return m_alpha;
}

double iABCoord::getBeta() const
{
	return m_beta;
}

double iABCoord::getGamma() const
{
	return 1 - m_alpha - m_beta;
}

bool iABCoord::isInside() const
{
	return m_alpha >= 0 && m_beta >= 0 && m_alpha + m_beta <= 1;
}
