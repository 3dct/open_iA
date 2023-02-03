// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QPainterPath>
#include <QPen>
#include <QRect>
#include <QSlider>
#include <QWidget>

#include <vtkSmartPointer.h>

class iAInterpolationSlider;

class QResizeEvent;
class QPaintEvent;
class QImage;
class QTimer;
class QSpinBox;

class vtkImageData;


class iAInterpolationSliderWidget : public QWidget
{
	Q_OBJECT

public:
	//iAInterpolationSliderWidget(Qt::Orientation orientation, QWidget* parent = Q_NULLPTR);
	iAInterpolationSliderWidget();

	double getT();
	void setT(double t);

	void setWeightA(double t)
	{
		setT(1 - t);
	}
	void setWeightB(double t)
	{
		setT(t);
	}

	void setTWaitingTimeMs(int waitingTimeMs);
	void resetTWaitingTime();

	void changeModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2);

private:
	void setTLater(double t);
	void setTPrivate(double t);
	void setTPrivate(double t, int a, int b);

	QSpinBox *m_spinBoxes[2];

	iAInterpolationSlider *m_slider;

	int m_timeToWaitT;
	QTimer *m_timerT;

signals:
	void tChanged(double t);

private slots:
	void onTTimeout();
	void onTChanged(double t);
	void onSpinBox1ValueChanged(int newValue);
	void onSpinBox2ValueChanged(int newValue);

};



class iAInterpolationSlider : public QWidget
{
	Q_OBJECT

public:
	iAInterpolationSlider();

	double getT();
	void setT(double t);

	void changeModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2);

	void setHistogramWaitingTimeMs(int waitingTimeMs);
	void resetHistogramWaitingTime();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	void calculateCoordinatesNow();

	void calculateHistogramLater();
	void calculateHistogramNow();

	void layOut(); // As in the actual verb: to lay out

	bool isPointInSlider(QPoint p);

	double m_t;

	int m_timeToWaitHistogramMs;
	QImage *m_histogramImg;
	QTimer *m_timerHistogram;

	QRect m_sliderRect;
	QRect m_histogramRect;
	QPen m_sliderPen;
	QPen m_sliderHandlePen;
	int m_sliderHeight;

	QPainterPath m_sliderHandle;
	QBrush m_sliderHandleBrush;

	vtkSmartPointer<vtkImageData> m_modalities[2];
	vtkSmartPointer<vtkImageData> m_interpolationVolume;

signals:
	void tChanged(double t);
	void volumeReady();
	void histogramReady();

private slots:
	void onVolumeReady();
	void onHistogramTimeout();
	void onHistogramReady();

};
