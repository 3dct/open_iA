// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAElementConcentrations.h"

#include "iAAccumulatedXRFData.h"
#include "iAElementalDecomposition.h"
#include "iAElementSpectralInfo.h"
#include "iAXRFData.h"

#include <iAMathUtility.h>

#include <vtkImageData.h>

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
	int threshold = accumulatedXRF->yBounds()[1]/20;

	iAEnergySpectrum unknownSpectrum;
	for (size_t i=0; i<accumulatedXRF->valueCount(); ++i)
	{
		unknownSpectrum.push_back(accumulatedXRF->avgData()[i]);
	}
	return fitSpectrum(unknownSpectrum, elements, threshold, m_averageConcentration);
}


bool iAElementConcentrations::calculateAverageConcentration(
	QSharedPointer<iAXRFData const> xrfData,
	QVector<iAElementSpectralInfo*> const & elements,
	QSharedPointer<iAAccumulatedXRFData const> accumulatedXRF)
{
	auto adaptedElementSpectra = GetAdaptedSpectra(xrfData, elements);
	return calculateAverageConcentration(adaptedElementSpectra, accumulatedXRF);
}


bool iAElementConcentrations::hasAvgConcentration() const
{
	return m_averageConcentration.size() > 0;
}


iAElementConcentrations::ImageListType& iAElementConcentrations::getImageList()
{
	return m_ElementConcentration;
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
	for (size_t i=0; i<m_ElementConcentration.size(); ++i)
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
	auto adaptedElementSpectra = QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum>>>::create(elements.size());
	int objSpectrumSize = static_cast<int>(xrfData->size());
	double minObjEnergy = xrfData->GetMinEnergy();
	double maxObjEnergy = xrfData->GetMaxEnergy();
	for (int i=0; i<elements.size(); ++i)
	{
		auto spectrum = QSharedPointer<iAEnergySpectrum>::create(objSpectrumSize);

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
