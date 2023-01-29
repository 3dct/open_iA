// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	Container & GetDataContainer();
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
