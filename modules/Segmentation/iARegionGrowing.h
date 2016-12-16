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

#include "itkOtsuThresholdImageFilter.h"
#include "itkOtsuMultipleThresholdsImageFilter.h"
#include "itkAdaptiveOtsuThresholdImageFilter.h"
#include "itkRobustAutomaticThresholdImageFilter.h"
#include "itkRemovePeaksOtsuThresholdImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "iAAlgorithms.h"

/**
 * Implementation of itkOtsuThresholdImageFilter, itkAdaptiveOtsuThresholdImageFilter and itkRobustAutomaticThresholdImageFilter threhold.
 * For itkOtsuThresholdImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1OtsuThresholdImageFilter.html
 * For itkAdaptiveOtsuThresholdImageFilter refer to FH Wels software team.
 * For itkRobustAutomaticThresholdImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1RobustAutomaticThresholdImageFilter.html
 * \remarks	Kana, 01/12/2010. 
 */

class iARegionGrowing : public iAAlgorithms
{
public:
	iARegionGrowing( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* l, QObject *parent = 0 );
	~iARegionGrowing( );

	void otsuMultipleThresh();
	void otsuThresh(  );
	void adaptiveOtsuThresh( );
	void ratsThresh(  );

	/**
	 * Sets an otsu multiple parameters. 
	 * \param	b					SetNumberOfHistogramBins. 
	 * \param	t					SetNumberOfThresholds. 
	 */

	void setOMTParameters( double b, double t ) 
					{ bins = b; threshs = t; };

	/**
	 * Sets an otsu parameters. 
	 * \param	b					SetNumberOfHistogramBins. 
	 * \param	o					SetOutsideValue. 
	 * \param	i					SetInsideValue. 
	 * \param	removepeaks			true to removepeaks. 
	 */

	void setOTParameters( double b, double o, double i, bool r ) 
					{ bins = b; outer = o; inner = i; removepeaks = r; };
	/**
	 * Sets a Adaptive otsu parameters. 
	 * \param	r		SetRadius. 
	 * \param	s		SetNumberOfSamples. 
	 * \param	l		SetNumberOfLevels. 
	 * \param	c		SetNumberOfControlPoints. 
	 * \param	b		SetNumberOfHistogramBins. 
	 * \param	o		SetOutsideValue. 
	 * \param	i		SetInsideValue. 
	 */

	void setAOTParameters( double r, unsigned int s, unsigned int l, unsigned int c, double b, double o, double i ) 
					{ radius = r; samples = s; levels = l; controlPoints = c; bins = b; outer = o; inner = i; };
	/**
	 * Sets a RAT parameters. 
	 * \param	pow					SetPower. 
	 * \param	o					SetOutsideValue. 
	 * \param	i					SetInsideValue. 
	 */

	void setRTParameters( double p, double o, double i ) 
					{ power = p; outer = o; inner = i; };

protected:
	void run();

private:
	double threshs, bins, inner, outer, radius, power, rthresh, othresh;
	bool removepeaks;
	unsigned int controlPoints, levels, samples;
	std::vector<double> omthreshs;

};
