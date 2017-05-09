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

enum iAIntensityFilterType
{
	DIFFERENCE_IMAGE,
	INVERT_INTENSITY,
	MASK_IMAGE,
	INTENSITY_WINDOWING,
	NORMALIZE_IMAGE,
	HISTOGRAM_MATCH,
	RESCALE_IMAGE,
};

/**
 * Implementation of DifferenceImageFilter and InvertIntensityImageFilter.
 * For DifferenceImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1DifferenceImageFilter.html
 * For InvertIntensityImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1InvertIntensityImageFilter.html
 * For Rescale Image filter refer to http://itk.org/ITKExamples/src/Filtering/ImageIntensity/RescaleAnImage/Documentation.html
 */
class iAIntensity : public iAAlgorithm
{
public:
	iAIntensity( QString fn, iAIntensityFilterType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	void setDIFParameters(double dt, double tr, vtkImageData* i2) 
	{ 
		DifferenceThreshold = dt; 
		ToleranceRadius = tr;
		image2 = i2;
	}

	void setWIIFParameters( double wmin, double wmax, double omin, double omax )
	{
		windowMinimum = wmin;
		windowMaximum = wmax;
		outputMinimum = omin;
		outputMaximum = omax;
	}

	void setHMFParameters( int hl, int mp, bool tami, vtkImageData* i2 )
	{
		histogramLevels = hl;
		matchPoints = mp;
		thresholdAtMeanIntensity = tami;
		image2 = i2;
	}

	void setRescaleParameters(double outMin, double outMax)
	{
		outputMin = outMin;
		outputMax = outMax;
	}

protected:
	void run();

private:
	double DifferenceThreshold, ToleranceRadius, windowMinimum, windowMaximum, outputMinimum, outputMaximum;
	int histogramLevels, matchPoints;	// Histogram Match Filter
	bool thresholdAtMeanIntensity;		// Histogram Match Filter
	vtkImageData* image2;
	iAIntensityFilterType m_type;
	double outputMin, outputMax;		// rescale intensity filter

	void difference();
	void invert_intensity();
	void mask();
	void intensity_windowing();
	void normalize();
	void histomatch();
	void rescaleImage();
};
