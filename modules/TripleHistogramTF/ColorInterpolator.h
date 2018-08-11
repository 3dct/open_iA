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

class ColorInterpolator
{
public:
	// Abstract
	virtual void interpolateColor2(double c1[3], double c2[3], double t, double result[3]) = 0;
	virtual void interpolateColor3(double c1[3], double c2[3], double c3[3], double alpha, double beta, double result[3]) = 0;

	// Implemented
	virtual double interpolateAlpha2(double a1, double a2, double t) {
		return (1 - t) * a1 + t * a2;
	}
	virtual double interpolateAlpha3(double a1, double a2, double a3, double alpha, double beta) {
		double gamma = 1 - alpha - beta;
		return alpha * a1 + beta * a2 + gamma * a3;
	}

	// Singleton controls
	static ColorInterpolator *getInstance() { return m_instance; }
	static void setInstance(ColorInterpolator *newInstance) { m_instance = newInstance; }
	static bool hasInstance() { return m_instance; }
	void makeGlobal() { m_instance = this; }

private:
	static ColorInterpolator *m_instance;

};

class LinearRGBColorInterpolator : public ColorInterpolator
{
	void interpolateColor2(double c1[3], double c2[3], double t, double result[3]) override
	{
		double oneMinusT = 1 - t;
		result[0] = oneMinusT * c1[0] + t * c2[0];
		result[1] = oneMinusT * c1[1] + t * c2[1];
		result[2] = oneMinusT * c1[2] + t * c2[2];
	}
	void interpolateColor3(double c1[3], double c2[3], double c3[3], double alpha, double beta, double result[3]) override
	{
		double gamma = 1 - alpha - beta;
		result[0] = alpha * c1[0] + beta * c2[0] + gamma * c3[0];
		result[1] = alpha * c1[1] + beta * c2[1] + gamma * c3[1];
		result[2] = alpha * c1[2] + beta * c2[2] + gamma * c3[2];
	}
};