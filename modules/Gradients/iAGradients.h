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

struct HOAccGradientDerrivativeSettings
{
	unsigned int order, direction, orderOfAcc;
};

enum iAGradientType
{
	DERIVATIVE,
	GRADIENT_MAGNITUDE,
	HIGHER_ORDER_ACCURATE_DERIVATIVE,
};

/**
 * An implementation of itkDerivativeImageFilter and itkGradientMagnitudeImageFilter.
 * For itkDerivativeImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1DerivativeImageFilter.html
 * For itkGradientMagnitudeImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1GradientMagnitudeImageFilter.html
 */
class iAGradients : public iAAlgorithm
{
public:
	iAGradients( QString fn, iAGradientType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	void setDParameters(double o, double d) { order = o; direction = d; };
	void setHOAGDParameters(const HOAccGradientDerrivativeSettings * settings) { m_HOAGDSettings = *settings; };
protected:
	void run();
private:
	unsigned int order, direction;
	HOAccGradientDerrivativeSettings m_HOAGDSettings;
	iAGradientType m_type;
	void derivative();
	void hoa_derivative();
	void gradient_magnitude();
};
