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

#include "iAvec3.h"

#include <QMap>

#include <vector>

typedef iAVec3T<double> Vec3D;

class vtkTable;

enum {
	PtStart = 0,
	PtCenter,
	PtEnd
};

const int DefaultSamplePoints = 200;

struct iAFiberData
{
	double phi, theta, length, diameter;
	Vec3D pts[3];
	iAFiberData();
	iAFiberData(vtkTable* table, size_t fiberID, QMap<uint, uint> const & mapping);
	iAFiberData(std::vector<double> const & data);
	static iAFiberData getOrientationCorrected(iAFiberData const & source, iAFiberData const & other);
};

void samplePoints(iAFiberData const & fiber, std::vector<Vec3D> & result, size_t numSamples=DefaultSamplePoints);
double getDistance(iAFiberData const & fiber1raw, iAFiberData const & fiber2,
	int distanceMeasure, double diagonalLength, double maxLength);