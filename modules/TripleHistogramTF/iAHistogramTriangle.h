/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include "iATripleModalityWidget.h"

class iAHistogramTriangle : public iATripleModalityWidget
{
public:
	iAHistogramTriangle(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f = 0);

	void initialize() override;
	void setModalityLabel(QString label, int index) override;

	void paintTriangle(QPainter &p);
	void paintHistograms(QPainter &p);
	void paintSlicers(QPainter &p);

protected:
	void paintEvent(QPaintEvent* event);
	void resizeEvent(QResizeEvent* event);

	void mousePressEvent(QMouseEvent *event) { forwardMouseEvent(event); }
	void mouseMoveEvent(QMouseEvent *event) { forwardMouseEvent(event); }
	void mouseReleaseEvent(QMouseEvent *event) { forwardMouseEvent(event); }

private:
	void calculatePositions() { calculatePositions(size().width(), size().height()); }
	void calculatePositions(int w, int h);

	void forwardMouseEvent(QMouseEvent *event);

	QTransform m_transformHistogramA; // left
	QTransform m_transformHistogramB; // right
	QTransform m_transformHistogramC; // bottom

	int left, top, right, bottom, centerX, width, height;
};