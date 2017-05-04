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

#include <QString>

/**
 * An itk blurring filter. The basic filter is itkDiscreteGaussianImageFilter.
 * For further details have look at http://www.itk.org/Doxygen/html/classitk_1_1DiscreteGaussianImageFilter.html#_details
 */
class iABlurring : public iAAlgorithm
{
public:
	iABlurring( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

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
	void discreteGaussian();
};
