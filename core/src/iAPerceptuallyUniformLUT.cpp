/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

