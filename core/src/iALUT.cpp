/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
 
#include "iALUT.h"

#include "iALookupTable.h"

#include <QColor>

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>

const QStringList colormaps = QStringList()
	<< "Diverging blue-gray-red"
	<< "Diverging red-gray-blue"
	<< "Black Body"
	<< "Extended Black Body"
	<< "Kindlmann"
	<< "Kindlmann Extended"
	<< "ColorBrewer single hue 5-class oranges"
	<< "ColorBrewer single hue 5-class grays"
	<< "ColorBrewer single hue 5-class oranges inv"
	<< "ColorBrewer single hue 5-class red inv"
	<< "ColorBrewer qualitative 12-class Set3"
	<< "ColorBrewer qualitative 9-class Set1"
	<< "ColorBrewer qualitative 12-class Paired";

const QStringList& iALUT::GetColorMapNames()
{
	return colormaps;
}

int iALUT::BuildLUT( vtkSmartPointer<vtkLookupTable> pLUT, double * lutRange, QString colorMap, int numCols /*= 256 */)
{
	auto ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToLab();
	QColor c;
	switch(colormaps.indexOf(colorMap))
	{
	case 0:
		// Diverging blue-gray-red
		ctf->AddRGBPoint(0.0, 0.230, 0.299, 0.754);
		ctf->AddRGBPoint(0.5, 0.865, 0.865, 0.865);
		ctf->AddRGBPoint(1.0, 0.706, 0.016, 0.150);
		break;

	case 1:
		// Diverging red-gray-blue
		ctf->AddRGBPoint(0.0, 0.706, 0.016, 0.150);
		ctf->AddRGBPoint(0.5, 0.865, 0.865, 0.865);
		ctf->AddRGBPoint(1.0, 0.230, 0.299, 0.754);
		break;
	
	case 2:
		// Black Body http://www.kennethmoreland.com/color-advice/ 
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.251720295995, 0.0884210421079, 0.0696046995283);
		ctf->AddRGBPoint(0.285714285714, 0.493237985431, 0.12134447042, 0.108818616983);
		ctf->AddRGBPoint(0.428571428571, 0.721654217074, 0.19689465803, 0.128274044848);
		ctf->AddRGBPoint(0.571428571429, 0.857243660245, 0.396604712155, 0.039717323074);
		ctf->AddRGBPoint(0.714285714286, 0.893886453749, 0.631451293491, 0.0130550121463);
		ctf->AddRGBPoint(0.857142857143, 0.913697911059, 0.845646813981, 0.222765707352);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;

	case 3:
		// Extended Black Body http://www.kennethmoreland.com/color-advice/ 
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.169638222229, 0.0586571629275, 0.420432193125);
		ctf->AddRGBPoint(0.285714285714, 0.363946896841, 0.0, 0.784994035048);
		ctf->AddRGBPoint(0.428571428571, 0.774966161238, 0.0, 0.455769190891);
		ctf->AddRGBPoint(0.571428571429, 0.923364177686, 0.326317530516, 0.233758094872);
		ctf->AddRGBPoint(0.714285714286, 0.961153380556, 0.593535622168, 0.189790175914);
		ctf->AddRGBPoint(0.857142857143, 0.913697911059, 0.845646813981, 0.222765707352);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;

	case 4:
		// Kindlmann http://www.kennethmoreland.com/color-advice/ 
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.2058836328, 0.0200081348995, 0.419451331093);
		ctf->AddRGBPoint(0.285714285714, 0.0335272327849, 0.22111999029, 0.703052629353);
		ctf->AddRGBPoint(0.428571428571, 0.021854471454, 0.451151641812, 0.380313980138);
		ctf->AddRGBPoint(0.571428571429, 0.0298263835637, 0.624285254063, 0.133059337268);
		ctf->AddRGBPoint(0.714285714286, 0.395075620074, 0.771599170072, 0.0373892404385);
		ctf->AddRGBPoint(0.857142857143, 0.976621102575, 0.813785197546, 0.509571446188);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;

	case 5:
		// Kindlmann Extended http://www.kennethmoreland.com/color-advice/ 
		ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
		ctf->AddRGBPoint(0.142857142857, 0.165244050347, 0.0224832200874, 0.47021606479);
		ctf->AddRGBPoint(0.285714285714, 0.0147354562034, 0.305005386829, 0.208719146502);
		ctf->AddRGBPoint(0.428571428571, 0.154492186368, 0.456128474882, 0.0218404104551);
		ctf->AddRGBPoint(0.571428571429, 0.866948968961, 0.388363362964, 0.0413685026023);
		ctf->AddRGBPoint(0.714285714286, 0.978023338129, 0.539236804307, 0.792434027154);
		ctf->AddRGBPoint(0.857142857143, 0.898924549826, 0.805934065795, 0.990584977084);
		ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
		break;

	case 6:
		// ColorBrewer single hue 5-class oranges (from orange to neutral)
		c.setRgb(166, 54, 3);	 ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(230, 85, 13);	 ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
		c.setRgb(253, 141, 60);	 ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
		c.setRgb(253, 190, 133); ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
		c.setRgb(254, 237, 222); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
		break;

	case 7:
		// ColorBrewer single hue 5-class grays
		c.setRgb(37, 37, 37);	 ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(99, 99, 99);	 ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
		c.setRgb(150, 150, 150); ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
		c.setRgb(204, 204, 204); ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
		c.setRgb(247, 247, 247); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
		break;

	case 8:
		// ColorBrewer single hue 5-class oranges (from neutral to orange)
		c.setRgb(254, 237, 222); ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(253, 190, 133); ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
		c.setRgb(253, 141, 60);	 ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
		c.setRgb(230, 85, 13);	 ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
		c.setRgb(166, 54, 3);	 ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
		break;

	case 9:
		// ColorBrewer single hue 5-class reds (from neutral to red)
		c.setRgb(254, 229, 217); ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(252, 174, 145); ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
		c.setRgb(251, 106, 74);	 ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
		c.setRgb(222, 45, 38);	 ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
		c.setRgb(165, 15, 21);	 ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
		break;

	case 10:
		// ColorBrewer qualitative 12-class Set3
		c.setRgb(141, 211, 199); ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(255, 255, 179); ctf->AddRGBPoint(0.09, c.redF(), c.greenF(), c.blueF());
		c.setRgb(190, 186, 218); ctf->AddRGBPoint(0.18, c.redF(), c.greenF(), c.blueF());
		c.setRgb(251, 128, 114); ctf->AddRGBPoint(0.27, c.redF(), c.greenF(), c.blueF());
		c.setRgb(128, 177, 211); ctf->AddRGBPoint(0.36, c.redF(), c.greenF(), c.blueF());
		c.setRgb(253, 180, 98);  ctf->AddRGBPoint(0.45, c.redF(), c.greenF(), c.blueF());
		c.setRgb(179, 222, 105); ctf->AddRGBPoint(0.54, c.redF(), c.greenF(), c.blueF());
		c.setRgb(252, 205, 229); ctf->AddRGBPoint(0.63, c.redF(), c.greenF(), c.blueF());
		c.setRgb(217, 217, 217); ctf->AddRGBPoint(0.72, c.redF(), c.greenF(), c.blueF());
		c.setRgb(188, 128, 189); ctf->AddRGBPoint(0.81, c.redF(), c.greenF(), c.blueF());
		c.setRgb(204, 235, 197); ctf->AddRGBPoint(0.90, c.redF(), c.greenF(), c.blueF());
		c.setRgb(255, 237, 111); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
		break;

	case 11:
		// ColorBrewer qualitative 9-class Set1
		c.setRgb(228, 26, 28);   ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(55, 126, 184);  ctf->AddRGBPoint(0.125, c.redF(), c.greenF(), c.blueF());
		c.setRgb(77, 175, 74);   ctf->AddRGBPoint(0.25, c.redF(), c.greenF(), c.blueF());
		c.setRgb(152, 78, 163);  ctf->AddRGBPoint(0.375, c.redF(), c.greenF(), c.blueF());
		c.setRgb(255, 127, 0);   ctf->AddRGBPoint(0.5, c.redF(), c.greenF(), c.blueF());
		c.setRgb(255, 255, 51);  ctf->AddRGBPoint(0.625, c.redF(), c.greenF(), c.blueF());
		c.setRgb(166, 86, 40);   ctf->AddRGBPoint(0.75, c.redF(), c.greenF(), c.blueF());
		c.setRgb(247, 129, 191); ctf->AddRGBPoint(0.875, c.redF(), c.greenF(), c.blueF());
		c.setRgb(153, 153, 153); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
		break;

	case 12:
		// ColorBrewer qualitative 12-class Paired
		c.setRgb(166, 206, 227); ctf->AddRGBPoint(0.0, c.redF(), c.greenF(), c.blueF());
		c.setRgb(31, 120, 180); ctf->AddRGBPoint(0.09, c.redF(), c.greenF(), c.blueF());
		c.setRgb(178, 223, 138); ctf->AddRGBPoint(0.18, c.redF(), c.greenF(), c.blueF());
		c.setRgb(51, 160, 44); ctf->AddRGBPoint(0.27, c.redF(), c.greenF(), c.blueF());
		c.setRgb(251, 154, 153); ctf->AddRGBPoint(0.36, c.redF(), c.greenF(), c.blueF());
		c.setRgb(227, 26, 28);  ctf->AddRGBPoint(0.45, c.redF(), c.greenF(), c.blueF());
		c.setRgb(253, 191, 111); ctf->AddRGBPoint(0.54, c.redF(), c.greenF(), c.blueF());
		c.setRgb(255, 127, 0); ctf->AddRGBPoint(0.63, c.redF(), c.greenF(), c.blueF());
		c.setRgb(202, 178, 214); ctf->AddRGBPoint(0.72, c.redF(), c.greenF(), c.blueF());
		c.setRgb(106, 61, 154); ctf->AddRGBPoint(0.81, c.redF(), c.greenF(), c.blueF());
		c.setRgb(255, 255, 153); ctf->AddRGBPoint(0.90, c.redF(), c.greenF(), c.blueF());
		c.setRgb(177, 89, 40); ctf->AddRGBPoint(1.0, c.redF(), c.greenF(), c.blueF());
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
	return ctf->GetSize();
}

int iALUT::BuildLUT( vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, QString colorMap, int numCols /*= 256 */)
{
	double lutRange[2] = { rangeFrom, rangeTo };
	return BuildLUT( pLUT, lutRange, colorMap, numCols);
}

iALookupTable open_iA_Core_API iALUT::Build(double * lutRange, QString colorMap, int numCols, double alpha)
{
	vtkSmartPointer<vtkLookupTable> vtkLUT(vtkSmartPointer<vtkLookupTable>::New());
	BuildLUT(vtkLUT, lutRange, colorMap, numCols);
	iALookupTable result(vtkLUT);
	return result;
}
