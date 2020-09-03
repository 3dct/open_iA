/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAModalityTransfer.h"

#include "charts/iAHistogramData.h"

#include <vtkImageData.h>

#include <cassert>

iAModalityTransfer::iAModalityTransfer(double range[2]):
	m_statisticsComputed(false)
{
	m_ctf = defaultColorTF(range);
	m_otf = defaultOpacityTF(range, true);
}

void iAModalityTransfer::computeStatistics(vtkSmartPointer<vtkImageData> img)
{
	if (m_statisticsComputed)	// already calculated
		return;
	defaultColorTF(m_ctf, img->GetScalarRange()); // Set range of rgb, rgba or vector pixel type images to fully opaque
	defaultOpacityTF(m_otf, img->GetScalarRange(), img->GetNumberOfScalarComponents() == 1);
	m_statisticsComputed = true;
}

void iAModalityTransfer::reset()
{
	m_statisticsComputed = false;
	m_histogramData.clear();
}

void iAModalityTransfer::computeHistogramData(vtkSmartPointer<vtkImageData> imgData, size_t binCount)
{
	if (imgData->GetNumberOfScalarComponents() != 1 || (m_histogramData && m_histogramData->numBin() == binCount))
		return;
	m_histogramData = iAHistogramData::create(imgData, binCount, &m_imageInfo);
}

QSharedPointer<iAHistogramData> const iAModalityTransfer::histogramData() const
{
	return m_histogramData;
}

vtkPiecewiseFunction* iAModalityTransfer::opacityTF()
{
	assert(m_otf);
	return m_otf;
}

vtkColorTransferFunction* iAModalityTransfer::colorTF()
{
	assert(m_ctf);
	return m_ctf;
}

iAImageInfo const & iAModalityTransfer::info() const
{
	// TODO: make sure image info is initialzed!
	return m_imageInfo;
}

bool iAModalityTransfer::statisticsComputed() const
{
	return m_statisticsComputed;
}
