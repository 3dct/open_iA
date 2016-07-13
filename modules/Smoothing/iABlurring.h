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
#pragma once

#include "iAFilter.h"

#include <itkCastImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>

#include <QString>

/**
 * An itk blurring filter. The basic filter is itkDiscreteGaussianImageFilter.
 * For further details have look at http://www.itk.org/Doxygen/html/classitk_1_1DiscreteGaussianImageFilter.html#_details
 * \remarks	Kana, 01/12/2010. 
 */

class iABlurring : public iAFilter
{
public:
	iABlurring( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iABlurring( );

	void discreteGaussian(  );

	/**
	 * Sets a itkDiscreteGaussianImageFilter parameters. 
	 * \param	v	Variance. 
	 * \param	me	maximum error. 
	 */

	void setDGParameters(double v, double me, int out) { variance = v; maximumError = me; outimg = out; };

protected:
	void run();

private:
	double variance, maximumError;
	int outimg;

};
