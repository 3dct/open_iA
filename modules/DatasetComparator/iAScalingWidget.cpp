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

QSize iAScalingWidget::sizeHint() const
{
	return QSize(2560, 90);
}

QSize iAScalingWidget::minimumSizeHint() const
{
	return QSize(400, 90);
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

void iAScalingWidget::setRange(double lower, double upper, double lowerRest, double upperRest, double linearLowerRest, double linearUpperRest)
{
	m_nonlinearLower = lower;
	m_nonlinearUpper = upper;
	m_rangeLowerRest = lowerRest;
	m_rangeUpperRest = upperRest;
	m_linearLowerRest = linearLowerRest;
	m_linearUpperRest = linearUpperRest;
}

void iAScalingWidget::paintEvent(QPaintEvent * /* event */)
{
	// TODO: whole widget is always painted (not needed if only red line moves)? 
	if (m_nonlinearScalingVec.size() == 0)
		return;

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(QPen(Qt::black));
	painter.drawStaticText(5, 10, m_nonlinearText);
	painter.drawStaticText(5, 70, m_normalText);

	double rgb[3]; QColor c;
	int leftOffset = m_nonlinearAxis->axisRect()->left();
	double linearBarStartPos = 0.0, nonlinearBarStartPos = 0.0, nonlinearBarWidth, linearBarWidth,
		linearScalingFactor = m_nonlinearAxis->axisRect()->width() /
		(m_nonlinearUpper-1 + m_linearUpperRest - (m_nonlinearLower + m_linearLowerRest)),
		nonlinearscalingFactor = m_nonlinearAxis->axisRect()->width() /
		(m_nonlinearScalingVec[m_nonlinearUpper] - m_rangeUpperRest - (m_nonlinearScalingVec[m_nonlinearLower] + m_rangeLowerRest));

	for (int hIdx = m_nonlinearLower + 1; hIdx <= m_nonlinearUpper; ++hIdx)
	{
		if (hIdx == m_nonlinearLower + 1)
		{
			nonlinearBarWidth = (m_nonlinearScalingVec[hIdx] - 
				(m_nonlinearScalingVec[hIdx-1] + m_rangeLowerRest)) * nonlinearscalingFactor;
			linearBarWidth = (m_nonlinearLower + 1 - m_nonlinearLower - m_linearLowerRest) * linearScalingFactor;
		}
		else if (hIdx == m_nonlinearUpper)
		{
			nonlinearBarWidth = (m_nonlinearScalingVec[hIdx] - 
				(m_nonlinearScalingVec[hIdx-1] + m_rangeUpperRest)) * nonlinearscalingFactor;
			linearBarWidth = m_linearUpperRest * linearScalingFactor;
		}
		else
		{
			nonlinearBarWidth = (m_nonlinearScalingVec[hIdx] - 
				m_nonlinearScalingVec[hIdx-1]) * nonlinearscalingFactor;
			linearBarWidth = linearScalingFactor;
		}

		QRectF nonlinearBar(leftOffset + nonlinearBarStartPos, 10.0, nonlinearBarWidth, 10.0);
		m_lut->GetColor(1 - m_impFunctVec[hIdx], rgb);
		c.setRgbF(rgb[0], rgb[1], rgb[2]);
		painter.setBrush(QBrush(c));
		painter.setPen(QPen(Qt::black));
		painter.drawRect(nonlinearBar);
		
		nonlinearBarStartPos += nonlinearBarWidth;
		linearBarStartPos += linearBarWidth;

		if (hIdx < m_nonlinearUpper)
			painter.drawLine(leftOffset + nonlinearBarStartPos, 20.0, leftOffset + linearBarStartPos, 70.0);
	}

	QRectF linearBar(leftOffset + 0.0, 70, linearBarStartPos, 10);
	painter.setBrush(QBrush(Qt::lightGray));
	painter.setPen(QPen(Qt::lightGray));
	painter.drawRect(linearBar);

	painter.setPen(QPen(Qt::red));
	painter.drawLine(m_nonlinearBarCursorPos, 10, m_nonlinearBarCursorPos, 20);
	painter.drawLine(m_nonlinearBarCursorPos, 20, m_linearBarCursorPos, 70);
	painter.drawLine(m_linearBarCursorPos, 70, m_linearBarCursorPos, 80);
}