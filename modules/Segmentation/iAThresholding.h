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

#include "iAFilter.h"

enum iAThresholdingType
{
	BINARY_THRESHOLD,
	OTSU_MULTIPLE_THRESHOLD,
	OTSU_THRESHOLD,
	ADAPTIVE_OTSU_THRESHOLD,
	RATS_THRESHOLD,
};

class iABinaryThreshold : public iAFilter
{
public:
	static QSharedPointer<iABinaryThreshold> Create();
	void Run(QMap<QString, QVariant> parameters) override;
private:
	iABinaryThreshold();
};

/**
 * Implementation of itkBinaryThresholdImageFilter, itkOtsuThresholdImageFilter, itkAdaptiveOtsuThresholdImageFilter and itkRobustAutomaticThresholdImageFilter threhold.
 * For itkBinaryThresholdImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1BinaryThresholdImageFilter.html
 * For itkOtsuThresholdImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1OtsuThresholdImageFilter.html
 * For itkAdaptiveOtsuThresholdImageFilter refer to FH Wels software team.
 * For itkRobustAutomaticThresholdImageFilter refer to http://www.itk.org/Doxygen/html/classitk_1_1RobustAutomaticThresholdImageFilter.html
 */
class iAThresholding : public iAAlgorithm
{
public:
	iAThresholding( QString fn, iAThresholdingType fid, vtkImageData* i, vtkPolyData* p, iALogger* l, QObject *parent = 0 );

	/**
	 * Sets otsu multiple parameters.
	 * \param	b					SetNumberOfHistogramBins. 
	 * \param	t					SetNumberOfThresholds. 
	 * \param	v					SetValleyEmphasis.
	 */
	void setOMTParameters( double b, double t, bool ve ) 
	{
		bins = b; threshs = t; valleyemphasis = ve;
	};

	/**
	 * Sets otsu parameters.
	 * \param	b					SetNumberOfHistogramBins. 
	 * \param	o					SetOutsideValue. 
	 * \param	i					SetInsideValue. 
	 * \param	removepeaks			true to removepeaks. 
	 */
	void setOTParameters( double b, double o, double i, bool r ) 
					{ bins = b; outer = o; inner = i; removepeaks = r; };
	/**
	 * Sets Adaptive otsu parameters.
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
	 * Sets RAT parameters.
	 * \param	pow					SetPower. 
	 * \param	o					SetOutsideValue. 
	 * \param	i					SetInsideValue. 
	 */
	void setRTParameters( double p, double o, double i ) 
					{ power = p; outer = o; inner = i; };

protected:
	virtual void performWork();
private:
	double threshs, bins, inner, outer, radius, power, rthresh, othresh;
	bool valleyemphasis;
	bool removepeaks;
	unsigned int controlPoints, levels, samples;
	std::vector<double> omthreshs;
	iAThresholdingType m_type;

	void otsuMultipleThresh();
	void otsuThresh();
	void adaptiveOtsuThresh();
	void ratsThresh();
};
