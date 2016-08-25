/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iAEnergySpectrumDiagramData.h"

#include "iAXRFData.h"

#include <vtkImageData.h>

iAEnergySpectrumDiagramData::iAEnergySpectrumDiagramData(iAXRFData * xrfData):
	m_energyFunction(NULL),
	m_xrfData_ext(xrfData)
{
}

iAEnergySpectrumDiagramData::~iAEnergySpectrumDiagramData()
{
	delete [] m_energyFunction;
}

void iAEnergySpectrumDiagramData::updateEnergyFunction(int x, int y, int z)
{
	if (!m_energyFunction)
	{
		m_energyFunction = new DataType[m_xrfData_ext->size()];
	}
	int extent[6];
	m_xrfData_ext->GetExtent(extent);
	if (x < extent[0] || x > extent[1] ||
		y < extent[2] || y > extent[3] ||
		z < extent[4] || z > extent[5])
	{
		std::fill_n(m_energyFunction, m_xrfData_ext->size(), 0);
		return;
	}
	iAXRFData::Iterator it= m_xrfData_ext->begin();
	int idx = 0;
	while (it != m_xrfData_ext->end())
	{
		m_energyFunction[idx] = static_cast<DataType>((*it)->GetScalarComponentAsFloat(x, y, z, 0));
		++it;
		++idx;
	}
}

iAAbstractDiagramData::DataType const * iAEnergySpectrumDiagramData::GetData() const
{
	return m_energyFunction;
}

size_t iAEnergySpectrumDiagramData::GetNumBin() const
{
	return m_xrfData_ext->size();
}

