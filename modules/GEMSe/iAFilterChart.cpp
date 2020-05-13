/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAFilterChart.h"

#include "iAParamHistogramData.h"

#include <charts/iAPlotTypes.h>
#include <iAMapper.h>
#include <iAMathUtility.h>
#include <iANameMapper.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

namespace
{
	const double InvalidMarker = std::numeric_limits<double>::lowest();
}

iAFilterChart::iAFilterChart(QWidget* parent,
	QString const & caption,
	QSharedPointer<iAParamHistogramData> data,
	QSharedPointer<iANameMapper> nameMapper,
	bool showCaption)
:
	iAChartWidget(parent, caption, ""),
	m_data(data),
	m_nameMapper(nameMapper),
	m_markedLocation(InvalidMarker),
	m_selectedHandle(-1),
	m_selectionOffset(0)
{
	addPlot(GetDrawer(m_data, DefaultColors::AllDataChartColor));
	m_minSliderPos = m_data->mapBinToValue(0);
	m_maxSliderPos = m_data->mapBinToValue(m_data->numBin());
	setCaptionPosition(Qt::AlignLeft | Qt::AlignTop);
	setShowXAxisLabel(showCaption);
	for (size_t i = 0; i < m_data->numBin(); ++i)
	{
		m_binColors.push_back(QColor(0, 0, 0, 0));
	}
}

double iAFilterChart::mapBinToValue(double bin) const
{
	return m_data->mapBinToValue(bin);
}

double iAFilterChart::mapValueToBin(double value) const
{
	return m_data->mapValueToBin(value);
}

QSharedPointer<iAPlot> iAFilterChart::GetDrawer(QSharedPointer<iAParamHistogramData> newData, QColor color)
{
	return (newData->valueType() == Categorical ||
		(newData->valueType() == Discrete && ((newData->xBounds()[1]- newData->xBounds()[0])  <= newData->numBin())))
		? QSharedPointer<iAPlot>(new iABarGraphPlot(newData, color, 2))
		: QSharedPointer<iAPlot>(new iAFilledLinePlot(newData, color));
}

void iAFilterChart::drawMarker(QPainter & painter, double markerLocation, QPen const & pen, QBrush const & brush)
{
	double diagX = xMapper().srcToDst(markerLocation)-m_translationX;
	QPolygon poly;
	const int MarkerTop = 0;
	poly.append(QPoint(diagX, MarkerTop));
	poly.append(QPoint(diagX+MarkerTriangleWidthHalf, MarkerTop+MarkerTriangleHeight));
	poly.append(QPoint(diagX-MarkerTriangleWidthHalf, MarkerTop+MarkerTriangleHeight));
	QPainterPath path;
	path.addPolygon(poly);
	painter.setPen(pen);
	painter.drawPolygon(poly);
	painter.fillPath(path, brush);
	/*
	// alternatively: pie:
	QRectF rectangle1( diagX - SliderPieSize, MarkerTop - SliderPieSize, 2*SliderPieSize, 2*SliderPieSize );
	painter.drawPie( rectangle1, -120 * 16, 60 * 16 );
	*/
}

void iAFilterChart::drawAxes(QPainter& painter)
{
	// draw bin colors: m_binColors
	{
		double y1 =  0;
		double y2 =  ChartColoringHeight;
		int binWidth = static_cast<int>(std::ceil(mapValue(0.0, static_cast<double>(m_data->numBin()), 0.0, chartWidth()*m_xZoom, static_cast<double>(1))));
		for (size_t b = 0; b < m_data->numBin(); ++b)
		{
			if (m_binColors[b].alpha() == 0)
			{
				continue;
			}
			double x1 = mapValue(0.0, static_cast<double>(m_data->numBin()), 0.0, chartWidth()*m_xZoom, static_cast<double>(b));

			QRect rect(x1, y1, binWidth, y2);
			painter.fillRect(rect, m_binColors[b]);
		}
	}

	if (m_markedLocation != InvalidMarker)
	{
		drawMarker(painter, m_markedLocation, DefaultColors::ChartMarkerPen, DefaultColors::ChartMarkerBrush);
	}
	drawMarker(painter, m_minSliderPos, DefaultColors::ChartSliderPen, DefaultColors::ChartSliderBrush);
	drawMarker(painter, m_maxSliderPos, DefaultColors::ChartSliderPen, DefaultColors::ChartSliderBrush);

	iAChartWidget::drawAxes(painter);
}

void iAFilterChart::SetBinColor(int bin, QColor const & color)
{
	assert(bin >= 0 && static_cast<size_t>(bin) < m_data->numBin());
	m_binColors[bin] = color;
}

void iAFilterChart::SetMarker(double value)
{
	m_markedLocation = value;
	update();
}

void iAFilterChart::RemoveMarker()
{
	SetMarker(InvalidMarker);
}

iAValueType iAFilterChart::GetRangeType() const
{
	return m_data->valueType();
}

double iAFilterChart::GetMinVisibleBin() const
{
	double minVisXBin = mapValue(0.0, chartWidth()*xZoom(), 0.0, static_cast<double>(m_data->numBin()), static_cast<double>(-m_translationX));
	return minVisXBin;
}

double iAFilterChart::GetMaxVisibleBin() const
{
	double maxVisXBin = mapValue(0.0, chartWidth()*xZoom(), 0.0, static_cast<double>(m_data->numBin()), static_cast<double>(chartWidth()-m_translationX));
	return maxVisXBin;
}

QString iAFilterChart::xAxisTickMarkLabel(double value, double stepWidth)
{
	if (plots().size() > 0 && plots()[0]->data()->valueType() == Categorical)
	{
		return (m_nameMapper && value < m_nameMapper->size()) ? m_nameMapper->name(static_cast<int>(value)): "";
	}
	return iAChartWidget::xAxisTickMarkLabel(value, stepWidth);
}

void iAFilterChart::contextMenuEvent(QContextMenuEvent * /*event*/)
{
	// disable context menu
}

void iAFilterChart::mousePressEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::LeftButton )
	{
		// horizontal + vertical panning:
		if ( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
		{
			m_translationStartX = m_translationX;
			m_translationStartY = m_translationY;
			iAChartWidget::changeMode( MOVE_VIEW_MODE, event );
			return;
		}

		if ( event->y() > geometry().height() - bottomMargin() - m_translationY
			  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			// check if we hit min or max handle:
			int x = event->x() - leftMargin();

			int minX = xMapper().srcToDst(m_minSliderPos);
			int maxX = xMapper().srcToDst(m_maxSliderPos);
			if ( abs(x-minX) <= MarkerTriangleWidthHalf)
			{
				m_selectedHandle = 0;
				m_selectionOffset = minX - x;
			}
			else if ( abs(x-maxX) <= MarkerTriangleWidthHalf)
			{
				m_selectedHandle = 1;
				m_selectionOffset = maxX - x;
			}
		}
	}
	iAChartWidget::mousePressEvent(event);
}

void iAFilterChart::mouseReleaseEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::LeftButton )
	{
		if (m_selectedHandle != -1)
		{
			emit selectionChanged();
			m_selectedHandle = -1;
			return;
		}
	}
	iAChartWidget::mouseReleaseEvent( event );
}

void iAFilterChart::mouseMoveEvent( QMouseEvent *event )
{
	if (	( event->buttons() == Qt::LeftButton ) &&
			( event->y() > geometry().height() - bottomMargin() - m_translationY			// mouse event below X-axis
			  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) ) &&
			  m_selectedHandle != -1)
	{
		int x = event->x() - leftMargin() + m_selectionOffset;
		if (x < 0 || x-m_translationX > static_cast<int>(chartWidth()*xZoom()) )
		{
			return;
		}
		double value = xMapper().dstToSrc(x);

		// snap to next valid value
		if (GetRangeType() == Categorical)
		{
			value = mapBinToValue(std::round(mapValueToBin(value)));
		}
		if (GetRangeType() == Discrete)
		{
			value = std::round(value);
		}
		switch (m_selectedHandle)
		{
			case 0:	m_minSliderPos = value;	break;
			case 1: m_maxSliderPos = value;	break;
			default: assert(false); break;
		}
		QString text( tr( "%1\n(data range: [%2..%3])" )
				  .arg( value )
				  .arg( m_data->xBounds()[0] )
				  .arg( m_data->xBounds()[1] )
		);
		QToolTip::showText( event->globalPos(), text, this );
		update();
		return;
	}
	else if ((event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
	{
		iAChartWidget::mouseMoveEvent( event );
	}
}

void iAFilterChart::SetMinMaxSlider(double min, double max)
{
	m_minSliderPos = min;
	m_maxSliderPos = max;
	update();
}

double iAFilterChart::GetMinSliderPos()
{
	return m_minSliderPos;
}

double iAFilterChart::GetMaxSliderPos()
{
	return m_maxSliderPos;
}
