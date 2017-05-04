/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAQtCaptionWidget.h"

#include "iAGEMSeConstants.h"

#include <QPainter>
#include <QVBoxLayout>

// TODO: replace with QLabel!

iAQtCaptionWidget::iAQtCaptionWidget(QWidget* parent, QString const & name):
	QWidget(parent),
	m_name(name),
	m_height(CaptionHeight)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setFixedHeight(m_height);
}
void iAQtCaptionWidget::paintEvent(QPaintEvent* ev)
{
	QPainter painter(this);
	QRect r(geometry());
	painter.fillRect(0, 0, r.width(), r.height(), DefaultColors::CaptionBrush);
	painter.setPen(DefaultColors::CaptionFontColor);
	QFont f(painter.font());
	f.setPointSize(FontSize);
	painter.setFont(f);
	QFontMetrics fm(painter.font());
	painter.drawText(10, m_height-5, m_name);
}


void SetCaptionedContent(QWidget* parent, QString const & caption, QWidget* w)
{
	QVBoxLayout	* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(new iAQtCaptionWidget(parent, caption));
	mainLayout->addWidget(w);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->setAlignment(Qt::AlignTop);
	parent->setLayout(mainLayout);
}