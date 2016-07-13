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

#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImage.h"

#include "iAFilter.h"

/**
 * An itk distance map. Basic filter itkSignedMaurerDistanceMapImageFilter.
 * Input image segmented binary image. Output image float datatype distance map.
 * Further details refer http://www.itk.org/Doxygen/html/classitk_1_1SignedMaurerDistanceMapImageFilter.html.
 * \remarks	Kana, 01/12/2010. 
 */

class iADistanceMap : public iAFilter
{
public:
	iADistanceMap( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iADistanceMap( );

	void signedmaurerdistancemap( );

	/**
	 * Sets a signedmaurerdistancemap parameters. 
	 * \param	i		The UseImageSpacingOn switch. 
	 * \param	t		The SquaredDistanceOff switch. 
	 * \param	c		The InsideIsPositiveOn switch. 
	 * \param	neg		The switch to set back ground = -1. 
	 */

	void setSMDMParameters( int i, int t, int c, int neg) { imagespacing = i; squareddistance = t; insidepositive = c; n = neg;};

protected:
	void run();

private:
	int imagespacing, insidepositive, squareddistance, n; 

};
