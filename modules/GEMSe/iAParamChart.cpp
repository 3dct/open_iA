/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iAParamChart.h"

#include "iAParamHistogramData.h"
#include "iAFunctionDrawers.h"
#include "iAMathUtility.h"
#include "iANameMapper.h"

#include <QMouseEvent>
#include <QToolTip>

namespace
{
	const double InvalidMarker = std::numeric_limits<double>::lowest();
}

iAParamChart::iAParamChart(QWidget* parent,
	vtkPiecewiseFunction* otf,
	vtkColorTransferFunction* ctf,
	QString const & caption,
	QSharedPointer<iAParamHistogramData> data,
	QSharedPointer<iANameMapper> nameMapper)
:
	iADiagramFctWidget(parent, 0, otf, ctf, caption),
	m_data(data),
	m_markedLocation(InvalidMarker),
	m_nameMapper(nameMapper),
	m_selectedHandle(-1)
{
	m_minSliderPos = m_data->mapBinToValue(0);
	m_maxSliderPos = m_data->mapBinToValue(m_data->GetNumBin());
	m_captionPosition = Qt::AlignLeft | Qt::AlignTop;
	m_showXAxisLabel = false;
	m_showFunctions = false;
	SetXAxisSteps(std::min(static_cast<int>(m_data->GetNumBin()), 20));
	for (int i = 0; i < m_data->GetNumBin(); ++i)
	{
		m_binColors.push_back(QColor(0, 0, 0, 0));
	}
}

QSharedPointer<iAAbstractDiagramRangedData> iAParamChart::GetData()
{
	return m_data;
}

QSharedPointer<iAAbstractDiagramRangedData> const iAParamChart::GetData() const
{
	return m_data;
}

double iAParamChart::mapBinToValue(double bin) const
{
	return m_data->mapBinToValue(bin);
}

double iAParamChart::mapValueToBin(double value) const
{
	return m_data->mapValueToBin(value);
}

QSharedPointer<iAAbstractDrawableFunction> iAParamChart::GetDrawer(QSharedPointer<iAParamHistogramData> data, QColor color)
{
	return
		IsDrawnDiscrete() ?
		QSharedPointer<iAAbstractDrawableFunction>(new iABarGraphDrawer(data, color, 2))
		: QSharedPointer<iAAbstractDrawableFunction>(new iAFilledLineFunctionDrawer(data, color))
		//: QSharedPointer<iAAbstractDrawableFunction>(new iALineFunctionDrawer(data, color))
		;
}

QSharedPointer<iAAbstractDrawableFunction> iAParamChart::CreatePrimaryDrawer()
{
	return GetDrawer(m_data, DefaultColors::AllDataChartColor);
}

void iAParamChart::drawMarker(QPainter & painter, double markerLocation, QPen const & pen, QBrush const & brush)
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

void iAParamChart::drawAxes(QPainter& painter)
{
	// draw bin colors: m_binColors
	{
		double y1 =  0;
		double y2 =  ChartColoringHeight;
		int binWidth = std::ceil(mapValue(0.0, static_cast<double>(m_data->GetNumBin()), 0.0, getActiveWidth()*xZoom, static_cast<double>(1)));
		for (int b = 0; b < m_data->GetNumBin(); ++b)
		{
			if (m_binColors[b].alpha() == 0)
			{
				continue;
			}
			double x1 = mapValue(0.0, static_cast<double>(m_data->GetNumBin()), 0.0, getActiveWidth()*xZoom, static_cast<double>(b));

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

	iADiagramFctWidget::drawAxes(painter);
}


void iAParamChart::SetBinColor(int bin, QColor const & color)
{
	assert(bin >= 0 && bin < m_data->GetNumBin());
	m_binColors[bin] = color;
}


void iAParamChart::SetMarker(double value)
{
	m_markedLocation = value;
	redraw();
}

void iAParamChart::RemoveMarker()
{
	m_markedLocation = InvalidMarker;
}

iAValueType iAParamChart::GetRangeType() const
{
	return m_data->GetRangeType();
}


double iAParamChart::GetMinVisibleBin() const
{
	double minVisXBin = mapValue(0.0, getActiveWidth()*xZoom, 0.0, static_cast<double>(m_data->GetNumBin()), static_cast<double>(-translationX));
	return minVisXBin;
}

double iAParamChart::GetMaxVisibleBin() const
{
	double maxVisXBin = mapValue(0.0, getActiveWidth()*xZoom, 0.0, static_cast<double>(m_data->GetNumBin()), static_cast<double>(getActiveWidth()-translationX));
	return maxVisXBin;
}

QString iAParamChart::GetXAxisCaption(double value, int placesBeforeComma, int requiredPlacesAfterComma)
{
	if (GetData()->GetRangeType() == Categorical)
	{
		assert(m_nameMapper);
		return (value < m_nameMapper->size()) ? m_nameMapper->GetName(static_cast<int>(value)):
			"";
	}
	return iADiagramFctWidget::GetXAxisCaption(value, placesBeforeComma, requiredPlacesAfterComma);
}

void iAParamChart::contextMenuEvent(QContextMenuEvent *event)
{
	// disable context menu
}


int iAParamChart::value2X(double value) const
{
	double bin = mapValueToBin(value);
	int xPos = mapValue(0.0, static_cast<double>(m_data->GetNumBin()), 0, static_cast<int>(getActiveWidth()*xZoom), bin) + translationX;
	return xPos;
}

double iAParamChart::x2value(int x) const
{
	double bin = mapValue(0, static_cast<int>(getActiveWidth()*xZoom), 0.0, static_cast<double>(m_data->GetNumBin()), x-translationX);
	double value = mapBinToValue(bin);
	return value;
}

void iAParamChart::mousePressEvent( QMouseEvent *event )
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
		
		if ( event->y() > geometry().height() - getBottomMargin() - translationY
			  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			// check if we hit min or max handle:
			int x = event->x() - getLeftMargin();

			int minX = value2X(m_minSliderPos);
			int maxX = value2X(m_maxSliderPos);
			if ( abs(x-minX) < MarkerTriangleWidthHalf)
			{
				m_selectedHandle = 0;
			}
			else if ( abs(x-maxX) < MarkerTriangleWidthHalf)
			{
				m_selectedHandle = 1;
			}
		}
	}
	iADiagramFctWidget::mousePressEvent(event);
}

void iAParamChart::mouseReleaseEvent( QMouseEvent *event )
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

void iAParamChart::mouseMoveEvent( QMouseEvent *event )
{
	if (	( event->buttons() == Qt::LeftButton ) &&
			( event->y() > geometry().height() - getBottomMargin() - translationY			// mouse event below X-axis
			  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) ) &&	
			  m_selectedHandle != -1)
	{
		int x = event->x() - getLeftMargin();
		if (x < 0 || x-translationX >= static_cast<int>(getActiveWidth()*xZoom) )
		{
			return;
		}
		double value = x2value(x);
		switch (m_selectedHandle)
		{
		case 0: m_minSliderPos = value; break;
		case 1: m_maxSliderPos = value; break;
		default: assert(false); break;
		}
		QString text( tr( "%1\n(data range: [%2..%3])" )
				  .arg( value )
				  .arg( m_data->GetDataRange()[0] )
				  .arg( m_data->GetDataRange()[1] )
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


void iAParamChart::SetMinMaxSlider(double min, double max)
{
	m_minSliderPos = min;
	m_maxSliderPos = max;
	redraw();
}

double iAParamChart::GetMinSliderPos()
{
	return m_minSliderPos;
}

double iAParamChart::GetMaxSliderPos()
{
	return m_maxSliderPos;
}
