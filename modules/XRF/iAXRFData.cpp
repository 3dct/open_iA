/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAXRFData.h"

#include "iASpectrumFilter.h"

#include <iAMathUtility.h>
#include <iATypedCallHelper.h>

#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkImageData.h>

#include <QThread>

#include <map>
#include <cassert>

iAXRFData::Iterator iAXRFData::begin() const
{
	return m_data.begin();
}

iAXRFData::Iterator iAXRFData::end() const
{
	return m_data.end();
}

size_t iAXRFData::size() const
{
	return m_data.size();
}

iAXRFData::Container * iAXRFData::GetDataPtr()
{
	return &m_data;
}

vtkSmartPointer<vtkImageData> const & iAXRFData::image(size_t idx) const
{
	return m_data[idx];
}

void iAXRFData::GetExtent(int extent[6]) const
{
	if (m_data.size() <= 0)
	{
		extent[0] = extent[1] = extent[2] = extent[3] = extent[4] = extent[5] = 0;
		return;
	}
	m_data[0]->GetExtent(extent);
}

namespace
{
	const size_t COLOR_COMPONENTS = 3;		// number of components for each color (e.g. 3 -> r,g,b or 4 -> r,g,b,a)
	const int COLOR_RANGE = 255;			// number of different values for each color component (e.g. [0..]255)
	const int COLOR_BITS  = 8;				// number of bits required for maximum color value (i.e., should be log2(COLOR_RANGE)
	const unsigned long COLOR_COMP_BITMASK = 0xFF;
}

template <typename T>
void addValues(double* result, void* data, size_t voxelCount, double energyColor[COLOR_COMPONENTS], double &componentMax)
{
	T* energyCounts = static_cast<T*>(data);
	for (int i=0; i<voxelCount; ++i)
	{
		// color = value of colorTransferFunction for this energy (volume)
		for (int comp = 0; comp<COLOR_COMPONENTS; ++comp)
		{
			size_t idx = COLOR_COMPONENTS*i+comp;
			double toAdd = energyCounts[i] * energyColor[comp];
			result[idx] += toAdd;
			if (result[idx] > componentMax)
			{
				componentMax = result[idx];
			}
		}
	}
}

class iAXRFCombinedVolumeUpdater: public QThread
{
public:
	iAXRFCombinedVolumeUpdater(iAXRFData& xrfData, vtkSmartPointer<vtkColorTransferFunction> colorTransferEnergies):
		xrfData(xrfData),
		colorTransferEnergies(colorTransferEnergies)
	{}
	void run()
	{
		if (xrfData.begin() == xrfData.end())
		{
			return;
		}
		int extent[6];
		vtkSmartPointer<vtkImageData> refImg = *xrfData.begin();
		refImg->GetExtent(extent);
		double spacing[3]; refImg->GetSpacing(spacing);
		double origin[3]; refImg->GetOrigin(origin);
		int xrange = extent[1]-extent[0]+1;
		int yrange = extent[3]-extent[2]+1;
		int zrange = extent[5]-extent[4]+1;
		size_t voxelCount = xrange*yrange*zrange;
		size_t colorValueCount = COLOR_COMPONENTS * voxelCount;

		double * colorValues = new double [colorValueCount];
		std::fill_n(colorValues, colorValueCount, 0.0);

		// first, add up all energy counts multiplied by the respective color component
		double componentMax(0.0);
		for (size_t i=0; i<xrfData.size(); ++i)
		{
			vtkSmartPointer<vtkImageData> countsForEnergy = xrfData.image(i);
			assert (countsForEnergy->GetNumberOfScalarComponents() == 1);
			int type = countsForEnergy->GetScalarType();

			double energyColor[COLOR_COMPONENTS];
			double energyIndex = mapNormTo(xrfData.GetMinEnergy(), xrfData.GetMaxEnergy(),
				mapToNorm(static_cast<size_t>(0), xrfData.size(), i));
			colorTransferEnergies->GetColor(
				energyIndex,
				energyColor);
			VTK_TYPED_CALL(addValues, type, colorValues, countsForEnergy->GetScalarPointer(), voxelCount, energyColor, componentMax);
		}

		vtkSmartPointer<vtkImageData> combinedVolume = vtkSmartPointer<vtkImageData>::New();
		combinedVolume->SetExtent(extent);
		combinedVolume->SetOrigin(origin);
		combinedVolume->SetSpacing(spacing);
		combinedVolume->AllocateScalars(VTK_UNSIGNED_LONG, 1);

		unsigned long* result = static_cast<unsigned long*>(combinedVolume->GetScalarPointer());

		// normalize colors by the maximum color component encountered;
		// build a color map of all resulting colors; and insert indices to this color map instead of actual colors
		std::map<unsigned long, unsigned long> colorMap;
		unsigned long lastColorIdx = 0;
		for (int i=0; i < voxelCount; i++)
		{
			unsigned long colorKey = 0;
			for (int comp = 0; comp<COLOR_COMPONENTS; ++comp)
			{
				colorValues[COLOR_COMPONENTS*i+comp] = colorValues[COLOR_COMPONENTS*i+comp] / componentMax * COLOR_RANGE;
				unsigned long curColorValue = static_cast<unsigned long>(colorValues[COLOR_COMPONENTS*i+comp]);
				colorKey <<= COLOR_BITS;
				colorKey |=  curColorValue;
			}
			std::map<unsigned long, unsigned long>::const_iterator it = colorMap.find(colorKey);
			if (it == colorMap.end())
			{
				colorMap.insert(std::make_pair(colorKey, lastColorIdx));
				result[i] = lastColorIdx;
				lastColorIdx++;
			}
			else
			{
				result[i] = it->second;
			}
		}

		// from the color map, build a color transfer function
		vtkSmartPointer<vtkDiscretizableColorTransferFunction> colorTransfer = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
		colorTransfer->DiscretizeOn();
		colorTransfer->SetNumberOfValues(colorMap.size());
		for (std::map<unsigned long, unsigned long>::const_iterator it = colorMap.begin(); it != colorMap.end(); ++it)
		{
			unsigned long colorVal = it->first;
			double colorComponents[COLOR_COMPONENTS];
			for (int i=COLOR_COMPONENTS-1; i>=0; --i)
			{
				colorComponents[i] = static_cast<double>(colorVal & COLOR_COMP_BITMASK) / COLOR_RANGE;
				colorVal >>= COLOR_BITS;
			}
			colorTransfer->AddRGBPoint(it->second, colorComponents[0], colorComponents[1], colorComponents[2]);
		}
		colorTransfer->Build();

		xrfData.SetColorTransferFunction(colorTransfer);
		xrfData.SetCombinedImage(combinedVolume);

		delete [] colorValues;
	}
private:
	iAXRFData& xrfData;
	vtkSmartPointer<vtkColorTransferFunction> colorTransferEnergies;
};

void iAXRFData::SetColorTransferFunction(vtkSmartPointer<vtkDiscretizableColorTransferFunction> ctf)
{
	m_colorTransfer = ctf;
}

void iAXRFData::SetCombinedImage(vtkSmartPointer<vtkImageData> img)
{
	m_combinedVolume = img;
}

QThread* iAXRFData::UpdateCombinedVolume(vtkSmartPointer<vtkColorTransferFunction> colorTransferEnergies)
{
	iAXRFCombinedVolumeUpdater* updater = new iAXRFCombinedVolumeUpdater(*this, colorTransferEnergies);
	updater->start();
	return updater;
}

vtkSmartPointer<vtkImageData> iAXRFData::GetCombinedVolume()
{
	return m_combinedVolume;
}

vtkSmartPointer<vtkDiscretizableColorTransferFunction> iAXRFData::GetColorTransferFunction()
{
	return m_colorTransfer;
}

template <typename T>
void isInRange(void* data, double const & minVal, double const & maxVal, bool & inRange)
{
	T value = *static_cast<char*>(data);
	inRange = (value >= minVal && value <= maxVal);
}

bool isInRange(vtkSmartPointer<vtkImageData> const & img, int x, int y, int z, double const & minVal, double const & maxVal)
{
	bool inRange = false;
	VTK_TYPED_CALL(isInRange, img->GetScalarType(), img->GetScalarPointer(x, y, z), minVal, maxVal, inRange);
	return inRange;
}

bool iAXRFData::CheckFilters(int x, int y, int z, QVector<iASpectrumFilter> const & filter, iAFilterMode mode) const
{
	for (QVector<iASpectrumFilter>::const_iterator it = filter.begin(); it != filter.end(); ++it)
	{
		bool inRange = isInRange(m_data[it->binIdx], x, y, z, it->minVal, it->maxVal);
		switch (mode)
		{
			case filter_AND: if (!inRange) { return 0; } break;
			case filter_OR : if ( inRange) { return 1; } break;
		}
	}
	return (mode == filter_AND)? 1 : /* filter_OR */ 0;
}

vtkSmartPointer<vtkImageData> iAXRFData::FilterSpectrum(QVector<iASpectrumFilter> const & filter, iAFilterMode mode)
{
	vtkSmartPointer<vtkImageData> result = vtkSmartPointer<vtkImageData>::New();

	if (m_data.empty())
	{
		return vtkSmartPointer<vtkImageData>();
	}
	int extent[6];
	m_data[0]->GetExtent(extent);
	double spacing[3]; m_data[0]->GetSpacing(spacing);
	double origin[3];  m_data[0]->GetOrigin(origin);

	result->SetExtent(extent);
	result->SetOrigin(origin);
	result->SetSpacing(spacing);
	result->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

#pragma omp parallel for
	for (int x=extent[0]; x<=extent[1]; ++x)
	{
		for (int y=extent[2]; y<=extent[3]; ++y)
		{
			for (int z=extent[4]; z<=extent[5]; ++z)
			{
				*(static_cast<unsigned char*>(result->GetScalarPointer(x, y, z))) = CheckFilters(x, y, z, filter, mode) ? 1 : 0;
			}
		}
	}
	return result;
}

void iAXRFData::SetEnergyRange(double minEnergy, double maxEnergy)
{
	m_minEnergy = minEnergy;
	m_maxEnergy = maxEnergy;
}


double iAXRFData::GetMinEnergy() const
{
	return m_minEnergy;
}

double iAXRFData::GetMaxEnergy() const
{
	return m_maxEnergy;
}
