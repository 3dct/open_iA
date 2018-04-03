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
#pragma once

#include "dlg_function.h"
#include "open_iA_Core_export.h"

#include <QColor>

class open_iA_Core_API dlg_gaussian : public dlg_function
{
	static double PI;

	QColor color;

	int    selectedPoint;
	bool   active;
	double mean;
	double sigma;
	double multiplier;
	
public:
	dlg_gaussian(iADiagramFctWidget *chart, QColor &color, bool reset = true);

	int getType() { return GAUSSIAN; }

	// abstract functions
	void draw(QPainter &painter);
	void draw(QPainter &painter, QColor color, int lineWidth);
	void drawOnTop(QPainter&) {}
	
	int selectPoint(QMouseEvent *event, int *x = NULL);
	int getSelectedPoint() { return 0; }
	int addPoint(int, int) { return 0; }
	void addColorPoint(int, double, double, double) {}
	void removePoint(int) {}
	void moveSelectedPoint(int x, int y);
	void changeColor(QMouseEvent *) {}
	
	bool isColored() { return false; }
	bool isEndPoint(int) { return true; }
	bool isDeletable(int) { return false; }
	
	void reset();
	
	void mousePressEvent(QMouseEvent*) {}
	void mouseMoveEvent(QMouseEvent*)  {}
	void mouseReleaseEvent(QMouseEvent*) {}
	void mouseReleaseEventAfterNewPoint(QMouseEvent*) {}

	// additional public functions
	void setMean(double mean) { this->mean = mean; }
	void setSigma(double sigma) { this->sigma = sigma; }
	void setMultiplier(double multiplier) { this->multiplier = multiplier; }
	void setMean(int mean) { this->mean = v2dX(mean); }
	void setSigma(int sigma) { this->sigma = i2dX(sigma)-i2dX(0); }
	void setMultiplier(int multiplier);

	double getMean() { return this->mean; }
	double getSigma() { return this->sigma; }
	double getCovariance() { return this->sigma*this->sigma; }
	double getMultiplier() { return this->multiplier; }

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
};
