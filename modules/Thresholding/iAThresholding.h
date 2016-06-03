/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IATHRESHOLDING_H
#define IATHRESHOLDING_H

#pragma once

#include "iAFilter.h"

/**
 * Basic filter is itkBinaryThresholdImageFilter. 
 * For itkBinaryThresholdImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1BinaryThresholdImageFilter.html
 */

class iAThresholding: public iAFilter
{
public:
	iAThresholding( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0);
	~iAThresholding();

	void binaryThresh (  );

	/**
	 * Sets a bt parameters. 
	 * \param	l		Lower Threshold
	 * \param	u		Upper Threshold
	 * \param	o		Outside Value
	 * \param	i		Inside Value
	 */

	void setBTParameters( double l, double u, double o, double i ) { lower = l; upper = u; outer = o; inner = i; };

protected:
	void run();

private:
	double lower, upper, outer, inner;
	
};
#endif
