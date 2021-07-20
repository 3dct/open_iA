/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iANModalSeedTracker.h"

#include "iANModalObjects.h"

#include <iAMdiChild.h>
#include <iASlicer.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QTimer>

// iANModalSeedTracker ----------------------------------------------------------------

iANModalSeedTracker::iANModalSeedTracker()
{
	// Do nothing
}

iANModalSeedTracker::iANModalSeedTracker(iAMdiChild* mdiChild)
{
	reinitialize(mdiChild);
}

void iANModalSeedTracker::reinitialize(iAMdiChild* mdiChild)
{
	constexpr iASlicerMode modes[iASlicerMode::SlicerCount]{iASlicerMode::YZ, iASlicerMode::XZ, iASlicerMode::XY};

	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		auto mode = modes[i];
		m_visualizers[i] = new iANModalSeedVisualizer(mdiChild, mode);
		connect(m_visualizers[i], &iANModalSeedVisualizer::binClicked,
			[this, mode](size_t sliceNumber) { emit binClicked(mode, sliceNumber); });
	}
}

iANModalSeedTracker::~iANModalSeedTracker()
{
	teardown();
}

#define iANModal_FOR_EACH_VISUALIZER_DO(visualizer, athing)                                       \
	for (int iANModal_vis_id = 0; iANModal_vis_id < iASlicerMode::SlicerCount; ++iANModal_vis_id) \
	{                                                                                             \
		auto visualizer = m_visualizers[iANModal_vis_id];                                         \
		athing;                                                                                   \
	}

void iANModalSeedTracker::teardown()
{
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->teardown());
}

void iANModalSeedTracker::addSeeds(const QList<iANModalSeed>& seeds, const iANModalLabel& label)
{
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->addSeeds(seeds, label));
}

void iANModalSeedTracker::removeSeeds(const QList<iANModalSeed>& seeds)
{
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->removeSeeds(seeds));
}

void iANModalSeedTracker::removeAllSeeds()
{
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->removeAllSeeds());
}

void iANModalSeedTracker::updateLater()
{
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->updateLater());
}

// iANModalSeedVisualizer -------------------------------------------------------------

iANModalSeedVisualizer::iANModalSeedVisualizer(iAMdiChild* mdiChild, iASlicerMode mode) :
	m_mode(mode), m_timer_resizeUpdate(new QTimer()), m_indexHovered(std::numeric_limits<size_t>::max())
{
	mdiChild->slicerContainerLayout(m_mode)->addWidget(this, 0);

	setMouseTracking(true);

	setToolTipDuration(-1);

	m_timer_resizeUpdate->setSingleShot(true);
	m_timer_resizeUpdate->setInterval(1000);  // In ms
	connect(m_timer_resizeUpdate, &QTimer::timeout, this, &iANModalSeedVisualizer::update);

	reinitialize(mdiChild);
}

void iANModalSeedVisualizer::reinitialize(iAMdiChild* mdiChild)
{
	auto slicerScrollBar = mdiChild->slicerScrollBar(m_mode);

	int scrollbarWidth = slicerScrollBar->minimumSize().width();
	int imageWidth = scrollbarWidth < 20 ? 20 : scrollbarWidth;
	setMinimumSize(QSize(imageWidth, 0));

	int min = slicerScrollBar->minimum();
	int max = slicerScrollBar->maximum();
	int range = max - min + 1;

	m_values.resize(range);
	std::fill(m_values.begin(), m_values.end(), 0);

	autoresize();
};

void iANModalSeedVisualizer::teardown()
{
	deleteLater();
}

namespace
{
	template <typename OpT>
	void processSeedCoord(const QList<iANModalSeed>& seeds, iASlicerMode mode, OpT op)
	{
		for (const auto& seed : seeds)
		{
			int i = -1;
			switch (mode)
			{
			case iASlicerMode::XY:
				i = seed.z;
				break;
			case iASlicerMode::XZ:
				i = seed.y;
				break;
			case iASlicerMode::YZ:
				i = seed.x;
				break;
			default:
				break;  // TODO: error
			}
			if (i != -1)
			{
				op(i);
			}
		}
	}
}

void iANModalSeedVisualizer::addSeeds(const QList<iANModalSeed>& seeds, const iANModalLabel& label)
{
	Q_UNUSED(label);
	processSeedCoord(seeds, m_mode, [this](int i) { ++m_values[i]; });
}

void iANModalSeedVisualizer::removeSeeds(const QList<iANModalSeed>& seeds)
{
	processSeedCoord(seeds, m_mode, [this](int i) { --m_values[i]; });
}

void iANModalSeedVisualizer::removeAllSeeds()
{
	std::fill(m_values.begin(), m_values.end(), 0);
}

void iANModalSeedVisualizer::updateLater()
{
	m_timer_resizeUpdate->start();
}

void iANModalSeedVisualizer::update()
{
	if (static_cast<size_t>(m_image.height()) > m_values.size())
	{
		update_imageLargerThanBuffer();
	}
	else
	{
		update_imageSmallerThanBuffer();
	}
	QWidget::update();
}

static constexpr QRgb HISTOGRAM_COLOR_FOREGROUND = qRgb(0, 114, 189);
static constexpr QRgb HISTOGRAM_COLOR_FOREGROUND_SELECTED = qRgb(0, 0, 0);
static constexpr QRgb HISTOGRAM_COLOR_BACKGROUND = qRgb(255, 255, 255);
static constexpr QRgb HISTOGRAM_COLOR_BACKGROUND_SELECTED = qRgb(191, 191, 191);

inline void iANModalSeedVisualizer::drawLine(int lineY, double value, double maxValue, bool selected)
{
	QRgb fg, bg;
	if (selected)
	{
		fg = HISTOGRAM_COLOR_FOREGROUND_SELECTED;
		bg = HISTOGRAM_COLOR_BACKGROUND_SELECTED;
	}
	else
	{
		fg = HISTOGRAM_COLOR_FOREGROUND;
		bg = HISTOGRAM_COLOR_BACKGROUND;
	}

	double lineLength_double = (value / maxValue) * ((double)m_image.width());
	int lineLength = ceil(lineLength_double);
	assert((value == 0.0 && lineLength == 0) || (lineLength >= 1 && lineLength <= m_image.width()));

	QRgb* line = (QRgb*)m_image.scanLine(lineY);
	int x = 0;
	for (; x < lineLength; ++x)
	{
		line[x] = fg;
	}
	for (; x < m_image.width(); ++x)
	{
		line[x] = bg;
	}
}

inline void iANModalSeedVisualizer::update_imageLargerThanBuffer()
{
	m_imageAccumulator.resize(0);
	unsigned int maxValue = *std::max_element(m_values.begin(), m_values.end());
	if (maxValue == 0)
	{
		m_image.fill(HISTOGRAM_COLOR_BACKGROUND);
		return;
	}
	double maxValue_double = (double)maxValue;
	for (int y = 0; y < m_image.height(); ++y)
	{
		size_t index = yToSliceNumber(y);
		unsigned int value = m_values[index];
		drawLine(y, value, maxValue_double, index == m_indexHovered);
	}
}

void iANModalSeedVisualizer::update_imageSmallerThanBuffer()
{
	if (m_imageAccumulator.size() != static_cast<size_t>(m_image.height()))
	{
		m_imageAccumulator.resize(m_image.height());
	}
	std::fill(m_imageAccumulator.begin(), m_imageAccumulator.end(), 0);
	unsigned int maxValue = 0;
	int ySelected = -1;
	for (size_t index = 0; index < m_values.size(); ++index)
	{
		unsigned int value = m_values[index];
		int y = sliceNumberToY(index);
		unsigned int accumulatedValue = m_imageAccumulator[y] + value;
		m_imageAccumulator[y] = accumulatedValue;

		if (accumulatedValue > maxValue)
		{
			maxValue = accumulatedValue;
		}

		if (index == m_indexHovered)
		{
			ySelected = y;
		}
	}
	if (maxValue == 0)
	{
		m_image.fill(HISTOGRAM_COLOR_BACKGROUND);
		return;
	}
	double maxValue_double = (double)maxValue;
	for (int y = 0; y < m_image.height(); ++y)
	{
		int index = y;
		unsigned int value = m_imageAccumulator[index];
		drawLine(y, value, maxValue_double, ySelected == y);
	}
}

inline size_t iANModalSeedVisualizer::yToSliceNumber(int y)
{
	int y_inv = m_image.height() - 1 - y;
	float valueIndex_float = ((float)y_inv / (float)(m_image.height() - 1) * (float)(m_values.size() - 1));
	size_t valueIndex = round(valueIndex_float);
	assert(valueIndex >= 0 && valueIndex < m_values.size());
	return valueIndex;
}

inline int iANModalSeedVisualizer::sliceNumberToY(size_t sliceNumber)
{
	float y_inv_float = (float)sliceNumber / (float)(m_values.size() - 1) * (float)(m_image.height() - 1);
	int y_inv = round(y_inv_float);
	int y = m_image.height() - 1 - y_inv;
	assert(y >= 0 && y < m_image.height());
	return y;
}

// EVENT FUNCTIONS --------------------------------------------------------------------

void iANModalSeedVisualizer::paint()
{
	QPainter p(this);
	p.drawImage(0, 0, m_image);
}

void iANModalSeedVisualizer::autoresize()
{
	resize(size().width(), size().height());
}

void iANModalSeedVisualizer::resize(int width, int height)
{
	if (m_image.isNull())
	{
		m_image = QImage(width, height, QImage::Format::Format_RGB32);
		update();
	}
	else
	{
		m_image = m_image.scaled(width, height);
		updateLater();
	}
}

void iANModalSeedVisualizer::click(int y)
{
	size_t index = yToSliceNumber(y);
	emit binClicked(index);
}

void iANModalSeedVisualizer::hover(int y)
{
	size_t index = yToSliceNumber(y);
	unsigned int value = m_values[index];

	bool aggregatedSlices = static_cast<size_t>(m_image.height()) < m_values.size();

	QString aggregatedSlicesText = "(because the height of this histogram\n"
								   "is too small to display one bin per\n"
								   "slice, some bins may contain the\n"
								   "added seed count of multiple slices)";

	QString sliceInfo = QString(aggregatedSlices ? "Click to select slice #%1" : "Slice #%1").arg(index);

	QString seedInfo = QString("%0 seed%1").arg(value).arg(value == 1 ? "" : "s");

	QString tooltipText;
	tooltipText += sliceInfo;
	if (!aggregatedSlices)
		tooltipText += "\n" + seedInfo;
	if (aggregatedSlices)
		tooltipText += "\n\n" + aggregatedSlicesText;

	setToolTip(tooltipText);

	setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

	if (index != m_indexHovered)
	{
		m_indexHovered = index;
		update();
	}
}

void iANModalSeedVisualizer::leave()
{
	m_indexHovered = std::numeric_limits<size_t>::max();
	update();
}

// EVENTS -----------------------------------------------------------------------------

void iANModalSeedVisualizer::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	paint();
}

void iANModalSeedVisualizer::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
	autoresize();
}

void iANModalSeedVisualizer::mousePressEvent(QMouseEvent* event)
{
	click(event->pos().y());
}

void iANModalSeedVisualizer::mouseMoveEvent(QMouseEvent* event)
{
	hover(event->pos().y());
}

void iANModalSeedVisualizer::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	leave();
}
