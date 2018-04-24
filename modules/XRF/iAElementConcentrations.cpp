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
#include "iAElementConcentrations.h"

#include "iAAccumulatedXRFData.h"
#include "iAElementalDecomposition.h"
#include "iAElementSpectralInfo.h"
#include "iAMathUtility.h"
#include "iAXRFData.h"

#include <vtkImageData.h>
#include <vtkVersion.h>

iAElementConcentrations::iAElementConcentrations():
	m_elementCount(0)
{
}


iAElementConcentrations::~iAElementConcentrations()
{
}

bool iAElementConcentrations::calculateAverageConcentration(
	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > elements,
	QSharedPointer<iAAccumulatedXRFData const> accumulatedXRF)
{
	int threshold = accumulatedXRF->YBounds()[1]/20;

	iAEnergySpectrum unknownSpectrum;
	for (size_t i=0; i<accumulatedXRF->GetNumBin(); ++i)
	{
		unknownSpectrum.push_back(accumulatedXRF->GetAvgData()[i]);
	}
	return fitSpectrum(unknownSpectrum, elements, threshold, m_averageConcentration);
}


bool iAElementConcentrations::calculateAverageConcentration(
	QSharedPointer<iAXRFData const> xrfData,
	QVector<iAElementSpectralInfo*> const & elements,
	QSharedPointer<iAAccumulatedXRFData const> accumulatedXRF)
{
	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > adaptedElementSpectra =
		GetAdaptedSpectra(xrfData, elements);
	return calculateAverageConcentration(adaptedElementSpectra, accumulatedXRF);
}


bool iAElementConcentrations::hasAvgConcentration() const
{
	return m_averageConcentration.size() > 0;
}


iAElementConcentrations::ImageListType * iAElementConcentrations::getImageListPtr()
{
	return &m_ElementConcentration;
}


iAElementConcentrations::ImagePointerType iAElementConcentrations::getImage(int idx)
{
	return m_ElementConcentration[idx];
}


void iAElementConcentrations::initImages(int elemCount, int extent[6], double spacing[3], double origin[3])
{
	for (int i=0; i<elemCount; ++i)
	{
		ImagePointerType newImage(ImagePointerType::New());
		newImage->SetSpacing(spacing);
		newImage->SetOrigin(origin);
		newImage->SetExtent(extent);
		newImage->AllocateScalars(VTK_FLOAT, 1);
		m_ElementConcentration.push_back(newImage);
	}
}


iAElementConcentrations::VoxelConcentrationType
	iAElementConcentrations::getConcentrationForVoxel(int x, int y, int z)
{
	VoxelConcentrationType result;
	for (int i=0; i<m_ElementConcentration.size(); ++i)
	{
		result.push_back(m_ElementConcentration[i]->GetScalarComponentAsFloat(x, y, z, 0));
	}
	return result;
}


iAElementConcentrations::VoxelConcentrationType
	const & iAElementConcentrations::getAvgConcentration()
{
	return m_averageConcentration;
}

void iAElementConcentrations::clear()
{
	m_ElementConcentration.clear();
}


QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > iAElementConcentrations::GetAdaptedSpectra(
	QSharedPointer<iAXRFData const> xrfData,
	QVector<iAElementSpectralInfo*> const & elements)
{
	// sample reference spectra to be in the same range as object's spectra
	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > adaptedElementSpectra
		(new QVector<QSharedPointer<iAEnergySpectrum> >(elements.size()));
	int objSpectrumSize = static_cast<int>(xrfData->size());
	double minObjEnergy = xrfData->GetMinEnergy();
	double maxObjEnergy = xrfData->GetMaxEnergy();
	for (int i=0; i<elements.size(); ++i)
	{
		QSharedPointer<iAEnergySpectrum> spectrum = QSharedPointer<iAEnergySpectrum>(new iAEnergySpectrum(objSpectrumSize));

		double minRefEnergy = elements[i]->GetEnergyData()[0];
		double maxRefEnergy = elements[i]->GetEnergyData()[elements[i]->GetEnergyData().size()-1];

		int refSpectrumSize = elements[i]->GetEnergyData().size();
		for (int objChannel=0; objChannel < objSpectrumSize; ++objChannel)
		{
			double energy = mapNormTo(minObjEnergy, maxObjEnergy, mapToNorm(0, objSpectrumSize-1, objChannel));
			if (energy < minRefEnergy || energy > maxRefEnergy)
			{
				(*spectrum)[objChannel] = 0;
			}
			else
			{
				// TODO: interpolate?
				int refIdx = mapNormTo(0, refSpectrumSize, mapToNorm(minRefEnergy, maxRefEnergy, energy));
				(*spectrum)[objChannel] = elements[i]->GetCountsData()[refIdx];
			}
		}
		(*adaptedElementSpectra)[i] = spectrum;
	}
	return adaptedElementSpectra;
}
