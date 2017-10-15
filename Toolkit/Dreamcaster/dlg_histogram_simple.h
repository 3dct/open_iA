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
#ifndef DLG_HISTOGRAM_H
#define DLG_HISTOGRAM_H


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

	// Constructor/Destructor
	dlg_histogram_simple(QWidget *parent);
	~dlg_histogram_simple();
	void initialize(unsigned int* histData, int numberOfBeans, double _dataRange[2]);

	bool isUpdateAutomatically();

	//draw the histogram
	void drawHistogram();
	void redraw();

protected:
	void paintEvent(QPaintEvent * );
	void resizeEvent (QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void enterEvent(QEvent *event);
	void leaveEvent(QEvent *event);
	void wheelEvent(QWheelEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

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
	
	void view2dataX(double *dataX, int viewX);
	void view2dataY(double *dataY, int viewY);
	void view2data(double *dataX, double *dataY, int viewX, int viewY);
	void data2viewX(int *viewX, double dataX, double oldDataRange0 = -1, double oldDataRange1 = -1);
	void data2viewY(int *viewY, double dataY);
	void data2view(int *viewX, int *viewY, double dataX, double dataY);
	
	void drawBackground(QPainter &painter);
	void drawHistogram(QPainter &painter);
	void drawAxes(QPainter &painter);
	void changeMode(Mode mode, QMouseEvent *event);
	void changeWheelMode(WheelMode mode);

	void zoomHistogramView(double value, int x, bool deltaMode);
	void zoomHistogram(double value, bool deltaMode);

public slots:
	void autoUpdate(bool toggled);
	void resetView();
};

#endif // dlg_histogram_H
