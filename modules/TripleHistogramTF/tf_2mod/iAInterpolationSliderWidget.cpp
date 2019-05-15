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

#include "iAInterpolationSliderWidget.h"

#include <iAToolsVTK.h>

#include <QResizeEvent>
#include <QVBoxLayout>
#include <QImage>
#include <QTimer>
#include <QPainter>
#include <QDebug>

#include <vtkImageData.h>
#include <vtkMath.h>
//#include <vtkSMPTools.h>



//iAInterpolationSlider::iAInterpolationSlider(Qt::Orientation orientation, QWidget* parent)
iAInterpolationSliderWidget::iAInterpolationSliderWidget(QWidget* parent) :
	m_slider(new iAInterpolationSlider(this))
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_slider, 0);

	setT(0.5);

	connect(m_slider, SIGNAL(tChanged(double)), this, SIGNAL(tChanged(double)));
}

void iAInterpolationSliderWidget::setT(double t)
{
	t = t > 1 ? 1 : (t < 0 ? 0 : t); // Make sure t is in range [0,1] (if not, clamp it)
	setTPrivate(t);
}

void iAInterpolationSliderWidget::setTPrivate(double t)
{
	m_t = t;

	int a = (1 - t) * 100;
	int b = 100 - a;

	emit tChanged(t);
}



// ------------------------------------------------------------------------------------------------
// iAInterpolationSlider 
// ------------------------------------------------------------------------------------------------

static const QImage::Format IMAGE_FORMAT = QImage::Format::Format_Grayscale8;
static const int TIMER_MS_DEFAULT = 5000;
static const int SLIDER_RECTANGLE_WIDTH = 30;

iAInterpolationSlider::iAInterpolationSlider(QWidget* parent) :
	m_timer_histogram(new QTimer()),
	m_histogramImg(new QImage(0, 0, IMAGE_FORMAT))
{
	m_sliderPen.setWidth(4);
	m_sliderPen.setColor(Qt::black);

	m_linePen.setWidth(2);
	m_sliderPen.setColor(Qt::black);

	m_timer_histogram->setSingleShot(true); // Fires only once or every interval
	setSchedulerWaitingTimeMs(TIMER_MS_DEFAULT); // 2.5 seconds
	connect(m_timer_histogram, SIGNAL(timeout()), this, SLOT(onHistogramTimeout()));

	connect(this, SIGNAL(histogramReady()), this, SLOT(onHistogramReady()));
}

double iAInterpolationSlider::getT() {
	return m_t;
}

void iAInterpolationSlider::setT(double t) {
	m_t = t;
	emit tChanged(m_t);
}

void iAInterpolationSlider::onHistogramTimeout() {
	calculateHistogramNow();
}

void iAInterpolationSlider::onHistogramReady() {
	update();
}

void iAInterpolationSlider::setSchedulerWaitingTimeMs(int waitingTimeMs) {
	m_timeToWaitMs = waitingTimeMs;
}

void iAInterpolationSlider::resetSchedulerWaitingTime() {
	m_timeToWaitMs = TIMER_MS_DEFAULT;
}

void iAInterpolationSlider::changeModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2) {
	m_modalities[0] = d1;
	m_modalities[1] = d2;
	calculateCoordinatesNow();
	calculateHistogramNow();
}



// Lay out and paint

void iAInterpolationSlider::layOut() {
	int w = size().width();
	int h = size().height();

	m_sliderRect = QRect(0, 0, SLIDER_RECTANGLE_WIDTH, h);
	m_histogramRect = QRect(m_sliderRect.width(), 0, (w - m_sliderRect.width()), h);
	m_lineHeight = (1 - getT()) * h;
}

void iAInterpolationSlider::paintEvent(QPaintEvent* event) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// Paint slider
	p.setPen(m_sliderPen);
	p.drawRect(m_sliderRect);

	// Paint line
	p.setPen(m_linePen);
	p.drawLine(0, m_lineHeight, SLIDER_RECTANGLE_WIDTH, m_lineHeight);

	// Paint histogram
	p.drawImage(m_histogramRect, *m_histogramImg, m_histogramImg->rect());
}

void iAInterpolationSlider::resizeEvent(QResizeEvent* event) {
	layOut();
	calculateHistogramLater();
}



// Schedule

void iAInterpolationSlider::calculateHistogramLater() {
	m_timer_histogram->start(m_timeToWaitMs);
}



// Now (do not schedule)

void iAInterpolationSlider::calculateCoordinatesNow() {
	m_interpolationVolume = vtkSmartPointer<vtkImageData>::New();

	auto d1 = m_modalities[0]; // Data 1
	auto d2 = m_modalities[1]; // Data 2

	// Get dimensions from first volume only, assuming all two have the same dimensions! TODO change?
	int* dims = d1->GetDimensions();
	m_interpolationVolume->SetDimensions(dims[0], dims[1], dims[2]);

	// One component: the interpolation value in the range [0 1].
	// Interpolation between 'a' and 'b' with value 't': a*(1-t) + b*(t)
	m_interpolationVolume->AllocateScalars(VTK_DOUBLE, 1);

	double rangea[2], rangeb[2], rangec[2];
	d1->GetScalarRange(rangea);
	d2->GetScalarRange(rangeb);
	rangea[1] -= rangea[0];
	rangeb[1] -= rangeb[0];
	rangec[1] -= rangec[0];

	// TODO parallelize
	double a, b, sum, *values;
	FOR_VTKIMG_PIXELS(m_interpolationVolume, x, y, z) {
		a = d1->GetScalarComponentAsDouble(x, y, z, 0);
		b = d2->GetScalarComponentAsDouble(x, y, z, 0);

		if (qIsNaN(a)) a = 0; else a = (a - rangea[0]) / rangea[1];
		if (qIsNaN(b)) b = 0; else b = (b - rangeb[0]) / rangeb[1];

		sum = a + b;

		values = static_cast<double*>(m_interpolationVolume->GetScalarPointer(x, y, z));
		if (sum == 0) {
			values[0] = 0.5;
			values[1] = 0.5;
		} else {
			values[0] = a / sum;
			values[1] = b / sum;
		}
	}
}

void iAInterpolationSlider::calculateHistogramNow() {
	int h = m_histogramRect.height();
	int w = m_histogramRect.width();

	auto counter = vtkSmartPointer<vtkImageData>::New();
	counter->SetDimensions(h, 1, 1);
	counter->AllocateScalars(VTK_UNSIGNED_LONG, 1); // Maximum: 4.294.967.295 (2^32 - 1)

	unsigned long max = 0;
	//unsigned long min = VTK_UNSIGNED_LONG_MAX;

	// On each iteration: ONE 3D-texture lookup, ONE 1D-texture lookup and ONE 1D-texture write
	// Could be implemented in parallel... TODO
	FOR_VTKIMG_PIXELS(m_interpolationVolume, x, y, z) {
		double t = m_interpolationVolume->GetScalarComponentAsDouble(x, y, z, 0);
		int pos = vtkMath::Round(h * t);

		// Count plus one -> -> -> -> -> -> -> -> -> -> -> -> -> -> -> -> -> -v right here
		unsigned long c = counter->GetScalarComponentAsDouble(pos, 1, 1, 0) + 1;
		max = vtkMath::Max(max, c);
		//min = vtkMath::Min(min, c);
		
		counter->SetScalarComponentFromDouble(pos, 1, 1, 0, c);
	}

	// Go through every pixel and set the pixel color based on the counts 1D-image
	// TODO parallelize
	// TODO accelerate using QImage::scanLine()
	QImage *buf = new QImage(w, h, IMAGE_FORMAT);
	double k = (double)w / (double)log(max);
	int grayValue, count;
	for (int y = 0; y < h; y++) {
		unsigned long c = counter->GetScalarComponentAsDouble(y, 1, 1, 0);
		int length = k * log(c);
		assert(length >= 0);
		assert(length <= w);
		for (int x = 0; x < length; x++) {
			buf->setPixelColor(x, y, QColor(160, 160, 160));
		}
	}
	auto del = m_histogramImg;
	m_histogramImg = buf;
	delete del;

	emit histogramReady();
}