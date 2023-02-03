// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class iAVolumeSettings
{
public:
	bool   LinearInterpolation;
	bool   Shading;
	double AmbientLighting;
	double DiffuseLighting;
	double SpecularLighting;
	double SpecularPower;
	double SampleDistance;
	double ScalarOpacityUnitDistance;
	int RenderMode;                       //!< a value out of the unnamed enum in vtkSmartVolumeMapper (e.g. DefaultRenderMode)

	iAVolumeSettings() :
		LinearInterpolation(true),
		Shading(true),
		AmbientLighting(0.2),
		DiffuseLighting(0.5),
		SpecularLighting(0.7),
		SpecularPower(10.0),
		SampleDistance(1.0),
		ScalarOpacityUnitDistance(-1.0),
		RenderMode(0) // 0 = DefaultRenderMode
	{}
};
