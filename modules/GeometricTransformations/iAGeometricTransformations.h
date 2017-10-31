/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAAlgorithm.h"
#include "iAFilter.h"
#include "iAFilterRunnerGUI.h"

IAFILTER_DEFAULT_CLASS(iAResampleFilter);

class iAResampleFilterRunner : public iAFilterRunnerGUI
{
public:
	static QSharedPointer<iAFilterRunnerGUI> Create();
	virtual QMap<QString, QVariant> LoadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi);
};

/**
 * Geometric transformation filters
 * For Extract ImageFilter refer to https://itk.org/Doxygen/html/classitk_1_1ExtractImageFilter.html#_details.
 */
class iAGeometricTransformations : public iAAlgorithm
{
public:

	iAGeometricTransformations( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	void setEParameters(double oX, double oY, double oZ, double sX, double sY, double sZ)
	{
		originX = oX;
		originY = oY;
		originZ = oZ;
		sizeX = sX;
		sizeY = sY;
		sizeZ = sZ;
	}
protected:
	virtual void performWork();
private:
	double originX, originY, originZ, spacingX, spacingY, spacingZ, sizeX, sizeY, sizeZ;
	QString interpolator;
};
