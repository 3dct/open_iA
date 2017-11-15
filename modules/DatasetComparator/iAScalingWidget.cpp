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

#include "iAScalingWidget.h"
#include "iAPerceptuallyUniformLUT.h"

#include <vtkLookupTable.h>

#include <QPainter>


iAScalingWidget::iAScalingWidget(QWidget* parent) :
	QWidget(parent),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_normalText("Normal"),
	m_nonlinearText("Nonlinear"),
	m_linearBarCursorPos(0),
	m_nonlinearBarCursorPos(0)
{
	setFixedHeight(90);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
}

void iAScalingWidget::setNonlinearAxis(QCPAxis* nla)
{
	m_nonlinearAxis = nla;
}

void iAScalingWidget::setNonlinearScalingVector(QVector<double> nlsv, QVector<double> impfv)
{
	m_nonlinearScalingVec = nlsv;
	m_impFunctVec = impfv;
	iAPerceptuallyUniformLUT::BuildLinearLUT(m_lut, 0.0, 1.0, 256);
}

void iAScalingWidget::setCursorPositions(double lcp, double nlcp)
{
	m_linearBarCursorPos = lcp;
	m_nonlinearBarCursorPos = nlcp;
	update();
}

void iAScalingWidget::paintEvent(QPaintEvent * /* event */)
{
	if (m_nonlinearScalingVec.size() == 0)
		return;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setPen(QPen(Qt::black));
	painter.drawStaticText(5, 10, m_normalText);
	painter.drawStaticText(5, 70, m_nonlinearText);

	int leftOffset = m_nonlinearAxis->axisRect()->left();
	double linearBarStartPos = 0.0, nonlinearBarStartPos = 0.0, 
		linearBarWidth = m_nonlinearAxis->axisRect()->width() / (double) m_nonlinearScalingVec.size();
	double scalingFactor = m_nonlinearAxis->axisRect()->width() /
		(m_nonlinearScalingVec.last() - m_nonlinearScalingVec.first());
	double rgb[3]; QColor c;

	for (int hIdx = 1; hIdx < m_nonlinearScalingVec.size(); ++hIdx)
	{
		double nonlinearBarWidth = (m_nonlinearScalingVec[hIdx] - m_nonlinearScalingVec[hIdx - 1]) * scalingFactor;
		QRect nonlinearBar(leftOffset + nonlinearBarStartPos, 10, nonlinearBarWidth, 10);
		m_lut->GetColor(1- m_impFunctVec[hIdx], rgb);
		c.setRgbF(rgb[0], rgb[1], rgb[2]);
		painter.setBrush(QBrush(c));
		painter.setPen(QPen(c));
		painter.drawRect(nonlinearBar);
		nonlinearBarStartPos += nonlinearBarWidth;

		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.drawLine(leftOffset + nonlinearBarStartPos, 20, leftOffset + linearBarStartPos, 70);
		painter.setRenderHint(QPainter::Antialiasing, false);

		QRect linearBar(leftOffset + linearBarStartPos, 70, linearBarWidth, 10);
		//m_lut->GetColor(hIdx, rgb);
		//c.setRgbF(rgb[0], rgb[1], rgb[2]);
		painter.setBrush(QBrush(Qt::lightGray));
		painter.setPen(QPen(Qt::lightGray));
		painter.drawRect(linearBar);
		linearBarStartPos += linearBarWidth;
	}
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(QPen(Qt::red));
	painter.drawLine(m_nonlinearBarCursorPos, 10, m_nonlinearBarCursorPos, 20);
	painter.drawLine(m_nonlinearBarCursorPos, 20, m_linearBarCursorPos, 70);
	painter.drawLine(m_linearBarCursorPos, 70, m_linearBarCursorPos, 80);
}