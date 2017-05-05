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
#include "iAFunctionalBoxplotQtDrawer.h"

#include "iAFunctionalBoxplot.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPoint>
#include <QPolygon>

#include <cmath>

namespace
{
	double ScaleXFactor = 4.0;
	double ScaleYFactor = 4.0;
	double PixelHeight = 500;
	int imgY(int fctA)
	{
		return std::floor(PixelHeight - (ScaleYFactor * (fctA)));
	}
	int imgX(double a)
	{
		return std::floor(ScaleXFactor * a);
	}
	QPoint img(double a, int fctA)
	{
		return QPoint(imgX(a), imgY(fctA));
	}
}
QSharedPointer<QImage> drawFunctionalBoxplot(FunctionalBoxPlot const * fbp, int width, int height)
{
	ScaleYFactor = PixelHeight / height;
	ScaleXFactor = std::min(32768 / width, static_cast<int>(ScaleXFactor));
	int PixelWidth = ScaleXFactor * width;
	assert (PixelWidth * PixelHeight < std::numeric_limits<int>::max());
	QSharedPointer<QImage> image(new QImage(PixelWidth, PixelHeight, QImage::Format_ARGB32));
	image->fill(QColor(0, 0, 0, 0));

	// reserve space in point list to avoid reallocation:
	QPolygon median;
	QPolygon centralRegion;
	QPolygon envelope;

		
	for (size_t a = 0; a<width; ++a)
	{
		centralRegion.append(img(a, fbp->getCentralRegion().getMin(a) ) );
		envelope.append(img(a, fbp->getEnvelope().getMin(a)));
		median.append(img(a + ((a<width-1)? 0.5 : 0), fbp->getMedian().get(a)));
		if (a<width-1)
		{
			centralRegion.append(img(a+1, fbp->getCentralRegion().getMin(a) ) );
			envelope.append(img(a+1, fbp->getEnvelope().getMin(a)));
		}
	}
	for (int a=width-1; a>=0; --a)
	{
		if (a<width-1)
		{
			centralRegion.append(img(a+1, fbp->getCentralRegion().getMax(a) ) );
			envelope.append(img(a+1, fbp->getEnvelope().getMax(a)));
		}
		centralRegion.append(img(a, fbp->getCentralRegion().getMax(a) ) );
		envelope.append(img(a, fbp->getEnvelope().getMax(a)));
	}
	QPainter painter(image.data());
	painter.setRenderHint(QPainter::Antialiasing);

	// a line for upper and lower envelope:
	QPainterPath envelopePath;
	envelopePath.addPolygon(envelope);
	painter.fillPath(envelopePath, QColor(73, 145, 188));

	// a filled polygon for the central region:
	QPainterPath centralPath;
	centralPath.addPolygon(centralRegion);
	painter.fillPath(centralPath, QColor(250, 194, 78));

	// a line for the median:
	QPen medianPen;
	medianPen.setStyle(Qt::SolidLine);
	medianPen.setColor(QColor(158,68,158));
	painter.setPen(medianPen);
	painter.drawPolyline(median);

	/*
	std::vector<iAFunction<size_t, unsigned int> *> const & outliers = fbp->getOutliers();
	for (size_t o=0; o<outliers.size(); ++o)
	{
		QPen outlierPen;
		outlierPen.setColor(QColor(241, 88, 84, 150));
		painter.setPen(outlierPen);
		QPolygon outlier;
		for (size_t a = 0; a<width; ++a)
		{
			if (!fbp->getCentralRegion().contains(a, outliers[o]->get(a)))
			{
				if (outlier.empty() && a > 0)
				{
					outlier.append(img(a - 0.5, outliers[o]->get(a-1)));
				}
				outlier.append(img(a + ((a<width-1)? 0.5 : 0), outliers[o]->get(a)));
			}
			else if (!outlier.empty())
			{
				outlier.append(img(a + ((a<width-1)? 0.5 : 0), outliers[o]->get(a)));
				if (a<width-1)
				{
					outlier.append(img(a+1, outliers[o]->get(a)));
				}
				painter.drawPolyline(outlier);
				outlier.clear();
			}
		}
		if (!outlier.empty())
		{
			painter.drawPolyline(outlier);
		}
	}
	*/
	return image;
}
