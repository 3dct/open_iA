// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

//! @deprecated to be removed soon, replaced by iADataSetRenderer / iAVolumeRenderer settings (see m_attributes member)
class iAguibase_API iAVolumeSettings
{
public:
	//! @{ general rendering settings
	bool   Shading;
	double AmbientLighting;
	double DiffuseLighting;
	double SpecularLighting;
	double SpecularPower;
	//! @}
	//! @{ volume rendering settings
	bool   LinearInterpolation;
	double SampleDistance;
	double ScalarOpacityUnitDistance;
	int RenderMode;                       //!< a value out of the unnamed enum in vtkSmartVolumeMapper (e.g. DefaultRenderMode)
	//! @}

	// implementation in iAVolumeRenderer.cpp
	iAVolumeSettings();
	QVariantMap toMap() const;
};
