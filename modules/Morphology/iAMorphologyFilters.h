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

#include "iAAlgorithm.h"

enum iAMorphologyOperationType
{
	DILATION_FILTER,
	EROSION_FILTER,
	VESSEL_ENHANCEMENT_FILTER,
};

/**
 * Implementation of erosion, dilation and binary thinning filters.
 */
class iAMorphologyFilters : public iAAlgorithm
{
public:
	iAMorphologyFilters( QString fn, iAMorphologyOperationType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	/**
	 * Sets a parameter for the morphology filters (e.g. radius of the structuring binary ball)
	 * \param	radius	The radius in int.
	 */
	void setMORPHParameters( int radius, vtkImageData* image) { r = radius; m_Image = image;};
protected:
	void run();
private:
	int r;
	vtkImageData* m_Image;
	iAMorphologyOperationType m_type;
	void DilationFilter();
	void ErosionFilter();
	void VesselEnhancementFilter();
};
