/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <QColor>
#include <QString>

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
	QColor BackgroundColor;
	int Mode;                       //!< a value out of the unnamed enum in vtkSmartVolumeMapper (e.g. DefaultRenderMode)

	iAVolumeSettings() :
		LinearInterpolation(true),
		Shading(true),
		AmbientLighting(0.2),
		DiffuseLighting(0.5),
		SpecularLighting(0.7),
		SpecularPower(10),
		SampleDistance(1),
		Mode(0)
	{}
};
