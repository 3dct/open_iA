/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <QWidget>
#include <QPaintEvent>
#include <vector>

class QPainter;

const double MAX_ZOOM = 1000;
const double MIN_ZOOM = 1;
const int OFFSET_FROM_TOP = 10;
class dlg_histogram_simple : public QWidget//, private Ui_Histogram
{
	//Q_OBJECT
public:
	enum Mode { NO_MODE, MOVE_VIEW_MODE, ZOOM_MODE, HIST_ZOOM_MODE };
	enum WheelMode { ZOOM_WHEEL_MODE, HIST_ZOOM_WHEEL_MODE };
	static const int SELECTED_POINT_RADIUS = 8;
	static const int SELECTED_POINT_SIZE = 2*SELECTED_POINT_RADIUS;
	static const int POINT_RADIUS = 6;
	static const int POINT_SIZE = 2*POINT_RADIUS;
	static double    ZOOM_STEP;
	static double    HIST_ZOOM_STEP;

	dlg_histogram_simple(QWidget *parent);
	void initialize(unsigned int* histData, int numberOfBeans, double _dataRange[2]);

	//! Returns if volume is updated automatically after changing the transfer function
	bool isUpdateAutomatically();
	void drawHistogram();
	void redraw();

protected:
	void paintEvent(QPaintEvent * ) override;
	void resizeEvent (QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;

private:
	QImage        image;
	QWidget       *activeChild;

	Mode mode;
	WheelMode wheelMode;

	bool draw;
	bool updateAutomatically;
	int width, height, numBin, bottomMargin, xPos;
	int selectedPoint;
	int dragStartPosX;
	int zoomX;
	int zoomY;
	double dataRange[2], oldDataRange[2];
	double accSpacing;
	double translation;
	double tmpTranslation;
	double zoom;
	double tmpZoom;
	double histZoom;
	double tmpHistZoom;
	std::vector<unsigned int> histoPtr;
	unsigned int maxFreq;

	//! Converts the x value of the histogram view to transfer function data
	//! \param dataX double pointer to save data
	//! \param viewX X value of the histogram view
	void view2dataX(double *dataX, int viewX);

	//! Converts the y value of the histogram view to transfer function data
	//! \param dataY double pointer to save data
	//! \param viewY Y value of the histogram view
	void view2dataY(double *dataY, int viewY);

	//! Converts the x and y value of the histogram view to transfer function data
	//! \param dataX double pointer to save data
	//! \param dataY double pointer to save data
	//! \param viewX X value of the histogram view
	//! \param viewY Y value of the histogram view
	void view2data(double *dataX, double *dataY, int viewX, int viewY);

	//! Converts x value of transfer function data to histogram view
	//! \param viewX int pointer to save view data
	//! \param dataX X value of the transfer function data
	void data2viewX(int *viewX, double dataX, double oldDataRange0 = -1, double oldDataRange1 = -1);

	//! Converts y value of transfer function data to histogram view
	//! \param viewY int pointer to save view data
	//! \param dataY Y value of the transfer function data
	void data2viewY(int *viewY, double dataY);

	//! Converts x and y value of transfer function data to histogram view
	//! \param viewX int pointer to save view data
	//! \param viewY int pointer to save view data
	//! \param dataX X value of the transfer function data
	//! \param dataY Y value of the transfer function data
	void data2view(int *viewX, int *viewY, double dataX, double dataY);

	void drawBackground(QPainter &painter);
	void drawHistogram(QPainter &painter);
	void drawAxes(QPainter &painter);

	//! Changes the mode of interaction. Possible modes are NO_MODE, MOVE_VIEW_MODE
	void changeMode(Mode mode, QMouseEvent *event);
	//! Changes the mode of interaction with the mouse wheel. Possible modes are ZOOM_WHEEL_MODE, HIST_ZOOM_WHEEL_MODE
	void changeWheelMode(WheelMode mode);

	//! Zooms the histogram view.
	//! \param value Indicates how big the zoom steps are
	//! \param x position where to zoom in and out
	//! \param deltaMode Indicates if the value is used as delta or absolute value
	void zoomHistogramView(double value, int x, bool deltaMode);

	//! Zooms the histogram.
	//! \param value Indicates how big the zoom steps are
	//! \param deltaMode Indicates if the value is used as delta or absolute value
	void zoomHistogram(double value, bool deltaMode);

public slots:
	//! If set to true rendering view updates are done automatically after changes in the transfer function.
	void autoUpdate(bool toggled);
	//! Reset the histogram view and redraw it.
	void resetView();
};
