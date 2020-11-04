/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

//! Class representing a Gaussian function in an iAChartWithFunctionsWidget.
//! Draws itself, and allows modifying its mean, sigma, "multiplier" (i.e. its height).
//! Can be used e.g. for fitting Gaussian curves to histogram peaks.
class open_iA_Core_API iAChartFunctionGaussian : public iAChartFunction
{
public:
	iAChartFunctionGaussian(iAChartWithFunctionsWidget *chart, QColor &color, bool reset = true);

	void draw(QPainter &painter) override;
	void draw(QPainter &painter, QColor color, int lineWidth) override;
	void drawOnTop(QPainter&) override {}
	int selectPoint(int mouseX, int mouseY) override;
	int getSelectedPoint() const override { return 0; }
	int addPoint(int, int) override { return 0; }
	void addColorPoint(int, double, double, double) override {}
	void removePoint(int) override {}
	void moveSelectedPoint(int x, int y) override;
	bool isEndPoint(int) const override { return true; }
	bool isDeletable(int) const override { return false; }
	void reset() override;
	virtual QString name() const override;
	size_t numPoints() const override;

	void setMean(double mean)   { m_mean = mean; }
	void setSigma(double sigma) { m_sigma = sigma; }
	void setMultiplier(double multiplier) { m_multiplier = multiplier; }

	double getMean()       { return m_mean; }
	double getSigma()      { return m_sigma; }
	double getCovariance() { return m_sigma * m_sigma; }
	double getMultiplier() { return m_multiplier; }

private:
	QColor m_color;
	int    m_selectedPoint;
	double m_mean;
	double m_sigma;
	double m_multiplier;
};
