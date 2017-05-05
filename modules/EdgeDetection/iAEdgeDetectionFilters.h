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

/**
 * Application of edge detection filter. Basic filter is itkCannyEdgeDetectionImageFilter.
 * For further details refer http://www.itk.org/Doxygen/html/classitk_1_1CannyEdgeDetectionImageFilter.html.
 */
class iAEdgeDetectionFilters : public iAAlgorithm
{

public:
	iAEdgeDetectionFilters( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	/**
	 * Sets canny edge detection parameters.
	 * \param	v	The variance.
	 * \param	m	The maximum error.
	 * \param	u	The upper threshold.
	 * \param	l	Thelower threshold.
	 */
	void setCEDParameters( double v, double m, double u, double l )
		{ variance = v; maximumError = m; upper = u; lower = l; };

protected:
	virtual void run();
private:
	double variance, maximumError, upper, lower;
	void cannyEdgeDetection();
};
