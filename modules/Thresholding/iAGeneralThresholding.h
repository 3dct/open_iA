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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAFilter.h"

/**
 * An itk general thresholding. Basic filter is itkThresholdImageFilter.
 * Application of general threshold using a range and a replacement value. 
 * For further details see http://www.itk.org/Doxygen/html/classitk_1_1ThresholdImageFilter.html
 * \remarks	CH, 01/12/2010. 
 */

class iAGeneralThresholding: public iAFilter
{
public:
	iAGeneralThresholding( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0);
	~iAGeneralThresholding();

	void thresholding (  );

	/**
	 * Sets a gt parameters. 
	 * \param	l		Lower threshold value. 
	 * \param	u		Upper threshold value. 
	 * \param	o		Value outside the range. 
	 */

	void setGTParameters( double l, double u, double o ) { lower = l; upper = u; outer = o; };

protected:
	void run();

private:
	double lower, upper, outer;
	
};
