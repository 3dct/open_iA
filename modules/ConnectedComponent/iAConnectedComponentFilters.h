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

#include <itkConnectedComponentImageFilter.h>
#include <itkScalarConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkImage.h>

#include "iAAlgorithm.h"
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

/**
 * Implementation of 2 itk filters. The basic filters are itkRelabelComponentImageFilter and itkConnectedComponentImageFilter.
 * For itkRelabelComponentImageFilter refer http://www.itk.org/Doxygen/html/classitk_1_1RelabelComponentImageFilter.html.
 * For itkConnectedComponentImageFilter refer http://www.itk.org/Doxygen/html/classitk_1_1ConnectedComponentImageFilter.html.
 * \remarks	Kana, 01/12/2010. 
 */

class iAConnectedComponentFilters : public iAAlgorithm
{
public:
	iAConnectedComponentFilters( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent = 0 );
	~iAConnectedComponentFilters( );

	void SimpleConnectedComponentFilter( );
	void ScalarConnectedComponentFilter( );
	void SimpleRelabelComponentImageFilter( );

	/**
	 * Sets a itkConnectedComponentImageFilter parameters. 
	 * \param	fullyconnected	The fullyconnected switch. 
	 */

	void setSCCFParameters( int fullyconnected ) { c = fullyconnected; };

	/**
	 * Sets a itkScalarConnectedComponentImageFilter parameters. 
	 * \param	fullyconnected	The fullyconnected switch. 
	 */

	void setScalarCCFParameters( double distanceThreshold ) { distTreshold = distanceThreshold; };

	/**
	 * Sets a itkRelabelComponentImageFilter parameters. 
	 * \param	writeroption	The switch writeroption. 
	 * \param	objectsize		The minimum objectsize. 
	 * \param	file			The filename. 
	 */

	void setSRCIFParameters( bool writeroption, int objectsize, QString file ) { w = writeroption; s = objectsize; f = file; };

protected:
	void run();

private:
	double distTreshold;
	int c;
	int s;
	bool w;
	QString f;

};
