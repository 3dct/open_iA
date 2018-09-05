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
* program.  If not, see http://aw.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iAHistogramTriangle.h"
#include "charts/iADiagramFctWidget.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QtMath>

const static qreal RAD60 = qDegreesToRadians(60.0);
const static qreal SIN60 = qSin(RAD60);
const static qreal COS60 = qCos(RAD60);
const static int HISTOGRAM_HEIGHT = 100;
const static int TRIANGLE_LEFT = qRound(SIN60 * HISTOGRAM_HEIGHT);
const static int TRIANGLE_TOP = qRound(COS60 * HISTOGRAM_HEIGHT);
const static int TRIANGLE_RIGHT = TRIANGLE_LEFT;
const static int TRIANGLE_BOTTOM = HISTOGRAM_HEIGHT;
const static double TRIANGLE_HEIGHT_RATIO = SIN60;
const static double TRIANGLE_WIDTH_RATIO = 1.0 / TRIANGLE_HEIGHT_RATIO;

iAHistogramTriangle::iAHistogramTriangle(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f)
	: iATripleModalityWidget(parent, mdiChild, f)
{
}

void iAHistogramTriangle::initialize()
{
	calculatePositions();
}

void iAHistogramTriangle::setModalityLabel(QString label, int index)
{
	//if (isReady()) {
	//	m_modalityLabels[index]->setText(label);
		iATripleModalityWidget::setModalityLabel(label, index);
	//}
}

void iAHistogramTriangle::resizeEvent(QResizeEvent* event)
{
	calculatePositions(event->size().width(), event->size().height());
}

// EVENTS ----------------------------------------------------------------------------------------------------------

void iAHistogramTriangle::forwardMouseEvent(QMouseEvent *event)
{
	if (m_triangleWidget->getTriangle().contains(event->pos().x(), event->pos().y())) {
		QApplication::sendEvent(m_triangleWidget, event);
		update();
	}
}

// POSITION AND PAINT ----------------------------------------------------------------------------------------------

void iAHistogramTriangle::calculatePositions(int totalWidth, int totalHeight)
{
	//int left, top, right, bottom, centerX, width, height; // Big triangle's positions

	{ // Triangle positions
		int aw  = totalWidth - TRIANGLE_LEFT - TRIANGLE_RIGHT; // Available width for the triangle
		int ah = totalHeight - TRIANGLE_TOP - TRIANGLE_BOTTOM;

		if ((double)aw / (double)ah < TRIANGLE_WIDTH_RATIO) {
			width = aw;
			height = qRound(aw * TRIANGLE_HEIGHT_RATIO);
		} else {
			width = qRound(ah * TRIANGLE_WIDTH_RATIO);
			height = ah;
		}

		//        /\ 
		//       /  \    BIG TRIANGLE
		//      /    \   bounding box: left, top, right, bottom (, centerX)
		//     /______\ 
		left = (aw - width) / 2 + TRIANGLE_LEFT;
		top = (ah - height) / 2 + TRIANGLE_TOP;
		right = left + width;
		bottom = top + height;
		centerX = left + (width / 2);

		//        /\           /\
		//       /  \         /__\       ____   SMALL TRIANGLE
		//      /    \  -->  /\  /\  --> \  /   bounding box: l, t, r, b (, cX)
		//     /______\     /__\/__\      \/
		int w = width / 2;
		int h = height / 2;
		int l = (left + centerX) / 2;
		int t = top + h;
		int r = l + w;
		//int b = bottom;
		//int cx = centerX;

		m_triangleWidget->recalculatePositions(w, h, BarycentricTriangle(l, t, r, t, centerX, bottom));
	}

	{ // Resize histograms
		m_histograms[0]->resize(width, HISTOGRAM_HEIGHT);
		m_histograms[1]->resize(width, HISTOGRAM_HEIGHT);
		m_histograms[2]->resize(width, HISTOGRAM_HEIGHT);
	}

	{ // Transform histogram A (left)
		m_transformHistogramA.reset();
		m_transformHistogramA.translate(left - TRIANGLE_LEFT, bottom - TRIANGLE_TOP);
		m_transformHistogramA.rotate(-60.0);
	}

	{ // Transform histogram B (right)
		m_transformHistogramB.reset();
		m_transformHistogramB.translate(centerX + TRIANGLE_LEFT, top - TRIANGLE_TOP);
		m_transformHistogramB.rotate(60.0);
	}

	{ // Transform histogram C (bottom)
		m_transformHistogramC.reset();
		m_transformHistogramC.translate(left, bottom);
	}

	{ // Transform slicer A (left histogram)

	}

	{ // Transform slicer B (right histogram)

	}

	{ // Transform slicer C (bottom histogram)

	}
}

void iAHistogramTriangle::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	paintSlicers(p);
	paintTriangle(p);
	paintHistograms(p);

	QPen pen;
	pen.setWidth(1);
	p.setPen(pen);

	pen.setColor(QColor(0, 0, 255));

	p.setTransform(m_transformHistogramA);
	p.drawRect(0, 0, width, HISTOGRAM_HEIGHT);

	p.setTransform(m_transformHistogramB);
	p.drawRect(0, 0, width, HISTOGRAM_HEIGHT);

	p.setTransform(m_transformHistogramC);
	p.drawRect(0, 0, width, HISTOGRAM_HEIGHT);
}

void iAHistogramTriangle::paintSlicers(QPainter &p)
{

}

void iAHistogramTriangle::paintTriangle(QPainter &p)
{
	m_triangleWidget->paintTriangleBorder(p);
	//m_triangleWidget->paintTriangleFill(p);
	m_triangleWidget->paintContext(p);
	m_triangleWidget->paintControlPoint(p);
}

void iAHistogramTriangle::paintHistograms(QPainter &p)
{
	p.setTransform(m_transformHistogramA);
	m_histograms[0]->render(&p);
	p.resetTransform();
}