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

	ImageListType * getImageListPtr();

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
