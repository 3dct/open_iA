// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAEnergySpectrum.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QVector>

#include <vector>

class vtkImageData;

class iAAccumulatedXRFData;
class iAElementSpectralInfo;
class iAXRFData;

class iAElementConcentrations
{
public:
	typedef vtkImageData ImageType;
	typedef vtkSmartPointer<ImageType> ImagePointerType;
	typedef QVector<double> VoxelConcentrationType;
	typedef std::vector<ImagePointerType> ImageListType;

	iAElementConcentrations();
	~iAElementConcentrations();

	ImageListType& getImageList();

	ImagePointerType getImage(int idx);

	VoxelConcentrationType getConcentrationForVoxel(int x, int y, int z);
	VoxelConcentrationType const & getAvgConcentration();
	void clear();
	bool calculateAverageConcentration(
		QSharedPointer<iAXRFData const> xrfData,
		QVector<iAElementSpectralInfo*> const & elements,
		QSharedPointer<iAAccumulatedXRFData const> accumulatedXRF);
	bool hasAvgConcentration() const;
private:
	void initImages(int elemCount, int extent[6], double spacing[3], double origin[3]);
	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > GetAdaptedSpectra(
		QSharedPointer<iAXRFData const> xrfData,
		QVector<iAElementSpectralInfo*> const & elements);
	bool calculateAverageConcentration(
		QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > elements,
		QSharedPointer<iAAccumulatedXRFData const> accumulatedXRF);

	int m_elementCount;
	VoxelConcentrationType  m_averageConcentration;
	ImageListType m_ElementConcentration;

	friend class iADecompositionCalculator;
};
