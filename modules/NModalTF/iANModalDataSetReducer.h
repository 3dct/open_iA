// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QList>

#include <memory>

class iAImageData;

class iANModalDataSetReducer
{
public:
	virtual ~iANModalDataSetReducer(){}
	virtual QList<std::shared_ptr<iAImageData>> reduce(const QList<
		std::shared_ptr<iAImageData>>&) = 0;  // TODO: make input and output std::vector<vtkSmartPointer<vtkImageData>>
	virtual int maxOutputLength() final
	{
		return 4;
	}
};
