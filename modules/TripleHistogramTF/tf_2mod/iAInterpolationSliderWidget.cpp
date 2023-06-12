// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAInterpolationSliderWidget.h"

#include <iAToolsVTK.h>

#include <QDebug>
#include <QGridLayout>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QSpinBox>
#include <QSpacerItem>
#include <QTimer>

#include <vtkImageData.h>
//#include <vtkMath.h>
//#include <vtkSMPTools.h>

#include <cmath>
#include <cassert>

static const int TIMER_T_MS_DEFAULT = 250; // in milliseconds

iAInterpolationSliderWidget::iAInterpolationSliderWidget() :
	m_slider(new iAInterpolationSlider()),
	m_timerT(new QTimer())
{
	for (int i = 0; i < 2; i++)
	{
		auto sb = m_spinBoxes[i] = new QSpinBox(this);
		sb->setRange(0, 100);
		sb->setSingleStep(1);
		sb->setSuffix("%");
	}

	m_timerT->setSingleShot(true); // Fires only once or every interval
	setTWaitingTimeMs(TIMER_T_MS_DEFAULT);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_spinBoxes[0], 0, 0, 1, 1);
	layout->addWidget(m_slider, 1, 0, 1, 2);
	layout->addWidget(m_spinBoxes[1], 2, 0, 1, 1);
	layout->setColumnStretch(0, 0);
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(0, 0);
	layout->setRowStretch(1, 1);
	layout->setRowStretch(2, 0);

	double t = 0.5;
	setT(t);

	connect(m_timerT, &QTimer::timeout, this, &iAInterpolationSliderWidget::onTTimeout);

	connect(m_spinBoxes[0], QOverload<int>::of(&QSpinBox::valueChanged), this, &iAInterpolationSliderWidget::onSpinBox1ValueChanged);
	connect(m_spinBoxes[1], QOverload<int>::of(&QSpinBox::valueChanged), this, &iAInterpolationSliderWidget::onSpinBox2ValueChanged);

	connect(m_slider, &iAInterpolationSlider::tChanged, this, &iAInterpolationSliderWidget::onTChanged);
}

void iAInterpolationSliderWidget::setT(double t)
{
	m_timerT->stop();

	t = t > 1 ? 1 : (t < 0 ? 0 : t); // Make sure t is in range [0,1] (if not, clamp it)

	int a = (1 - t) * 100;
	int b = 100 - a;
	setTPrivate(t, a, b);
	emit tChanged(t);
}

void iAInterpolationSliderWidget::setTLater(double t)
{
	QSignalBlocker blocker(m_slider);
	setTPrivate(t);
	m_timerT->start(m_timeToWaitT);
}

void iAInterpolationSliderWidget::setTPrivate(double t, int a, int b)
{
	assert(a + b == 100);
	assert(t >= 0 && t <= 1);

	int ab[2] = { a, b };
	for (int i = 0; i < 2; i++)
	{
		QSpinBox *sb = m_spinBoxes[i];
		QSignalBlocker blocker(sb);
		sb->setValue(ab[i]);
	}

	QSignalBlocker blocker(m_slider);
	m_slider->setT(t);
}

void iAInterpolationSliderWidget::setTPrivate(double t)
{
	int a = (1 - t) * 100;
	int b = 100 - a;
	setTPrivate(t, a, b);
}

void iAInterpolationSliderWidget::onTChanged(double t)
{
	setTLater(t);
}

void iAInterpolationSliderWidget::onTTimeout()
{
	QSignalBlocker blocker(m_slider);
	setT(m_slider->getT());
}

void iAInterpolationSliderWidget::onSpinBox1ValueChanged(int newValue)
{
	int a = newValue;
	int b = 100 - newValue;
	double t = b / 100.0f;
	setTPrivate(t, a, b);
}

void iAInterpolationSliderWidget::onSpinBox2ValueChanged(int newValue)
{
	int b = newValue;
	int a = 100 - newValue;
	double t = b / 100.0f;
	setTPrivate(t, a, b);
}

void iAInterpolationSliderWidget::changeModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2)
{
	m_slider->changeModalities(d1, d2);
}

void iAInterpolationSliderWidget::setTWaitingTimeMs(int waitingTimeMs)
{
	m_timeToWaitT = waitingTimeMs;
}

void iAInterpolationSliderWidget::resetTWaitingTime()
{
	m_timeToWaitT = TIMER_T_MS_DEFAULT;
}

double iAInterpolationSliderWidget::getT()
{
	return m_slider->getT();
}



// ------------------------------------------------------------------------------------------------
// iAInterpolationSlider
// ------------------------------------------------------------------------------------------------

static const QImage::Format IMAGE_FORMAT = QImage::Format::Format_Grayscale8;
static const int TIMER_HISTOGRAM_MS_DEFAULT = 2500; // in milliseconds
static const int SLIDER_RECTANGLE_WIDTH = 30;
static const int HISTOGRAM_BAR_LENGTH_MIN = 1;

iAInterpolationSlider::iAInterpolationSlider() :
	m_histogramImg(new QImage(0, 0, IMAGE_FORMAT)),
	m_timerHistogram(new QTimer())
{
	m_sliderPen.setWidth(4);
	m_sliderPen.setColor(Qt::black);

	m_sliderHandlePen.setWidth(2);
	m_sliderHandlePen.setColor(Qt::red);

	// Create slider handle
	{
		int l = 0;
		int r = SLIDER_RECTANGLE_WIDTH;
		int t = 0;
		int w = SLIDER_RECTANGLE_WIDTH;
		int h = SLIDER_RECTANGLE_WIDTH;
		int cy = t + h / 2;

		m_sliderHandle.addEllipse(0, -cy, w, h);
		m_sliderHandle.moveTo(r, 0);
		m_sliderHandle.lineTo(l, 0);

		m_sliderHandleBrush = QBrush(QColor::fromRgb(0, 0, 0, 0));  // TODO: values from 0..255 -> was 0.5 in alpha -> maybe 127 was meant?
	}

	//setMouseTracking(true); // to enable mouse move events without the mouse button needing to be pressed

	m_timerHistogram->setSingleShot(true); // Fires only once or every interval
	setHistogramWaitingTimeMs(TIMER_HISTOGRAM_MS_DEFAULT);

	connect(this, &iAInterpolationSlider::volumeReady, this, &iAInterpolationSlider::onVolumeReady);
}

double iAInterpolationSlider::getT()
{
	return m_t;
}

void iAInterpolationSlider::onHistogramTimeout()
{
	calculateHistogramNow();
}

void iAInterpolationSlider::onVolumeReady()
{
	connect(this, &iAInterpolationSlider::histogramReady, this, &iAInterpolationSlider::onHistogramReady);
	connect(m_timerHistogram, &QTimer::timeout, this, &iAInterpolationSlider::onHistogramTimeout);
	calculateHistogramNow();
}

void iAInterpolationSlider::onHistogramReady()
{
	update();
}

void iAInterpolationSlider::setHistogramWaitingTimeMs(int waitingTimeMs)
{
	m_timeToWaitHistogramMs = waitingTimeMs;
}

void iAInterpolationSlider::resetHistogramWaitingTime()
{
	m_timeToWaitHistogramMs = TIMER_HISTOGRAM_MS_DEFAULT;
}

void iAInterpolationSlider::changeModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2)
{
	m_modalities[0] = d1;
	m_modalities[1] = d2;
	calculateCoordinatesNow();
}



// Lay out and paint

bool iAInterpolationSlider::isPointInSlider(QPoint p)
{
	QRect rect = QRect(0, 0, SLIDER_RECTANGLE_WIDTH, m_sliderHeight);
	return rect.contains(p);
}

void iAInterpolationSlider::layOut()
{
	int w = size().width();
	int h = size().height();

	m_sliderRect = QRect(0, 0, SLIDER_RECTANGLE_WIDTH, h);
	m_histogramRect = QRect(m_sliderRect.width(), 0, (w - m_sliderRect.width()), h);
	m_sliderHeight = h;
}

void iAInterpolationSlider::paintEvent(QPaintEvent* /*event*/)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// Paint slider
	p.setPen(m_sliderPen);
	p.drawRect(m_sliderRect);

	// Paint handle
	int handleHeight = getT() * m_sliderHeight;
	auto sliderHandle = m_sliderHandle;
	sliderHandle.translate(0, handleHeight);
	//p.fillPath(sliderHandle, m_sliderHandleBrush);
	p.setPen(m_sliderHandlePen);
	p.drawPath(sliderHandle);

	// Paint histogram
	p.drawImage(m_histogramRect, m_histogramImg->scaled(m_histogramRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void iAInterpolationSlider::resizeEvent(QResizeEvent* /*event*/)
{
	layOut();
	calculateHistogramLater();
}


// Mouse events

void iAInterpolationSlider::mousePressEvent(QMouseEvent* event)
{
	if (isPointInSlider(event->pos()))
	{
		int y = event->pos().y();
		double t = (double)y / (double)m_sliderHeight;
		setT(t);
	}
}

void iAInterpolationSlider::mouseMoveEvent(QMouseEvent* event)
{
	// Only called on mouse drag
	int y = event->pos().y();
	y = y < 0 ? 0 : (y > m_sliderHeight ? m_sliderHeight : y); // clamp 'y' to range [0, m_sliderHeight]
	double t = (double)y / (double)m_sliderHeight;
	setT(t);
}



// Schedule

void iAInterpolationSlider::calculateHistogramLater()
{
	m_timerHistogram->start(m_timeToWaitHistogramMs);
}




// Now (do not schedule)

void iAInterpolationSlider::setT(double t)
{
	m_t = t;
	update();
	emit tChanged(m_t);
}

void iAInterpolationSlider::calculateCoordinatesNow()
{
	m_interpolationVolume = vtkSmartPointer<vtkImageData>::New();

	auto d1 = m_modalities[0]; // Data 1
	auto d2 = m_modalities[1]; // Data 2

	// Get dimensions from first volume only, assuming all two have the same dimensions! TODO change?
	int* dims = d1->GetDimensions();
	m_interpolationVolume->SetDimensions(dims[0], dims[1], dims[2]);

	// One component: the interpolation value in the range [0 1].
	// Interpolation between 'a' and 'b' with value 't': a*(1-t) + b*(t)
	m_interpolationVolume->AllocateScalars(VTK_FLOAT, 1);

	double rangea[2], rangeb[2];
	d1->GetScalarRange(rangea);
	d2->GetScalarRange(rangeb);
	rangea[1] -= rangea[0];
	rangeb[1] -= rangeb[0];

	// TODO parallelize
	float a, b, sum, *values;
	FOR_VTKIMG_PIXELS(m_interpolationVolume, x, y, z)
	{
		a = d1->GetScalarComponentAsFloat(x, y, z, 0);
		b = d2->GetScalarComponentAsFloat(x, y, z, 0);

		if (qIsNaN(a)) a = 0; else a = (a - rangea[0]) / rangea[1];
		if (qIsNaN(b)) b = 0; else b = (b - rangeb[0]) / rangeb[1];

		sum = a + b;

		values = static_cast<float*>(m_interpolationVolume->GetScalarPointer(x, y, z));
		if (sum == 0)
		{
			values[0] = 0.5;
		}
		else
		{
			values[0] = a / sum;
		}
	}

	emit volumeReady();
}

void iAInterpolationSlider::calculateHistogramNow()
{
	m_timerHistogram->stop();

	int h = m_histogramRect.height();
	int w = m_histogramRect.width();

	if (h == 0 || w == 0)
	{
		return;
	}

	auto counter = std::vector<unsigned long>(h);
	std::fill(counter.begin(), counter.end(), 0);

	unsigned long max = 0;
	//unsigned long min = VTK_UNSIGNED_LONG_MAX;

	// On each iteration: ONE 3D-texture lookup, ONE 1D-texture lookup and ONE 1D-texture write
	// Could be implemented in parallel... TODO
	FOR_VTKIMG_PIXELS(m_interpolationVolume, x, y, z)
	{
		float t = m_interpolationVolume->GetScalarComponentAsFloat(x, y, z, 0);
		int pos = std::floor( (h-1) * t );

		assert(pos >= 0 && pos < h);

		unsigned long c = ++counter[pos];
		max = c > max ? c : max;
		//min = vtkMath::Min(min, c);

		//counter->SetScalarComponentFromDouble(pos, 0, 0, 0, c);
	}

	const int histogramBarLengthInterval = (w - 1) - HISTOGRAM_BAR_LENGTH_MIN;

	// Go through every pixel and set the pixel color based on the counts array
	// TODO parallelize
	// TODO accelerate using QImage::scanLine()
	QImage *buf = new QImage(w, h, IMAGE_FORMAT);
	buf->fill(Qt::white);
	if (max > 0)
	{
		double k = static_cast<double>(histogramBarLengthInterval) / std::log(max);
		for (int y = 0; y < h; y++)
		{
			unsigned long c = counter[y];
			if (c > 0)
			{
				int length = k * std::log(c) + HISTOGRAM_BAR_LENGTH_MIN;
				//int length = (c / max) * histogramBarLengthInterval + HISTOGRAM_BAR_LENGTH_MIN;
				assert(length >= 0 && length <= w);
				for (int x = 0; x < length; x++)
				{
					buf->setPixelColor(x, y, QColor(100, 100, 100));
				}
			}
		}
	}
	auto del = m_histogramImg;
	m_histogramImg = buf;
	delete del;

	emit histogramReady();
}
