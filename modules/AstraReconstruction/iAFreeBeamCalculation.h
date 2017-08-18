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

enum iAFreeBeamCalculationType
{
	FREEBEAMCALCULATION,
};

/**
 * FreeBeamCalculation filter
 * refer to http://www.itk.org/Doxygen/html/classitk_1_1ExtractImageFilter.html#_details.
 */
class iAFreeBeamCalculation : public iAAlgorithm
{
public:
	iAFreeBeamCalculation(QString fn, iAFreeBeamCalculationType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0);

	/**
	 * Sets an e parameters.
	 * \param	oX	Origin x coordinate.
	 * \param	oY	Origin y coordinate.
	 * \param	oZ	Origin z coordinate.
	 * \param	sX	Size x coordinate.
	 * \param	sY	Size y coordinate.
	 * \param	sZ	Size z coordinate.
	 * \param	d	Dimensions.
	 */

	void setEParameters(double oX, double oY, double oZ,
		double sX, double sY, double sZ, bool mmfbi, int mmfbiv)
	{
		originX = oX;
		originY = oY;
		originZ = oZ;
		sizeX = sX;
		sizeY = sY;
		sizeZ = sZ;
		manualMeanFreeBeamIntensity = mmfbi;
		manualMeanFreeBeamIntensityValue = mmfbiv;
	}

protected:
	virtual void performWork();
private:
	double originX, originY, originZ, spacingX, spacingY, spacingZ, sizeX, sizeY, sizeZ;
	bool manualMeanFreeBeamIntensity;
	int manualMeanFreeBeamIntensityValue;
	iAFreeBeamCalculationType m_operation;
};
