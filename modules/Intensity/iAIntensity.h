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
 * Implementation of DifferenceImageFilter and InvertIntensityImageFilter.
 * For DifferenceImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1DifferenceImageFilter.html
 * For InvertIntensityImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1InvertIntensityImageFilter.html
 * \remarks	Kana, 01/12/2010. 
 */

class iAIntensity : public iAFilter
{
public:
	iAIntensity( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iAIntensity();

	/**
	 * Sets DifferenceImageFilter parameters. 
	 * \param	dt	The difference threshold. 
	 * \param	tr	The tolerance radius. 
	 * \param	i2	The second vtkImageData*. 
	 */

	void setDIFParameters(double dt, double tr, vtkImageData* i2) 
	{ 
		DifferenceThreshold = dt; 
		ToleranceRadius = tr;
		image2 = i2;
	};

	/**
	* Sets Intensity_WindowingImageFilter parameters.
	* \param	wmin	The window minimum.
	* \param	wmax	The window maximum.
	* \param	omin	The output minimum.
	* \param	omax	The output maximum.
	*/
	
	void setWIIFParameters( double wmin, double wmax, double omin, double omax )
	{
		windowMinimum = wmin;
		windowMaximum = wmax;
		outputMinimum = omin;
		outputMaximum = omax;
	};

protected:
	void run();
	void difference(  );
	void invert_intensity( );
	void mask();
	void intensity_windowing();

private:
	double DifferenceThreshold, ToleranceRadius, windowMinimum, windowMaximum, outputMinimum, outputMaximum;
	vtkImageData* image2;
};
