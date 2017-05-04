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

enum iAWatershedType
{
	WATERSHED,
	MORPH_WATERSHED,
};

/**
 * Implementation of itkWatershedImageFilter. For itkWatershedImageFilter refer to 
 * http://www.itk.org/Doxygen/html/classitk_1_1WatershedImageFilter.html
 */
class iAWatershedSegmentation : public iAAlgorithm
{
public:
	iAWatershedSegmentation( QString fn, iAWatershedType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );

	void setWParameters( double l, double t ) { level = l; threshold = t; };
	void setMWSParameters( double mwsl, bool mwsmwsl, bool mwsFC )
	{
		mwsLevel = mwsl; mwsMarkWSLines = mwsmwsl; mwsFullyConnected = mwsFC;
	}

	vtkImageData* getImageDataNew ( ) { return imageDataNew; }
protected:
	virtual void run();
private:
	double level, threshold;
	double mwsLevel; // Morphological Watershed Segmentation Filter
	bool mwsMarkWSLines, mwsFullyConnected;	// Morphological Watershed Segmentation Filter
	iAWatershedType m_type;
	vtkImageData* imageDataNew;
	void watershed();
	void morph_watershed();
};
