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

class BarycentricTriangle;

class BCoord
{
public:
	BCoord(double alpha, double beta);
	BCoord() : BCoord((double)1 / (double)3, (double)1 / (double)3) {}
	BCoord(BarycentricTriangle triangle, double x, double y);

	double getAlpha() const;
	double getBeta() const;
	double getGamma() const;
	bool isInside() const;

	double operator[] (int x) {
		switch (x) {
		case 0: return getAlpha();
		case 1: return getBeta();
		case 2: return getGamma();
		default: return 0;
		}
	}

	bool operator== (const BCoord that) {
		return getAlpha() == that.getAlpha() && getBeta() == that.getBeta();
	}

	bool operator!= (const BCoord that) {
		return getAlpha() != that.getAlpha() || getBeta() != that.getBeta();
	}

private:
	double m_alpha;
	double m_beta;

};
