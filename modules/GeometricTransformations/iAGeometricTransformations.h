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

enum iAGeometricTransformationType
{
	EXTRACT_IMAGE,
	RESAMPLER,
};

/**
 * Geometric transformation filters
 * For Resample ImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html.
 * For Extract ImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1ExtractImageFilter.html#_details.
 */
class iAGeometricTransformations : public iAAlgorithm
{
public:
	static const QString InterpLinear;
	static const QString InterpNearestNeighbour;
	static const QString InterpBSpline;
	static const QString InterpWindowedSinc;

	iAGeometricTransformations( QString fn, iAGeometricTransformationType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	/**
	 * Sets a r parameters. 
	 * \param	oX	Origin x coordinate. 
	 * \param	oY	Origin y coordinate. 
	 * \param	oZ	Origin z coordinate. 
	 * \param	spX	Spacing x coordinate. 
	 * \param	spY	Spacing y coordinate. 
	 * \param	spZ	Spacing z coordinate. 
	 * \param	sX	Size x coordinate. 
	 * \param	sY	Size y coordinate. 
	 * \param	sZ	Size z coordinate. 
	 */

	void setRParameters(double oX, double oY, double oZ, 
		double spX, double spY, double spZ, 
		double sX, double sY, double sZ,
		QString const & interp)
	{
		originX = oX;
		originY = oY;
		originZ = oZ;
		spacingX = spX;
		spacingY = spY;
		spacingZ = spZ;
		sizeX = sX;
		sizeY = sY;
		sizeZ = sZ;
		interpolator = interp;
	}

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
		double sX, double sY, double sZ, unsigned int d = 3)
	{
		originX = oX;
		originY = oY;
		originZ = oZ;
		sizeX = sX;
		sizeY = sY;
		sizeZ = sZ;
		dim = d; 
	}

protected:
	void run();
private:
	double originX, originY, originZ, spacingX, spacingY, spacingZ, sizeX, sizeY, sizeZ;
	QString interpolator;
	unsigned int dim;
	iAGeometricTransformationType m_operation;
	void extractImage();
	void resampler();
};
