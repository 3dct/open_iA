/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iADecompositionCalculator.h"

#include "iAAccumulatedXRFData.h"
#include "iAElementConcentrations.h"
#include "iAElementalDecomposition.h"
#include "iAXRFData.h"

#include <vtkImageData.h>

iADecompositionCalculator::iADecompositionCalculator(
	QSharedPointer<iAElementConcentrations> data,
	QSharedPointer<iAXRFData const> xrfData,
	QSharedPointer<iAAccumulatedXRFData const> accumulatedXRF
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

int iADecompositionCalculator::ElementCount() const
{
	return m_elements.size();
}

void iADecompositionCalculator::Stop()
{
	m_stopped = true;
	wait();
}

void iADecompositionCalculator::run()
{
	int threshold = m_accumulatedXRF->GetMaxValue()/20;

	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > >  adaptedElementSpectra =
		m_data->GetAdaptedSpectra(m_xrfData, m_elements);

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
			int percent = static_cast<int>(static_cast<double>(x*height*depth + y*depth)/pixelCount*100);
			progress(percent);
		}
	}
	if (!m_stopped)
	{
		emit success();
	}
}
