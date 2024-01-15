// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAXRFOverlay.h"

#include "iAAccumulatedXRFData.h"

void initSpectraColormap(
	vtkSmartPointer<vtkColorTransferFunction> colormapLUT,
	iAAccumulatedXRFData const * accData,
	double val, double max,
	QImage const & spectraHistogramColormap)
{
	int colormapHeight = spectraHistogramColormap.height();
	double sensitivity = (max - val) / max;
	sensitivity *= sensitivity; //square scaling
	double scale = sensitivity * accData->yBounds()[1] / colormapHeight;
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

std::shared_ptr<QImage> CalculateSpectraHistogramImage(
	vtkSmartPointer<vtkColorTransferFunction> colormapLUT,
	iAAccumulatedXRFData * accData,
	QImage const & spectraHistogramColormap,
	long numBin,
	double sensVal, double sensMax, double threshVal, double threshMax,
	bool smoothFade)
{
	CountType * histData = 0;
	CountType maxHistVal;
	size_t numHist;
	accData->retrieveHistData(numBin, histData, numHist, maxHistVal);
	initSpectraColormap(colormapLUT, accData, sensVal, sensMax, spectraHistogramColormap);

	double opacThreshold = 0.1 * threshVal / threshMax;
	int width = std::abs(static_cast<int>(numHist));
	auto result = std::make_shared<QImage>(width, numBin, QImage::Format_ARGB32);
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
			result->setPixel( std::abs(static_cast<int>(x)), inv_y, color.rgba());
		}
	}
	return result;
}
