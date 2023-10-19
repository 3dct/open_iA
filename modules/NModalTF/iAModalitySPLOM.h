// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QDockWidget>

#include <memory>
#include <vector>

class iAImageData;
class iAQSplom;
class iASPLOMData;

class vtkColorTransferFunction;
class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;

class iAModalitySPLOM: public QDockWidget
{
	Q_OBJECT
public:
	iAModalitySPLOM();
	void setData(std::vector<iAImageData*> dataSets);
private slots:
	void splomSelection(std::vector<size_t> const &);
private:
	iAQSplom* m_splom;
	std::shared_ptr<iASPLOMData> m_data;
	int m_extent[6];
	double m_spacing[3];
	double m_origin[3];
	vtkSmartPointer<vtkColorTransferFunction> m_selection_ctf;
	vtkSmartPointer<vtkPiecewiseFunction> m_selection_otf;
	bool m_selected;
	uint m_SPLOMSelectionChannelID;
};
