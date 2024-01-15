// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATriangleButton.h"

#include "iAGEMSeConstants.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

iATriangleButton::iATriangleButton():
	m_expanded(false)
{
	setStyleSheet("background-color: transparent;");
}

void iATriangleButton::paintEvent(QPaintEvent* /*ev*/)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	QRect g(geometry());
	p.setPen(Qt::black);

	int partX = g.width()/4;
	int partY = partX*2;

	p.translate(g.width()/2, g.height()/2);
	if (m_expanded)
	{
		p.rotate(45);
	}

	QPolygon poly;
	poly<< QPoint(-partX, -partY)
		<< QPoint(partX, 0)
		<< QPoint(-partX,  partY);

	if (m_expanded)
	{
		QPainterPath path;
		path.addPolygon(poly);
		p.fillPath(path, DefaultColors::TriangleButtonSelectedBrush);
	}
	p.setPen(DefaultColors::TriangleButtonPen);
	p.drawPolygon(poly);
}

void iATriangleButton::mouseReleaseEvent(QMouseEvent * ev)
{
	if (ev->button() == Qt::LeftButton)
	{
		Toggle();
		emit clicked();
	}
}


bool iATriangleButton::Toggle()
{
	m_expanded = !m_expanded;
	update();
	return m_expanded;
}

bool iATriangleButton::IsExpanded() const
{
	return m_expanded;
}
