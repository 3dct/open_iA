// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVec3.h"

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
	//! generate fiber data from given table, mapping, and optional curved points
	iAFiberData(vtkTable* table, size_t fiberID, QMap<uint, uint> const & mapping, std::vector<iAVec3f> curvedPts /*= std::vector<iAVec3f>()*/);
	//! generate from a vector containing start, middle end points (each 3 coordinates), phi, theta, length and diameter (i.e. 13 values overall)
	iAFiberData(std::vector<double> const & data);
	static iAFiberData getOrientationCorrected(iAFiberData const & source, iAFiberData const & other);
};

//! check if a point is contained in a fiber
bool pointContainedInFiber(iAVec3f const& point, iAFiberData const& fiber);
//! Samples points inside of the cylinder spanned by a single fiber
void samplePoints(iAFiberData const& fiber, std::vector<iAVec3f>& result, size_t numSamples = DefaultSamplePoints, double RadiusFactor = 1.0);

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

QStringList getAvailableDissimilarityMeasureNames();
