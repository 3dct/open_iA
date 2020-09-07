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

void iANModalSeedTracker::teardown() {
	for (auto vis : m_visualizers) {
		vis->teardown();
	}
}

void iANModalSeedTracker::addSeeds(const QList<iANModalSeed> &seeds) {
	for (auto vis : m_visualizers) {
		vis->addSeeds(seeds);
	}
}

void iANModalSeedTracker::removeSeeds(const QList<iANModalSeed> &seeds) {
	for (auto vis : m_visualizers) {
		vis->removeSeeds(seeds);
	}
}

void iANModalSeedTracker::update() {
	for (auto vis : m_visualizers) {
		vis->update();
	}
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

	if (!m_initialized) {
		widget->horizontalLayout_2->addWidget(this, 0);
	}

	autoresize();
};

void iANModalSeedVisualizer::teardown() {
	deleteLater();
}

void iANModalSeedVisualizer::addSeeds(const QList<iANModalSeed> &seeds) {

}

void iANModalSeedVisualizer::removeSeeds(const QList<iANModalSeed> &seeds) {

}

void iANModalSeedVisualizer::update() {

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
	m_image = QImage(width, height, QImage::Format::Format_RGB888);
	// At this point, m_image contains uninitialized data
}

void iANModalSeedVisualizer::click() {

}

void iANModalSeedVisualizer::hover() {

}

// EVENTS -----------------------------------------------------------------------------

void iANModalSeedVisualizer::paintEvent(QPaintEvent* event) {

}

void iANModalSeedVisualizer::resizeEvent(QResizeEvent* event) {
	autoresize();
	
	// temporary TODO remove
	m_image.fill(QColor(255, 0, 0));
}

void iANModalSeedVisualizer::mousePressEvent(QMouseEvent* event) {

}

void iANModalSeedVisualizer::mouseMoveEvent(QMouseEvent* event) {

}