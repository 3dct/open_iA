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
 
#include "pch.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "iAPieChartWidget.h"

#include <QPainter>

#include <algorithm>

namespace
{
	const int Margin = 30;
}

iAPiePiece::iAPiePiece():
	percentage(0.0)
{}

iAPiePiece::iAPiePiece(QString const & n, double p, QColor & c):
	name(n), percentage(p), color(c)
{
}

iAPieChartWidget::iAPieChartWidget(QWidget *parent):
	QWidget(parent),
	m_pieces(0)
{
	setMinimumSize(150, 150);
}

void iAPieChartWidget::paintEvent(QPaintEvent * e)
{
	QWidget::paintEvent(e);

	if (m_pieces.empty())
	{
		return;
	}

	QPainter painter(this);

	int pieWidth  = this->width() - 2*Margin;
	int pieHeight = this->height() - 2*Margin;
	int diameter  = std::min(pieWidth, pieHeight);
	int radius    = diameter / 2.0;

	int pieCenterX = this->width() / 2;
	int pieCenterY = this->height() / 2;

	double curStartAngle = 0;
	for (QVector<iAPiePiece>::const_iterator it = m_pieces.begin();
		it != m_pieces.end(); ++it)
	{
		double alpha = M_PI * (curStartAngle + it->percentage*1.8) / 180;
		int xSign = 1;
		int ySign = -1;
		if (alpha >= 90 && alpha < 180)
		{
			alpha -= 90;
			xSign = -1;
			ySign = -1;
		} else if (alpha >= 180 && alpha < 270)
		{
			alpha -= 180;
			xSign = -1;
			ySign = 1;
		} else if (alpha >= 270) {
			alpha -= 270;
			ySign = 1;
		}
		int xFromCenter = (cos(alpha) * radius / 1.5);
		int yFromCenter = (sin(alpha) * radius / 1.5);
		int textX = pieCenterX + (xSign) * xFromCenter;
		int textY = pieCenterY + (ySign) * yFromCenter;

		painter.setPen(it->color);
		QColor brushColor(it->color);
		brushColor.setAlpha(brushColor.alpha()/4);
		painter.setBrush(brushColor);

		painter.drawPie(pieCenterX-radius, pieCenterY-radius, diameter, diameter, curStartAngle * 16, (it->percentage*3.6) * 16);
		
		QFont myFont;
		QFontMetrics fm(myFont);
		int width=fm.width(it->name);
		int height=fm.height();
		QRect textRect(textX-width/2, textY-height/2, width, height);

		QRect textBorder(textRect);
		textBorder.adjust(-2, -1, +1, +1);
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(textBorder);

		QTextOption o;
		o.setAlignment(Qt::AlignCenter);
		painter.setPen((painter.background().color() == Qt::black) ? Qt::white : Qt::black);
		painter.drawText(textRect, it->name, o);

		curStartAngle += it->percentage*3.6;
	}
}

void iAPieChartWidget::clearPieces()
{
	m_pieces.clear();
}


void iAPieChartWidget::addPiece(QString name, double percentage, QColor color)
{
	m_pieces.push_back(iAPiePiece(name, percentage, color));
}

bool iAPieChartWidget::empty() const
{
	return m_pieces.empty();
}
