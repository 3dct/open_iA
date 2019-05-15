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

#include <QWidget>
#include <QSlider>
#include <QRect>
#include <QPen>

#include <vtkSmartPointer.h>

class iAInterpolationSlider;

class QResizeEvent;
class QPaintEvent;
class QImage;
class QTimer;

class vtkImageData;


class iAInterpolationSliderWidget : public QWidget
{
	Q_OBJECT

public:
	//iAInterpolationSliderWidget(Qt::Orientation orientation, QWidget* parent = Q_NULLPTR);
	iAInterpolationSliderWidget(QWidget* parent = Q_NULLPTR);

	double getT() { return m_t; }
	void setT(double t);

private:
	void setTPrivate(double t);

	double m_t;
	iAInterpolationSlider *m_slider;

signals:
	void tChanged(double t);

};



class iAInterpolationSlider : public QWidget
{
	Q_OBJECT

public:
	iAInterpolationSlider(QWidget* parent = Q_NULLPTR);

	double getT();
	void setT(double t);

	void changeModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2);

	void setSchedulerWaitingTimeMs(int waitingTimeMs);
	void resetSchedulerWaitingTime();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event);

private:
	void calculateCoordinatesNow();

	void calculateHistogramLater();
	void calculateHistogramNow();

	void layOut(); // As in the actual verb: to lay out

	double m_t;
	int m_timeToWaitMs;
	QImage *m_histogramImg;
	QTimer *m_timer_histogram;

	QRect m_sliderRect;
	QRect m_histogramRect;
	QPen m_sliderPen;
	QPen m_linePen;
	int m_lineHeight;

	vtkSmartPointer<vtkImageData> m_modalities[2];
	vtkSmartPointer<vtkImageData> m_interpolationVolume;

signals:
	void tChanged(double t);
	void histogramReady();

private slots:
	void onHistogramTimeout();
	void onHistogramReady();

};