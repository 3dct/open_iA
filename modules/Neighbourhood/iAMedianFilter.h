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

#include "iAAlgorithm.h"
#include "itkGradientMagnitudeImageFilter.h"
#include <itkCastImageFilter.h>

/**
 * An implementation of itkMedianImageFilter.
 * For itkMedianImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1MedianImageFilter.html
 *  * \remarks	JW, 10/10/2012. 
 */

class iAMedianFilter : public iAAlgorithm
{
public:
	iAMedianFilter( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iAMedianFilter();

	/**
	 * Sets iAMedianFilter parameters. 
	 * \param	r_x		radius along x. 
	 * \param	r_y		radius along y.
	 * \param	r_z		radius along z.
	 */

	void setDParameters(unsigned int r_x, unsigned int r_y, unsigned int r_z) { iRx = r_x; iRy = r_y; iRz = r_z; };

protected:
	void run();
	void median( );

private:
	unsigned int iRx, iRy, iRz; 

};
