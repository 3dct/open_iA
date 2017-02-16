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
#pragma once

#include <QPoint>
#include <QMouseEvent>
#include <QKeyEvent>

#include "vtkPolyData.h"

#include "iADiagramWidget.h"

// FORWARD DECLARATIONS
class vtkPolyData;
class vtkDataArray;

class QPaintEvent;
class QPainter;

class iAProfileWidget : public iADiagramWidget
{
	Q_OBJECT

public:
	static const int SELECTED_POINT_RADIUS = 8;
	static const int SELECTED_POINT_SIZE = 2*SELECTED_POINT_RADIUS;
	static const int POINT_RADIUS = 6;
	static const int POINT_SIZE = 2*POINT_RADIUS;
	static const int TEXT_Y = 15;
	static const int TEXT_X = 15;

	// Constructor/Destructor
	iAProfileWidget(QWidget *parent, vtkPolyData* profData, double rayLength, QString yCapt = "Y Axis", QString xCapt = "X Axis");
	void initialize(vtkPolyData* profData, double rayLength);

	//draw the histogram
	void drawProfilePlot();
	void redraw();

	int getMax() { return yHeight; }
protected:
	void paintEvent(QPaintEvent * );

private:
	vtkPolyData*       profileData;
	
	QPoint		lastpoint;
	QWidget		* activeChild;
	QPoint		contextPos;

	int xPos;
	double numBin, rayLen;
	double yDataRange[2];
	double min_intensity[3];
	double max_intensity[3];
	vtkDataArray * scalars;
	double yHeight;

	QString yCaption, xCaption;

	void drawHistogram(QPainter &painter);
	void drawAxes(QPainter &painter);
	void drawXAxis(QPainter &painter);
	void drawYAxis(QPainter &painter);
	void selectBin(QMouseEvent *event);

Q_SIGNALS:
	void binSelected(int newBin);
};
