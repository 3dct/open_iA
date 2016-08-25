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
#include "itkGradientMagnitudeImageFilter.h"
#include "itkImage.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkHessianRecursiveGaussianImageFilter.h"
#include "itkImageToParametricSpaceFilter.h"
#include "itkMesh.h"
#include "itkImageFileReader.h"
#include "itkSphereSpatialFunction.h"
#include "itkInteriorExteriorMeshFilter.h"
#include "itkParametricSpaceToImageSpaceMeshFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkSymmetricSecondRankTensor.h"
#include "itkSymmetricEigenAnalysisImageFilter.h"
#include "itkImageAdaptor.h"
#include "itkCastImageFilter.h"
#include "itkPointSetToImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkJoinImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"
#include "iAPixelAccessors.h"
#include <string>
#include <iostream>
#include "itkImageRegionIterator.h"
#include <itkCastImageFilter.h>
#include <itkSmartPointer.h>


/**
 * A implementation of the computation of the hessian matrix and eigenanalysis.
 * \remarks	Arikan, 18/04/2012. 
 */

class iAHessianEigenanalysis : public iAFilter
{
public:
	iAHessianEigenanalysis( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iAHessianEigenanalysis();

	/**
	 * Sets iAHessianEigenanalysis parameters.
	 * \param	sigma		Sigma value.
	 * \param	hessian     Is hessian already computed.
	 * \param	eigen		Which eigenvalue should computed.
	 */

	void setCParameters(double sigma, bool hessian, int eigen) { 
		this->sigma = sigma; 
		hessianComputed = hessian;
		nr = eigen;
	};

	void setLapParameters(unsigned int sigma){
		this->sigma = sigma; 
	}; 

protected:
	void run();
	
	/** Computes hessian of given image */
	void computeHessian( );

	/** Computes laplacian of Gaussian (LoG) of given image. */
	void computeLaplacian(); 

private:
	unsigned int sigma;
	bool hessianComputed;
	int nr;
};
