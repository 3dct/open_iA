// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

class vtkImageData;

typedef vtkSmartPointer<vtkImageData> vtkImagePointer;

class QString;

class iAUncertaintyImages
{
public:
	enum SourceType
	{
		AvgAlgorithmEntropyEntrSum,
		Neighbourhood3x3Entropy,
		LabelDistributionEntropy,
		SourceCount
	};
	virtual ~iAUncertaintyImages(); //!< implementation in iAEnsemble.cpp
	virtual vtkImagePointer GetEntropy(int source) const =0;
	virtual vtkImagePointer GetReference() const =0;
	virtual QString GetSourceName(int source) const =0;
	virtual bool HasReference() const = 0;
};
