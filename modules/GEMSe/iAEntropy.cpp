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
 
#include "iAEntropy.h"

#include "iAConsole.h"
#include "iAToolsVTK.h" // for AllocateImage
#include "iAVtkDraw.h"
#include "iAMathUtility.h"

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>

// TODO: use EntropyImageFilter instead!

vtkSmartPointer<vtkLookupTable> BuildEntropyCTF()
{
	const int NumberOfColors = 256;
	double rgbEmpty[3] = { 0.0, 0.0, 0.0 };
	double rgbFull[3] = { 1.0, 1.0, 1.0 };
	double hsvEmpty[3], hsvFull[3];
	vtkMath::RGBToHSV(rgbEmpty, hsvEmpty);
	vtkMath::RGBToHSV(rgbFull, hsvFull);

	vtkSmartPointer<vtkLookupTable> result = vtkSmartPointer<vtkLookupTable>::New();
	result->SetNumberOfTableValues(NumberOfColors);
	result->SetHueRange(hsvEmpty[0], hsvFull[0]);
	result->SetSaturationRange(hsvEmpty[1], hsvFull[1]);
	result->SetValueRange(hsvEmpty[2], hsvFull[2]);
	result->SetTableRange(0.0, 1.0);
	//result->SetAlphaRange(0.0, 1.0);
	result->SetRampToLinear();
	result->Build();
	return result;
}

vtkSmartPointer<vtkImageData> BuildEntropyImage(QVector<vtkSmartPointer<vtkImageData> > data)
{
	if (data.size() < 2)
	{
		DEBUG_LOG("At least two images required to calculate entropy image!\n");
		return vtkSmartPointer<vtkImageData>();
	}
	vtkSmartPointer<vtkImageData> entropyImage = AllocateImage(data[0]);
	int * dims = data[0]->GetDimensions();

	double limit = -std::log(1.0 / data.size());
	double normalizeFactor = 1 / limit;
	for (int x = 0; x<dims[0]; ++x)
	{
		for (int y = 0; y<dims[1]; ++y)
		{
			for (int z = 0; z<dims[2]; ++z)
			{
				double entropy = 0.0;
				double probSum = 0.0;
				for (int p = 0; p<data.size(); ++p)
				{
					vtkSmartPointer<vtkImageData> probImgP = data[p];
					double probP = probImgP->GetScalarComponentAsDouble(x, y, z, 0);
					probSum += probP;
					assert(probP >= 0 && probP <= 1);
					if (probP > 0)
					{
						entropy += (probP * std::log(probP));
					}
				}
				entropy = -entropy;
				assert(entropy >= -0.00001 && entropy <= limit + 0.00001);
				assert(probSum >= 0.99999 && probSum <= 1.00001);
				entropy = clamp(0.0, limit, entropy);
				drawPixel(entropyImage, x, y, z, entropy * normalizeFactor);
			}
		}
	}
	return entropyImage;
}