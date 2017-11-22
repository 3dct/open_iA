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
#include "iAFilterChart.h"

#include "iAParamHistogramData.h"
#include "charts/iAPlotTypes.h"
#include "iAMathUtility.h"
#include "iANameMapper.h"

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
	iADiagramFctWidget(parent, 0, caption),
	m_data(data),
	m_markedLocation(InvalidMarker),
	m_nameMapper(nameMapper),
	m_selectedHandle(-1)
{
	AddPlot(GetDrawer(m_data, DefaultColors::AllDataChartColor));
	m_minSliderPos = m_data->MapBinToValue(0);
	m_maxSliderPos = m_data->MapBinToValue(m_data->GetNumBin());
	SetCaptionPosition(Qt::AlignLeft | Qt::AlignTop);
	SetShowXAxisLabel(showCaption);
	for (int i = 0; i < m_data->GetNumBin(); ++i)
	{
		m_binColors.push_back(QColor(0, 0, 0, 0));
	}
}

double iAFilterChart::mapBinToValue(double bin) const
{
	return m_data->MapBinToValue(bin);
}

double iAFilterChart::mapValueToBin(double value) const
{
	return m_data->MapValueToBin(value);
}

QSharedPointer<iAPlot> iAFilterChart::GetDrawer(QSharedPointer<iAParamHistogramData> data, QColor color)
{
	return
		IsDrawnDiscrete() ?
		QSharedPointer<iAPlot>(new iABarGraphDrawer(data, color, 2))
		: QSharedPointer<iAPlot>(new iAFilledLineFunctionDrawer(data, color))
		//: QSharedPointer<iAPlot>(new iALineFunctionDrawer(data, color))
		;
}

void iAFilterChart::drawMarker(QPainter & painter, double markerLocation, QPen const & pen, QBrush const & brush)
{
	double diagX = value2X(markerLocation)-translationX;
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

void iAFilterChart::DrawAxes(QPainter& painter)
{
	// draw bin colors: m_binColors
	{
		double y1 =  0;
		double y2 =  ChartColoringHeight;
		int binWidth = std::ceil(mapValue(0.0, static_cast<double>(m_data->GetNumBin()), 0.0, ActiveWidth()*xZoom, static_cast<double>(1)));
		for (int b = 0; b < m_data->GetNumBin(); ++b)
		{
			if (m_binColors[b].alpha() == 0)
			{
				continue;
			}
			double x1 = mapValue(0.0, static_cast<double>(m_data->GetNumBin()), 0.0, ActiveWidth()*xZoom, static_cast<double>(b));

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

	iADiagramFctWidget::DrawAxes(painter);
}


void iAFilterChart::SetBinColor(int bin, QColor const & color)
{
	assert(bin >= 0 && bin < m_data->GetNumBin());
	m_binColors[bin] = color;
}


void iAFilterChart::SetMarker(double value)
{
	m_markedLocation = value;
	redraw();
}

void iAFilterChart::RemoveMarker()
{
	SetMarker(InvalidMarker);
}

iAValueType iAFilterChart::GetRangeType() const
{
	return m_data->GetRangeType();
}


double iAFilterChart::GetMinVisibleBin() const
{
	double minVisXBin = mapValue(0.0, ActiveWidth()*xZoom, 0.0, static_cast<double>(m_data->GetNumBin()), static_cast<double>(-translationX));
	return minVisXBin;
}

double iAFilterChart::GetMaxVisibleBin() const
{
	double maxVisXBin = mapValue(0.0, ActiveWidth()*xZoom, 0.0, static_cast<double>(m_data->GetNumBin()), static_cast<double>(ActiveWidth()-translationX));
	return maxVisXBin;
}

QString iAFilterChart::GetXAxisTickMarkLabel(double value, int placesBeforeComma, int requiredPlacesAfterComma)
{
	assert(Plots().size() > 0);
	if (Plots()[0]->GetData()->GetRangeType() == Categorical)
	{
		assert(m_nameMapper);
		return (value < m_nameMapper->size()) ? m_nameMapper->GetName(static_cast<int>(value)):
			"";
	}
	return iADiagramFctWidget::GetXAxisTickMarkLabel(value, placesBeforeComma, requiredPlacesAfterComma);
}

void iAFilterChart::contextMenuEvent(QContextMenuEvent *event)
{
	// disable context menu
}


int iAFilterChart::value2X(double value) const
{
	double bin = mapValueToBin(value);
	int xPos = mapValue(0.0, static_cast<double>(m_data->GetNumBin()), 0, static_cast<int>(ActiveWidth()*xZoom), bin) + translationX;
	return xPos;
}

double iAFilterChart::x2value(int x) const
{
	double bin = mapValue(0, static_cast<int>(ActiveWidth()*xZoom), 0.0, static_cast<double>(m_data->GetNumBin()), x-translationX);
	double value = mapBinToValue(bin);
	return value;
}

void iAFilterChart::mousePressEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::LeftButton )
	{
		// horizontal + vertical panning:
		if ( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
		{
			translationStartX = translationX;
			translationStartY = translationY;
			iADiagramWidget::changeMode( MOVE_VIEW_MODE, event );
			return;
		}
		
		if ( event->y() > geometry().height() - BottomMargin() - translationY
			  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			// check if we hit min or max handle:
			int x = event->x() - LeftMargin();

			int minX = value2X(m_minSliderPos);
			int maxX = value2X(m_maxSliderPos);
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
	iADiagramFctWidget::mousePressEvent(event);
}

void iAFilterChart::mouseReleaseEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::LeftButton )
	{
		if (m_selectedHandle != -1)
		{
			emit SelectionChanged();
			m_selectedHandle = -1;
			return;
		}
	}
	iADiagramFctWidget::mouseReleaseEvent( event );
}

void iAFilterChart::mouseMoveEvent( QMouseEvent *event )
{
	if (	( event->buttons() == Qt::LeftButton ) &&
			( event->y() > geometry().height() - BottomMargin() - translationY			// mouse event below X-axis
			  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) ) &&	
			  m_selectedHandle != -1)
	{
		int x = event->x() - LeftMargin() + m_selectionOffset;
		if (x < 0 || x-translationX > static_cast<int>(ActiveWidth()*xZoom) )
		{
			return;
		}
		double value = x2value(x);

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
				  .arg( m_data->XBounds()[0] )
				  .arg( m_data->XBounds()[1] )
		);
		QToolTip::showText( event->globalPos(), text, this );
		redraw();
		return;
	}
	else if ((event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
	{
		iADiagramWidget::mouseMoveEvent( event );
	}
}


void iAFilterChart::SetMinMaxSlider(double min, double max)
{
	m_minSliderPos = min;
	m_maxSliderPos = max;
	redraw();
}

double iAFilterChart::GetMinSliderPos()
{
	return m_minSliderPos;
}

double iAFilterChart::GetMaxSliderPos()
{
	return m_maxSliderPos;
}
