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
#pragma once

#include "iASegmentation.h"


#ifdef __GNUC__
#include <inttypes.h>
typedef int64_t __int64;
#else
/* Borland and Microsoft compilers */
#endif

/**
 * Implementation of itkWatershedImageFilter. For itkWatershedImageFilter refer to 
 * http://www.itk.org/Doxygen/html/classitk_1_1WatershedImageFilter.html
 */

class iAWatershedSegmentation : public iASegmentation
{

public:
	iAWatershedSegmentation( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	virtual ~iAWatershedSegmentation();
	void watershed();
	void morph_watershed();

	void setWParameters( double l, double t ) { level = l; threshold = t; };
	void setMWSParameters( QString f, double l, bool mwsmwsl, bool mwsFC, bool mwsSRGBI ){ mwsRGBFilePath = f; mwsLevel = l;
	mwsMarkWSLines = mwsmwsl; mwsFullyConnected = mwsFC; mwsSaveRGBImage = mwsSRGBI; };

	vtkImageData* getImageDataNew ( ) { return imageDataNew; }

protected:
	virtual void run();

private:
	double level, threshold;
	double mwsLevel; // Morphological Watershed Segmentation Filter
	bool mwsMarkWSLines, mwsFullyConnected, mwsSaveRGBImage;	// Morphological Watershed Segmentation Filter
	QString mwsRGBFilePath; // Morphological Watershed Segmentation Filter

	vtkImageData* imageDataNew;
};
