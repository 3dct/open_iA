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

#include <vtkSmartPointer.h>

#include <QObject>
#include <QVector>

#include <vector>

class vtkColorTransferFunction;
class vtkDiscretizableColorTransferFunction;
class vtkImageData;

class QThread;

struct iASpectrumFilter;

enum iAFilterMode
{
	filter_AND,
	filter_OR,
};

class iAXRFData {
public:
	typedef std::vector<vtkSmartPointer<vtkImageData> >	Container;
	typedef Container::const_iterator	Iterator;
	Iterator begin() const;
	Iterator end() const;
	size_t size() const;
	Container * GetDataPtr();
	vtkSmartPointer<vtkImageData> const & image(size_t idx) const;
	void GetExtent(int extent[6]) const;
	QThread* UpdateCombinedVolume(vtkSmartPointer<vtkColorTransferFunction> colorTransferEnergies);
	vtkSmartPointer<vtkImageData> GetCombinedVolume();
	vtkSmartPointer<vtkDiscretizableColorTransferFunction> GetColorTransferFunction();

	//! returns a mask image which contains a 1 for a voxel where the counts in all the given filters lie inside
	//! the [min,max] interval specified in the filter
	vtkSmartPointer<vtkImageData> FilterSpectrum(QVector<iASpectrumFilter> const & filter, iAFilterMode mode);
	bool CheckFilters(int x, int y, int z, QVector<iASpectrumFilter> const & filter, iAFilterMode mode) const;
	
	void SetColorTransferFunction(vtkSmartPointer<vtkDiscretizableColorTransferFunction> ctf);
	void SetCombinedImage(vtkSmartPointer<vtkImageData> combinedVolume);

	void SetEnergyRange(double minEnergy, double maxEnergy);
	double GetMinEnergy() const;
	double GetMaxEnergy() const;
private:
	Container m_data;
	vtkSmartPointer<vtkImageData> m_combinedVolume;
	vtkSmartPointer<vtkDiscretizableColorTransferFunction> m_colorTransfer;
	double m_minEnergy, m_maxEnergy;
};
