// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAChartFunction.h"

#include "iacharts_export.h"

#include <QColor>

//! Class representing a Gaussian function in an iAChartWithFunctionsWidget.
//! Draws itself, and allows modifying its mean, sigma, "multiplier" (i.e. its height).
//! Can be used e.g. for fitting Gaussian curves to histogram peaks.
class iAcharts_API iAChartFunctionGaussian : public iAChartFunction
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
