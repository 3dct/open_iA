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

#include "iANModalSeedTracker.h"

#include "iANModalObjects.h"

#include "mdichild.h"
#include "dlg_slicer.h"

#include <QSlider>

// iANModalSeedTracker ----------------------------------------------------------------

iANModalSeedTracker::iANModalSeedTracker() {
	// Do nothing
}

iANModalSeedTracker::iANModalSeedTracker(MdiChild *mdiChild) {
	reinitialize(mdiChild);
}

void iANModalSeedTracker::reinitialize(MdiChild *mdiChild) {
	constexpr iASlicerMode modes[iASlicerMode::SlicerCount]{
		iASlicerMode::YZ,
		iASlicerMode::XZ,
		iASlicerMode::XY
	};

	for (int i = 0; i < iASlicerMode::SlicerCount; ++i) {
		m_visualizers[i] = new iANModalSeedVisualizer(mdiChild, modes[i]);
	}
}

iANModalSeedTracker::~iANModalSeedTracker() {
	teardown();
}

#define iANModal_FOR_EACH_VISUALIZER_DO(visualizer, athing) \
for (int iANModal_vis_id = 0; iANModal_vis_id < iASlicerMode::SlicerCount; ++iANModal_vis_id) { \
	auto visualizer = m_visualizers[iANModal_vis_id]; \
	athing; \
}

void iANModalSeedTracker::teardown() {
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->teardown());
}

void iANModalSeedTracker::addSeeds(const QList<iANModalSeed> &seeds, const iANModalLabel &label) {
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->addSeeds(seeds, label));
}

void iANModalSeedTracker::removeSeeds(const QList<iANModalSeed> &seeds) {
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->removeSeeds(seeds));
}

void iANModalSeedTracker::removeSeeds(const iANModalLabel &label) {
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->removeSeeds(label));
}

void iANModalSeedTracker::update() {
	iANModal_FOR_EACH_VISUALIZER_DO(vis, vis->update());
}

// iANModalSeedVisualizer -------------------------------------------------------------

iANModalSeedVisualizer::iANModalSeedVisualizer(MdiChild *mdiChild, iASlicerMode mode) : m_mode(mode) {
	reinitialize(mdiChild);
}

void iANModalSeedVisualizer::reinitialize(MdiChild *mdiChild) {
	auto widget = mdiChild->slicerDockWidget(m_mode);

	int scrollbarWidth = widget->verticalScrollBar->minimumSize().width();
	int imageWidth = scrollbarWidth < 20 ? 20 : scrollbarWidth;
	setMinimumSize(QSize(imageWidth, 0));

	int min = widget->verticalScrollBar->minimum();
	int max = widget->verticalScrollBar->maximum();
	int range = max - min;

	m_values.resize(range);
	std::fill(m_values.begin(), m_values.end(), 0);

	m_valuesLabelled.resize(range);

	if (!m_initialized) {
		widget->horizontalLayout_2->addWidget(this, 0);
	}

	autoresize();
};

void iANModalSeedVisualizer::teardown() {
	deleteLater();
}

void iANModalSeedVisualizer::addSeeds(const QList<iANModalSeed> &seeds, const iANModalLabel &label) {

#define iANModal_ADD_SEEDS(membervar) { \
	for (const auto &seed : seeds) { \
		++m_values[seed.membervar]; \
		auto map = m_valuesLabelled[seed.membervar]; \
		++map[id]; \
	} \
}

	int id = label.id;
	switch (m_mode) {
	case iASlicerMode::XY: iANModal_ADD_SEEDS(z); break;
	case iASlicerMode::XZ: iANModal_ADD_SEEDS(y); break;
	case iASlicerMode::YZ: iANModal_ADD_SEEDS(x); break;
	default: break; // TODO: error 
	}
}

void iANModalSeedVisualizer::removeSeeds(const QList<iANModalSeed> &seeds) {

}

void iANModalSeedVisualizer::removeSeeds(const iANModalLabel &label) {

}

void iANModalSeedVisualizer::update() {
	constexpr QRgb HISTOGRAM_COLOR_FOREGROUND = qRgb(0, 114, 189);
	constexpr QRgb HISTOGRAM_COLOR_BACKGROUND = qRgb(255, 255, 255);

	unsigned int maxValue = *std::max_element(m_values.begin(), m_values.end());

	if (maxValue == 0) {
		//m_image.fill(HISTOGRAM_COLOR_BACKGROUND);
		m_image.fill(QColor(255, 255, 255));

	} else {
		float maxValue_float = (float) maxValue;
		for (int y = 0; y < m_image.height(); ++y) {

			float valueIndex_float = ((float) y / (float) (m_image.height()) * (float) (m_values.size() - 1));
			int valueIndex = round(valueIndex_float);
			assert(valueIndex >= 0 && valueIndex < m_values.size());
			unsigned int value = m_values[valueIndex];

			float lineLength_float = ((float) value / maxValue_float) * ((float) m_image.width());
			int lineLength = ceil(lineLength_float);
			assert((value == 0 && lineLength == 0) || (lineLength >= 1 && lineLength <= m_image.width()));

			QRgb *line = (QRgb*) m_image.scanLine(y);
			int x = 0;
			for (; x < lineLength; ++x) {
				line[x] = HISTOGRAM_COLOR_FOREGROUND;
			}
			for (; x < m_image.width(); ++x) {
				line[x] = HISTOGRAM_COLOR_BACKGROUND;
			}
		}
	}

	QWidget::update();
}

// EVENT FUNCTIONS --------------------------------------------------------------------

void iANModalSeedVisualizer::paint() {
	QPainter p(this);
	p.drawImage(0, 0, m_image);
}

void iANModalSeedVisualizer::autoresize() {
	resize(size().width(), size().height());
}

void iANModalSeedVisualizer::resize(int width, int height) {
	m_image = QImage(width, height, QImage::Format::Format_RGB32);
	// At this point, m_image contains uninitialized data
}

void iANModalSeedVisualizer::click(int y) {

}

void iANModalSeedVisualizer::hover(int y) {

}

// EVENTS -----------------------------------------------------------------------------

void iANModalSeedVisualizer::paintEvent(QPaintEvent* event) {
	paint();
}

void iANModalSeedVisualizer::resizeEvent(QResizeEvent* event) {
	autoresize();
}

void iANModalSeedVisualizer::mousePressEvent(QMouseEvent* event) {
	click(event->pos().y());
}

void iANModalSeedVisualizer::mouseMoveEvent(QMouseEvent* event) {
	hover(event->pos().y());
}