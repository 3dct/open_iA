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
#include "iAXRFOverlay.h"

#include "iAAccumulatedXRFData.h"

void initSpectraColormap(
	vtkSmartPointer<vtkColorTransferFunction> colormapLUT,
	QSharedPointer<iAAccumulatedXRFData> accData,
	double val, double max,
	QImage const & spectraHistogramColormap)
{
	int colormapHeight = spectraHistogramColormap.height();
	double sensitivity = (max - val) / max;
	sensitivity *= sensitivity; //square scaling
	double scale = sensitivity * accData->YBounds()[1] / colormapHeight;
	colormapLUT->RemoveAllPoints();
	for (int y=0; y<colormapHeight; ++y)
	{
		int inv_y = colormapHeight - y - 1;
		QColor color( spectraHistogramColormap.pixel(0, inv_y) );
		double c = 1.0 / 255.0; double rgb[3] = {c*color.red(), c*color.green(), c*color.blue()};
		colormapLUT->AddRGBPoint( scale*y, rgb[0], rgb[1], rgb[2] );
	}
	colormapLUT->Modified();
}

QSharedPointer<QImage> CalculateSpectraHistogramImage(
	vtkSmartPointer<vtkColorTransferFunction> colormapLUT,
	QSharedPointer<iAAccumulatedXRFData> accData,
	QImage const & spectraHistogramColormap,
	long numBin,
	double sensVal, double sensMax, double threshVal, double threshMax,
	bool smoothFade)
{
	CountType * histData = 0;
	CountType maxHistVal;
	size_t numHist;	
	accData->RetrieveHistData(numBin, histData, numHist, maxHistVal);
	initSpectraColormap(colormapLUT, accData, sensVal, sensMax, spectraHistogramColormap);

	double opacThreshold = 0.1 * threshVal / threshMax;
	int width = abs(static_cast<int>(numHist));
	QSharedPointer<QImage> result = QSharedPointer<QImage>( new QImage(width, numBin, QImage::Format_ARGB32) );
	for (size_t x = 0; x < numHist; ++x)
	{
		for(int y = 0; y < numBin; ++y)
		{
			int inv_y = numBin - y - 1;
			double val = (double)histData[y + x*numBin];
			double normVal = val / maxHistVal;
			double *rgb = colormapLUT->GetColor(val);
			QColor color(255.0*rgb[0], 255.0*rgb[1], 255.0*rgb[2]);
			if( normVal < opacThreshold )
				smoothFade ? color.setAlpha( 255.0 * normVal / opacThreshold ) : color.setAlpha( 0.0 );
			result->setPixel( abs(static_cast<int>(x)), inv_y, color.rgba());
		}
	}
	return result;
}