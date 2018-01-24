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
 
#include "pch.h"
#include "iAPerceptuallyUniformLUT.h"

#include <QColor>

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>

const QStringList colorMaps = QStringList()\
<< "Diverging blue-gray-red"\
<< "Black Body"\
<< "Extended Black Body"\
<< "Kindlmann";

void iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( vtkSmartPointer<vtkLookupTable> pLUT, 
	double * lutRange, int numCols /*= 256 */, QString colorMap /*= 0 */ )
{
	auto ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToLab();
	switch(colorMaps.indexOf(colorMap))
	{
	case 0:
		// Diverging blue-gray-red
		ctf->AddRGBPoint(0.0, 0.230, 0.299, 0.754);
		ctf->AddRGBPoint(0.5, 0.865, 0.865, 0.865);
		ctf->AddRGBPoint(1.0, 0.706, 0.016, 0.150);
		break;
	
	case 1:
		// http://www.kennethmoreland.com/color-advice/ Black Body
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.251720295995, 0.0884210421079, 0.0696046995283);
		ctf->AddRGBPoint(0.285714285714, 0.493237985431, 0.12134447042, 0.108818616983);
		ctf->AddRGBPoint(0.428571428571, 0.721654217074, 0.19689465803, 0.128274044848);
		ctf->AddRGBPoint(0.571428571429, 0.857243660245, 0.396604712155, 0.039717323074);
		ctf->AddRGBPoint(0.714285714286, 0.893886453749, 0.631451293491, 0.0130550121463);
		ctf->AddRGBPoint(0.857142857143, 0.913697911059, 0.845646813981, 0.222765707352);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;

	case 2:
		// http://www.kennethmoreland.com/color-advice/ Extended Black Body
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.169638222229, 0.0586571629275, 0.420432193125);
		ctf->AddRGBPoint(0.285714285714, 0.363946896841, 0.0, 0.784994035048);
		ctf->AddRGBPoint(0.428571428571, 0.774966161238, 0.0, 0.455769190891);
		ctf->AddRGBPoint(0.571428571429, 0.923364177686, 0.326317530516, 0.233758094872);
		ctf->AddRGBPoint(0.714285714286, 0.961153380556, 0.593535622168, 0.189790175914);
		ctf->AddRGBPoint(0.857142857143, 0.913697911059, 0.845646813981, 0.222765707352);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;

	case 3:
		// http://www.kennethmoreland.com/color-advice/ Kindlmann
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.2058836328, 0.0200081348995, 0.419451331093);
		ctf->AddRGBPoint(0.285714285714, 0.0335272327849, 0.22111999029, 0.703052629353);
		ctf->AddRGBPoint(0.428571428571, 0.021854471454, 0.451151641812, 0.380313980138);
		ctf->AddRGBPoint(0.571428571429, 0.0298263835637, 0.624285254063, 0.133059337268);
		ctf->AddRGBPoint(0.714285714286, 0.395075620074, 0.771599170072, 0.0373892404385);
		ctf->AddRGBPoint(0.857142857143, 0.976621102575, 0.813785197546, 0.509571446188);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;
	}
	pLUT->SetRange( lutRange );
	pLUT->SetTableRange( lutRange );
	pLUT->SetNumberOfColors( numCols );
	for( int i = 0; i < numCols; ++i )
	{
		double rgb[3];
		ctf->GetColor( (double)i / numCols, rgb );
		pLUT->SetTableValue( i, rgb[0], rgb[1], rgb[2] );
	}
	pLUT->Build();
}

void iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, int numCols /*= 256 */, QString colorMap /*= Diverging blue-gray-red */)
{
	double lutRange[2] = { rangeFrom, rangeTo };
	BuildPerceptuallyUniformLUT( pLUT, lutRange, numCols, colorMap );
}

void iAPerceptuallyUniformLUT::BuildLinearLUT(vtkSmartPointer<vtkLookupTable> pLUT, double* lutRange, int numCols /*= 256 */)
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToLab();
	//// ColorBrewer Single hue 5-class Oranges
	//QColor c(166, 54, 3);	 ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
	//c.setRgb(230, 85, 13);	 ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
	//c.setRgb(253, 141, 60);	 ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
	//c.setRgb(253, 190, 133); ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
	//c.setRgb(254, 237, 222); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());

	// ColorBrewer Single hue 5-class Grays
	QColor c(37, 37, 37);	 ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
	c.setRgb(99, 99, 99);	 ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
	c.setRgb(150, 150, 150); ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
	c.setRgb(204, 204, 204); ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
	c.setRgb(247, 247, 247); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());

	pLUT->SetRange(lutRange);
	pLUT->SetTableRange(lutRange);
	pLUT->SetNumberOfColors(numCols);
	for (int i = 0; i < numCols; ++i)
	{
		double rgb[3];
		ctf->GetColor((double)i / numCols, rgb);
		pLUT->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
	}
	pLUT->Build();
}

void iAPerceptuallyUniformLUT::BuildLinearLUT(vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, int numCols /*= 256 */)
{
	double lutRange[2] = { rangeFrom, rangeTo };
	BuildLinearLUT(pLUT, lutRange, numCols);
}