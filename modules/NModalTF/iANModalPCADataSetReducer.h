// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iANModalDataSetReducer.h"

#include <iAConnector.h>

#include <vtkSmartPointer.h>

class vtkImageData;

class iANModalPCADataSetReducer : public iANModalDataSetReducer
{
public:
	QList<std::shared_ptr<iAImageData>> reduce(const QList<std::shared_ptr<iAImageData>>&) override;

private:
	template <class T>
	void itkPCA(std::vector<iAConnector>& connectors);
	template <class T>
	void ownPCA(std::vector<iAConnector>& connectors);
};
