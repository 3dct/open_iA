// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAEnergySpectrum.h"

#include <vtkSmartPointer.h>

#include <QVector>

#include <memory>
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
		iAXRFData const * xrfData,
		QVector<iAElementSpectralInfo*> const & elements,
		iAAccumulatedXRFData const * accumulatedXRF);
	bool hasAvgConcentration() const;
private:
	void initImages(int elemCount, int extent[6], double spacing[3], double origin[3]);
	std::shared_ptr<QVector<std::shared_ptr<iAEnergySpectrum> > > GetAdaptedSpectra(
		iAXRFData const * xrfData,
		QVector<iAElementSpectralInfo*> const & elements);
	bool calculateAverageConcentration(
		std::shared_ptr<QVector<std::shared_ptr<iAEnergySpectrum> > > elements,
		iAAccumulatedXRFData const * accumulatedXRF);

	int m_elementCount;
	VoxelConcentrationType  m_averageConcentration;
	ImageListType m_ElementConcentration;

	friend class iADecompositionCalculator;
};
