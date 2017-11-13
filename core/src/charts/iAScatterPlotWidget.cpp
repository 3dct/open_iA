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
#include "iAScatterPlotWidget.h"

#include "iALookupTable.h"
#include "iAScatterPlot.h"
#include "iAScatterPlotSelectionHandler.h"
#include "iASPLOMData.h"

#include <vtkLookupTable.h>
#include <vtkSmartPointer.h>

#include <QMouseEvent>

class iAScatterPlotStandaloneHandler : public iAScatterPlotSelectionHandler
{
public:
	virtual QVector<unsigned int> & getSelection()
	{
		return m_selection;
	}
	void setSelection(QVector<unsigned int> const & selection)
	{
		m_selection = selection;
	}
	virtual const QList<int> & getHighlightedPoints() const
	{
		return m_highlight;
	}
	virtual int getVisibleParametersCount() const
	{
		return 2;
	}
	virtual double getAnimIn() const
	{
		return 1.0;
	}
	virtual double getAnimOut() const
	{
		return 0.0;
	}
private:
	QList<int> m_highlight;
	QVector<unsigned int> m_selection;
};


const int iAScatterPlotWidget::PaddingLeft = 45;
const int iAScatterPlotWidget::PaddingTop = 5;
const int iAScatterPlotWidget::PaddingRight = 5;
const int iAScatterPlotWidget::PaddingBottom = 45;
const int iAScatterPlotWidget::TextPadding = 2;


iAScatterPlotWidget::iAScatterPlotWidget(QSharedPointer<iASPLOMData> data) :
	m_data(data),
	m_scatterPlotHandler(new iAScatterPlotStandaloneHandler())
{
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	m_scatterplot = new iAScatterPlot(m_scatterPlotHandler.data(), this);
	m_scatterplot->setData(0, 1, data);
}

void iAScatterPlotWidget::SetPlotColor(QColor const & c, double rangeMin, double rangeMax)
{
	auto lut = vtkSmartPointer<vtkLookupTable>::New();
	double lutRange[2] = { rangeMin, rangeMax };
	lut->SetRange(lutRange);
	lut->Build();
	vtkIdType lutColCnt = lut->GetNumberOfTableValues();
	for (vtkIdType i = 0; i < lutColCnt; i++)
	{
		double rgba[4]; lut->GetTableValue(i, rgba);
		rgba[0] = c.red() / 255.0;
		rgba[1] = c.green() / 255.0;
		rgba[2] = c.blue() / 255.0;
		rgba[3] = c.alpha() / 255.0;
		lut->SetTableValue(i, rgba);
	}
	lut->Build();
	QSharedPointer<iALookupTable> lookupTable(new iALookupTable(lut.GetPointer()));
	m_scatterplot->setLookupTable(lookupTable, m_data->parameterName(0));
}

void iAScatterPlotWidget::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);
	painter.beginNativePainting();
	QColor bgColor(QWidget::palette().color(QWidget::backgroundRole()));
	QColor fg(QWidget::palette().color(QPalette::Text));
	m_scatterplot->settings.tickLabelColor = fg;
	glClearColor(bgColor.red() / 255.0, bgColor.green() / 255.0, bgColor.blue() / 255.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	painter.endNativePainting();
	m_scatterplot->paintOnParent(painter);

	// print axes tick labels:
	painter.save();
	QList<double> ticksX, ticksY; QList<QString> textX, textY;
	m_scatterplot->printTicksInfo(&ticksX, &ticksY, &textX, &textY);
	painter.setPen(m_scatterplot->settings.tickLabelColor);
	QPoint tOfs(45, 45);
	long tSpc = 5;
	for (long i = 0; i < ticksY.size(); ++i)
	{
		double t = ticksY[i]; QString text = textY[i];
		painter.drawText(QRectF(0, t - tOfs.y(), tOfs.x() - tSpc, 2 * tOfs.y()), Qt::AlignRight | Qt::AlignVCenter, text);
	}
	painter.rotate(-90);
	for (long i = 0; i < ticksX.size(); ++i)
	{
		double t = ticksX[i]; QString text = textX[i];
		painter.drawText(QRectF(-tOfs.y() + tSpc + PaddingBottom - height() - TextPadding, t - tOfs.x(), tOfs.y() - tSpc, 2 * tOfs.x()), Qt::AlignRight | Qt::AlignVCenter, text);
	}
	painter.restore();

	QFontMetrics fm = painter.fontMetrics();
	// print axes labels:
	painter.save();
	painter.setPen(m_scatterplot->settings.tickLabelColor);
	painter.drawText(QRectF(0, height() - fm.height() - TextPadding, width(), fm.height()), Qt::AlignHCenter | Qt::AlignTop, m_data->parameterName(0));
	painter.rotate(-90);
	painter.drawText(QRectF(-height(), 0, height(), fm.height()), Qt::AlignCenter | Qt::AlignTop, m_data->parameterName(1));
	painter.restore();
}

void iAScatterPlotWidget::resizeEvent(QResizeEvent* event)
{
	QRect size(geometry());
	size.moveTop(0);
	size.moveLeft(0);
	size.adjust(PaddingLeft, PaddingTop, -PaddingRight, -PaddingBottom);
	if (size.width() > 0 && size.height() > 0)
	{
		m_scatterplot->setRect(size);
	}
}

void iAScatterPlotWidget::wheelEvent(QWheelEvent * event)
{
	if (event->x() >= PaddingLeft && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom))
	{
		m_scatterplot->SPLOMWheelEvent(event);
		update();
	}
}

void iAScatterPlotWidget::mousePressEvent(QMouseEvent * event)
{
	if (event->x() >= PaddingLeft && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom))
	{
		m_scatterplot->SPLOMMousePressEvent(event);
	}
}

void iAScatterPlotWidget::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->x() >= PaddingLeft && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom))
	{
		m_scatterplot->SPLOMMouseReleaseEvent(event);
		update();
	}
}

void iAScatterPlotWidget::mouseMoveEvent(QMouseEvent * event)
{
	if (event->x() >= PaddingLeft && event->x() <= (width() - PaddingRight) &&
		event->y() >= PaddingTop && event->y() <= (height() - PaddingBottom))
	{
		m_scatterplot->SPLOMMouseMoveEvent(event);
	}
}

void iAScatterPlotWidget::keyPressEvent(QKeyEvent * event)
{
	switch (event->key())
	{
	case Qt::Key_R: //if R is pressed, reset all the applied transformation as offset and scaling
		m_scatterplot->setTransform(1.0, QPointF(0.0f, 0.0f));
		break;
	}
}

QVector<unsigned int> iAScatterPlotWidget::GetSelection()
{
	return m_scatterPlotHandler->getSelection();
}

void iAScatterPlotWidget::SetSelection(QVector<unsigned int> const & selection)
{
	m_scatterPlotHandler->setSelection(selection);
}

void iAScatterPlotWidget::SetSelectionColor(QColor const & c)
{
	m_scatterplot->settings.selectionColor = c;
}
