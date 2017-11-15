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

void iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( vtkSmartPointer<vtkLookupTable> pLUT, double * lutRange, int numCols /*= 256 */ )
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToLab();
	ctf->AddRGBPoint( 0.0, 0.230, 0.299, 0.754 );
	ctf->AddRGBPoint( 0.5, 0.865, 0.865, 0.865 );
	ctf->AddRGBPoint( 1.0, 0.706, 0.016, 0.150 );
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

void iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, int numCols /*= 256 */ )
{
	double lutRange[2] = { rangeFrom, rangeTo };
	BuildPerceptuallyUniformLUT( pLUT, lutRange, numCols );
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