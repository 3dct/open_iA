/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <iASlicerMode.h>

#include <QWidget>

struct iANModalSeed;
struct iANModalLabel;
class iANModalSeedVisualizer;

class iAMdiChild;

class QTimer;

class iANModalSeedTracker : public QObject
{
	Q_OBJECT

public:
	iANModalSeedTracker();
	iANModalSeedTracker(iAMdiChild* mdiChild);
	void reinitialize(iAMdiChild* mdiChild);

	~iANModalSeedTracker();
	void teardown();

	void addSeeds(const QList<iANModalSeed>&, const iANModalLabel&);
	void removeSeeds(const QList<iANModalSeed>&);
	void removeAllSeeds();

	void updateLater();

private:
	iANModalSeedVisualizer* m_visualizers[iASlicerMode::SlicerCount];

signals:
	void binClicked(iASlicerMode slicerMode, int sliceNumber);
};

class iANModalSeedVisualizer : public QWidget
{
	Q_OBJECT

public:
	iANModalSeedVisualizer(iAMdiChild* mdiChild, iASlicerMode mode);
	void reinitialize(iAMdiChild* mdiChild);
	void teardown();

	void addSeeds(const QList<iANModalSeed>&, const iANModalLabel&);
	void removeSeeds(const QList<iANModalSeed>&);
	void removeAllSeeds();

	void updateLater();

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	iASlicerMode m_mode;
	std::vector<unsigned int> m_values;
	QImage m_image;
	std::vector<unsigned int> m_imageAccumulator;
	QTimer* m_timer_resizeUpdate;

	size_t m_indexHovered;

	size_t yToSliceNumber(int y);
	int sliceNumberToY(size_t sliceNumber);

	void paint();
	void autoresize();
	void resize(int width, int heigth);
	void click(int y);
	void hover(int y);
	void leave();

	void update_imageLargerThanBuffer();
	void update_imageSmallerThanBuffer();
	void drawLine(int lineY, double value, double maxValue, bool selected);

signals:
	void binClicked(size_t sliceNumber);

public slots:
	void update();
};
