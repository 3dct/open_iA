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

#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itkBilateralImageFilter.h>

#include "iAAlgorithm.h"

/**
 * Application of two edge preserving smoothing methods (itkGradientAnisotropicDiffusionImageFilter and itkCurvatureAnisotropicDiffusionImageFilter).
 * For further reference please look at http://www.itk.org/Doxygen/html/classitk_1_1GradientAnisotropicDiffusionImageFilter.html, 
 * http://www.itk.org/Doxygen/html/classitk_1_1CurvatureAnisotropicDiffusionImageFilter.html,
 * http://www.itk.org/Doxygen/html/classitk_1_1BilateralImageFilter.html
 *  
 */

class iAEdgePreservingSmoothing : public iAAlgorithm
{
public:
	iAEdgePreservingSmoothing( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iAEdgePreservingSmoothing( );

	void gradientAnisotropicDiffusion(  );
	void curvatureAnisotropicDiffusion(  );
	void bilateral( );

	/**
	 * Sets smoothing parameters. 
	 * \param	i		NumberOfIterations. 
	 * \param	t		TimeStep. 
	 * \param	c		ConductanceParameter. 
	 */
	void setADParameters(unsigned int i, double t, double c) { iterations = i; timestep = t; conductance = c;};
	
	/**
	 * Sets smoothing parameters. 
	 * \param	r		rangesigma. 
	 * \param	d		domainsigma. 
	 */
	void setBParameters( double r, double d ) { rangesigma = r; domainsigma = d; };


protected:
	void run();

private:
	unsigned int iterations; 
	double timestep, conductance, rangesigma, domainsigma;

};
