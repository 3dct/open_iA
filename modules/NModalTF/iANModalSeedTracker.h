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

#include "iASlicerMode.h"

#include <QWidget>

struct iANModalSeed;
class iANModalSeedVisualizer;

class MdiChild;

class iANModalSeedTracker {

public:
	iANModalSeedTracker();
	iANModalSeedTracker(MdiChild *mdiChild);
	void reinitialize(MdiChild *mdiChild);

	~iANModalSeedTracker();
	void teardown();

	void addSeeds(const QList<iANModalSeed> &);
	void removeSeeds(const QList<iANModalSeed> &);

	void update();

private:
	iANModalSeedVisualizer *m_visualizers[iASlicerMode::SlicerCount];

};

class iANModalSeedVisualizer : public QWidget {
	Q_OBJECT

public:
	iANModalSeedVisualizer(MdiChild *mdiChild, iASlicerMode mode);
	void reinitialize(MdiChild *mdiChild);
	void teardown();

	void addSeeds(const QList<iANModalSeed> &);
	void removeSeeds(const QList<iANModalSeed> &);

	void update();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	bool m_initialized = false;

	iASlicerMode m_mode;
	std::vector<unsigned int> m_values;
	QImage m_image;

	void paint();
	void autoresize();
	void resize(int width, int heigth);
	void click();
	void hover();

};