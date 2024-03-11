// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADecompositionCalculator.h"

#include "iAAccumulatedXRFData.h"
#include "iAElementConcentrations.h"
#include "iAElementalDecomposition.h"
#include "iAXRFData.h"

#include <vtkImageData.h>

iADecompositionCalculator::iADecompositionCalculator(
	std::shared_ptr<iAElementConcentrations> data,
	iAXRFData const * xrfData,
	iAAccumulatedXRFData const * accumulatedXRF
):
	m_data(data),
	m_xrfData(xrfData),
	m_accumulatedXRF(accumulatedXRF),
	m_stopped(false)
{}

void iADecompositionCalculator::AddElement(iAElementSpectralInfo* element)
{
	m_elements.push_back(element);
}

bool iADecompositionCalculator::NoElements() const
{
	return m_elements.isEmpty();
}

void iADecompositionCalculator::Stop()
{
	m_stopped = true;
	wait();
}

iAProgress* iADecompositionCalculator::progress()
{
	return &m_progress;
}

void iADecompositionCalculator::run()
{
	int threshold = m_accumulatedXRF->yBounds()[1]/20;

	auto adaptedElementSpectra = m_data->GetAdaptedSpectra(m_xrfData, m_elements);

	if (!m_data->calculateAverageConcentration(adaptedElementSpectra, m_accumulatedXRF))
	{
		// if average spectrum fitting went wrong, something really strange is going on
		// TODO: Notify user about this!
		return;
	}

	int extent[6];
	m_xrfData->GetExtent(extent);

	vtkSmartPointer<vtkImageData> img = *m_xrfData->begin();
	double spacing[3];
	double origin[3];
	img->GetOrigin(origin);
	img->GetSpacing(spacing);

	m_data->initImages(m_elements.size(), extent, spacing, origin);

	iAElementConcentrations::VoxelConcentrationType concentration;
	concentration.reserve(m_elements.size());
	int width = extent[1]-extent[0]+1;
	int height = extent[3]-extent[2]+1;
	int depth = extent[5]-extent[4]+1;
	int pixelCount = width * height * depth;
	for (int x=extent[0]; x<=extent[1] && !m_stopped; ++x)
	{
		for (int y=extent[2]; y<=extent[3] && !m_stopped; ++y)
		{
#pragma omp parallel for private (concentration)
			for (int z=extent[4]; z<=extent[5]; ++z)
			{
				iAEnergySpectrum unknownSpectrum;
				iAXRFData::Iterator it= m_xrfData->begin();
				while (it != m_xrfData->end())
				{
					unknownSpectrum.push_back(static_cast<unsigned int>((*it)->GetScalarComponentAsFloat(x, y, z, 0)));
					++it;
				}
				concentration.clear();
				fitSpectrum(unknownSpectrum, adaptedElementSpectra, threshold, concentration);
				for (int i=0; i<concentration.size() && !m_stopped; ++i)
				{
					m_data->m_ElementConcentration[i]->SetScalarComponentFromDouble(x, y, z, 0, concentration[i]);
				}
			}
			m_progress.emitProgress((x * height * depth + y * depth) * 100.0 / pixelCount);
		}
	}
	if (!m_stopped)
	{
		emit success();
	}
}
