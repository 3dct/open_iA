// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QList>
#include <QSharedPointer>

class vtkImageData;

class iAImageData;

class iANModalBackgroundRemover
{
public:
	enum OutputType
	{
		BACKGROUND = 0,
		FOREGROUND = 1
	};
	enum MaskMode
	{
		INVALID,
		NONE,
		REMOVE,
		HIDE
	};
	struct Mask
	{
		vtkSmartPointer<vtkImageData> mask;
		MaskMode maskMode;
	};
	virtual ~iANModalBackgroundRemover(){};
	virtual Mask removeBackground(
		const QList<std::shared_ptr<iAImageData>>&) = 0;  // TODO: make input std::vector<vtkSmartPointer<vtkImageData>>
};
