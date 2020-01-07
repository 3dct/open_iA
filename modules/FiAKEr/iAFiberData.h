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

#include "iAvec3.h"

#include <QMap>

#include <vector>

class vtkTable;

enum {
	PtStart = 0,
	PtCenter,
	PtEnd
};

const int DefaultSamplePoints = 200;

//! Characteristics of a single fiber
struct iAFiberData
{
	double phi, theta, length, diameter;
	std::vector<iAVec3f> pts;
	std::vector<iAVec3f> curvedPoints;
	iAFiberData();
	iAFiberData(vtkTable* table, size_t fiberID, QMap<uint, uint> const & mapping, std::vector<iAVec3f> curvedPts /*= std::vector<iAVec3f>()*/);
	iAFiberData(std::vector<double> const & data);
	static iAFiberData getOrientationCorrected(iAFiberData const & source, iAFiberData const & other);
};

//! Samples points inside of the cylinder spanned by a single fiber
void samplePoints(iAFiberData const & fiber, std::vector<iAVec3f> & result, size_t numSamples=DefaultSamplePoints);

//! Computes the similarity between two fibers according to a given measure
//! @param fiber1raw data of the first fiber
//! @param fiber2 data of the second fiber
//! @param measureID the id of the measure (see implementation for short descriptions of each measure,
//!     also see iARefDistCompute::getSimilarityMeasureNames for names of the measures,
//!     for their mathematical definition see the FIAKER paper (doi: 10.1111/cgf.13688)
//! @param diagonalLength length of the diagonal of the dataset (i.e. the maximum possible length of a fiber)
//! @param maxLength the maximum length difference in the dataset, i.e. the length of the longest fiber
//!     in the dataset minus the length of the shortest one
double getDissimilarity(iAFiberData const & fiber1raw, iAFiberData const & fiber2,
	int measureID, double diagonalLength, double maxLength);
