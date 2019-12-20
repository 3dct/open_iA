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
#pragma once

#include "iAChartFunction.h"
#include "open_iA_Core_export.h"

#include <QColor>

class open_iA_Core_API iAChartFunctionGaussian : public iAChartFunction
{
public:
	iAChartFunctionGaussian(iADiagramFctWidget *chart, QColor &color, bool reset = true);

	int getType() override { return GAUSSIAN; }
	void draw(QPainter &painter) override;
	void draw(QPainter &painter, QColor color, int lineWidth) override;
	void drawOnTop(QPainter&) override {}
	int selectPoint(QMouseEvent *event, int *x = nullptr) override;
	int getSelectedPoint() override { return 0; }
	int addPoint(int, int) override { return 0; }
	void addColorPoint(int, double, double, double) override {}
	void removePoint(int) override {}
	void moveSelectedPoint(int x, int y) override;
	void changeColor(QMouseEvent *) override {}
	bool isColored() override { return false; }
	bool isEndPoint(int) override { return true; }
	bool isDeletable(int) override { return false; }
	void reset() override;
	size_t numPoints() const override;

	// additional public functions
	void setMean(double mean)   { m_mean = mean; }
	void setMean(int mean)      { m_mean = v2dX(mean); }
	void setSigma(double sigma) { m_sigma = sigma; }
	void setSigma(int sigma)    { m_sigma = i2dX(sigma) - i2dX(0); }
	void setMultiplier(double multiplier) { m_multiplier = multiplier; }
	void setMultiplier(int multiplier);

	double getMean() { return m_mean; }
	double getSigma() { return m_sigma; }
	double getCovariance() { return m_sigma* m_sigma; }
	double getMultiplier() { return m_multiplier; }

private:

	// convert view to data
	double v2dX(int x);
	double v2dY(int y);

	// convert data to view
	int d2vX(double x);
	int d2vY(double y);

	// convert data to image
	int d2iX(double x);
	int d2iY(double y);

	// convert image to data
	double i2dX(int x);

	QColor m_color;
	int    m_selectedPoint;
	double m_mean;
	double m_sigma;
	double m_multiplier;
};
