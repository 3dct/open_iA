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
 
#ifndef IAPARTICLECHARACTERIZATION_H
#define IAPARTICLECHARACTERIZATION_H

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
 * A implementation of the computation of particle characterization
 * \remarks	Arikan, 10/08/2012. 
 */

class iAParticleCharacterization : public iAFilter
{
public:
	iAParticleCharacterization( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iAParticleCharacterization();

	/**
	 * Sets iAParticleCharacterization parameters.
	 * \param	param1		some value.
	 */

	void setCParameters(double param1) { 
		this->param1 = param1; 
	};

protected:
    void run();
	void compute_particletest( );

private:
	unsigned int param1;
};
#endif
