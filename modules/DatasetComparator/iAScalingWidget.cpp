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


const int nonlinearBarStartPosY = 10;
const int nonlinearBarHeight = 10;
const int linearBarHeight = 10;
const int barSpacing = 50;
const int linearBarStartPosY = nonlinearBarStartPosY + nonlinearBarHeight + barSpacing;
const int widgetHeight = nonlinearBarStartPosY + nonlinearBarHeight +
	barSpacing + linearBarHeight + nonlinearBarStartPosY;

iAScalingWidget::iAScalingWidget(QWidget* parent) :
	QOpenGLWidget(parent),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_linearBarCursorPos(0),
	m_nonlinearBarCursorPos(0)
{
	setFixedHeight(widgetHeight);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);

	// For multisampling
	QSurfaceFormat format = QSurfaceFormat();
	format.setSamples(4);
	this->setFormat(format);
}

QSize iAScalingWidget::sizeHint() const
{
	return QSize(2560, widgetHeight);
}

QSize iAScalingWidget::minimumSizeHint() const
{
	return QSize(400, widgetHeight);
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

void iAScalingWidget::setRange(double lowerIdx, double upperIdx, double nonlinearLowerRest, 
	double nonlinearUpperRest, double linearLowerRest, double linearUpperRest)
{
	m_nonlinearLowerIdx = lowerIdx;
	m_nonlinearUpperIdx = upperIdx;
	m_nonlinearLowerRest = nonlinearLowerRest;
	m_nonlinearUpperRest = nonlinearUpperRest;
	m_linearLowerRest = linearLowerRest;
	m_linearUpperRest = linearUpperRest;
}

void iAScalingWidget::setBkgrdThrRanges(QList<QCPRange> bkgrdRangeList)
{
	m_bkgrdRangeList = bkgrdRangeList;
}

void iAScalingWidget::setSelection(QCPDataSelection sel)
{
	m_sel = sel;
}

void iAScalingWidget::initializeGL()
{
	glClearColor(1.0, 1.0, 1.0, 1.0);
}

void iAScalingWidget::paintGL()
{
	// TODO: whole widget is always painted (not needed if only red line moves)? 
	if (m_nonlinearScalingVec.size() == 0)
		return;

	QPainter painter(this);
	painter.setClipRegion(QRegion(m_nonlinearAxis->axisRect()->left(),
		nonlinearBarStartPosY, m_nonlinearAxis->axisRect()->width(),
		nonlinearBarHeight + barSpacing + linearBarHeight));
	double rgb[3]; QColor c;
	int leftOffset = m_nonlinearAxis->axisRect()->left();
	double linearBarStartPosX = 0, nonlinearBarStartPosX = 0, nonlinearBarWidth, linearBarWidth,
		linearScalingFactor = m_nonlinearAxis->axisRect()->width() /
		(m_nonlinearUpperIdx-1 + m_linearUpperRest - (m_nonlinearLowerIdx + m_linearLowerRest)),
		nonlinearScalingFactor = m_nonlinearAxis->axisRect()->width() /
		(m_nonlinearScalingVec[m_nonlinearUpperIdx] - m_nonlinearUpperRest -
		(m_nonlinearScalingVec[m_nonlinearLowerIdx] + m_nonlinearLowerRest));

	m_prevNonlinearBarStartPosX = 0.0, m_prevLinearBarStartPosX = 0.0;
	for (int hIdx = m_nonlinearLowerIdx + 1; hIdx <= m_nonlinearUpperIdx; ++hIdx)
	{
		if (hIdx == m_nonlinearLowerIdx + 1)
		{
			nonlinearBarWidth = (m_nonlinearScalingVec[hIdx] - 
				(m_nonlinearScalingVec[hIdx-1] + m_nonlinearLowerRest)) * nonlinearScalingFactor;
			linearBarWidth = (m_nonlinearLowerIdx + 1 - m_nonlinearLowerIdx - m_linearLowerRest) *
				linearScalingFactor;
		}
		else if (hIdx == m_nonlinearUpperIdx)
		{
			nonlinearBarWidth = ((m_nonlinearScalingVec[hIdx] - m_nonlinearUpperRest) -
				m_nonlinearScalingVec[hIdx-1]) * nonlinearScalingFactor;
			linearBarWidth = m_linearUpperRest * linearScalingFactor;
		}
		else
		{
			nonlinearBarWidth = (m_nonlinearScalingVec[hIdx] - 
				m_nonlinearScalingVec[hIdx-1]) * nonlinearScalingFactor;
			linearBarWidth = linearScalingFactor;
		}

		painter.setRenderHint(QPainter::Antialiasing, false);
		m_lut->GetColor(m_impFunctVec[hIdx], rgb);
		c.setRgbF(rgb[0], rgb[1], rgb[2]);
		painter.setBrush(QBrush(c));
		painter.setPen(QPen(c));
		painter.drawRect(leftOffset + nonlinearBarStartPosX, nonlinearBarStartPosY,
			nonlinearBarWidth, nonlinearBarHeight);
		painter.setRenderHint(QPainter::Antialiasing, true);
		
		nonlinearBarStartPosX += nonlinearBarWidth;
		linearBarStartPosX += linearBarWidth;

		QLinearGradient gradient(
			leftOffset + m_prevLinearBarStartPosX, 20,
			leftOffset + m_prevLinearBarStartPosX, 70);
		gradient.setColorAt(0.0, c);
		gradient.setColorAt(1.0, Qt::lightGray);
		painter.setBrush(gradient);
		painter.setPen(Qt::NoPen);

		QPainterPath path;
		path.moveTo(QPoint(leftOffset + m_prevNonlinearBarStartPosX, 
			nonlinearBarStartPosY + nonlinearBarHeight));
		path.lineTo(QPoint(leftOffset + nonlinearBarStartPosX, 
			nonlinearBarStartPosY + nonlinearBarHeight));
		path.lineTo(QPoint(leftOffset + linearBarStartPosX, linearBarStartPosY));
		path.lineTo(QPoint(leftOffset + m_prevLinearBarStartPosX, linearBarStartPosY));
		path.lineTo(QPoint(leftOffset + m_prevNonlinearBarStartPosX, 
			nonlinearBarStartPosY + nonlinearBarHeight));
		painter.drawPath(path);

		m_prevNonlinearBarStartPosX = nonlinearBarStartPosX;
		m_prevLinearBarStartPosX = linearBarStartPosX;
	}
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setBrush(QBrush(Qt::lightGray));
	painter.setPen(QPen(Qt::lightGray));
	painter.drawRect(leftOffset, linearBarStartPosY, linearBarStartPosX, linearBarHeight);
	painter.setRenderHint(QPainter::Antialiasing, true);

	painter.setPen(QPen(Qt::red, 1.5));
	painter.drawLine(m_nonlinearBarCursorPos, nonlinearBarStartPosY, m_nonlinearBarCursorPos, 
		nonlinearBarStartPosY + nonlinearBarHeight);
	painter.drawLine(m_nonlinearBarCursorPos, nonlinearBarStartPosY + nonlinearBarHeight, 
		m_linearBarCursorPos, linearBarStartPosY);
	painter.drawLine(m_linearBarCursorPos, linearBarStartPosY, m_linearBarCursorPos, 
		linearBarStartPosY + linearBarHeight);

	if (!m_sel.isEmpty())
	{
		double linearStart, linearWidth, nonlinearStart, nonlinearWidth;
		for (auto range : m_sel.dataRanges())
		{
			linearStart = leftOffset + (range.begin() - (m_nonlinearLowerIdx + m_linearLowerRest)) * 
				linearScalingFactor;
			linearWidth = (range.end() - 1 - range.begin()) * linearScalingFactor;
			nonlinearStart = leftOffset + (m_nonlinearScalingVec[range.begin()] -
				(m_nonlinearScalingVec[m_nonlinearLowerIdx] + m_nonlinearLowerRest)) * 
				nonlinearScalingFactor;
			nonlinearWidth = (m_nonlinearScalingVec[range.end() - 1] -
				m_nonlinearScalingVec[range.begin()]) * nonlinearScalingFactor;
			painter.setBrush(Qt::NoBrush);
			painter.drawRect(nonlinearStart, nonlinearBarStartPosY, nonlinearWidth, nonlinearBarHeight);
			painter.drawRect(linearStart, linearBarStartPosY, linearWidth, linearBarHeight);
			painter.drawLine(nonlinearStart, nonlinearBarStartPosY + nonlinearBarHeight, 
				linearStart, linearBarStartPosY);
			painter.drawLine(nonlinearStart + nonlinearWidth, nonlinearBarStartPosY + nonlinearBarHeight,
				linearStart + linearWidth, linearBarStartPosY);
		}
	}
}