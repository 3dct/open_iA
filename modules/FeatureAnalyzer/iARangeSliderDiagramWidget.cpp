// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARangeSliderDiagramWidget.h"

#include <iAChartFunctionTransfer.h>
#include <iAHistogramData.h>
#include <iACSVToQTableWidgetConverter.h>
#include <iAMapper.h>
#include <iAMathUtility.h>
#include <iATransferFunctionPtrs.h>

#include <QToolTip>

#include <cassert>

std::shared_ptr<iAHistogramData> createFilteredPlotData(
	std::shared_ptr<iAPlotData> other, int firstBin, int lastBin)
{
	auto result = iAHistogramData::create("Filtered "+other->name(), other->valueType(),
		other->xBounds()[0], other->xBounds()[1], other->valueCount());
	assert(other->valueCount() < std::numeric_limits<int>::max());
	for (int i = 0; i < static_cast<int>(other->valueCount()); ++i)
	{
		result->setBin(i, (i >= firstBin && i <= lastBin) ? other->yValue(i) : 0);
	}
	return result;
}

iARangeSliderDiagramWidget::iARangeSliderDiagramWidget(QWidget* parent, vtkPiecewiseFunction* oTF,
	vtkColorTransferFunction* cTF, std::shared_ptr<iAHistogramData> data, QMultiMap<double, QList<double>>* histogramMap,
	const QTableWidget* rawTable, QString const& xlabel, QString const& yLabel) :
	iAChartWithFunctionsWidget(parent, xlabel, yLabel),
	m_data(data),
	m_selectionOrigin(QPoint(0, 0)),
	m_selectionRubberBand(new QRubberBand(QRubberBand::Rectangle, this)),
	m_xLabel(xlabel),
	m_yLabel(yLabel),
	m_selectionColor(qRgb(255, 0, 127)),
	m_firstSelectedBin(-1),
	m_lastSelectedBin(-1),
	m_histogramMap(histogramMap),
	m_rawTable(rawTable),
	m_tf(new iATransferFunctionPtrs(cTF, oTF))
{
	setTransferFunction(m_tf.get());
	addPlot(std::make_shared<iABarGraphPlot>(m_data, QColor(70, 70, 70, 255)));
	m_selectionRubberBand->hide();
	( (iAChartTransferFunction*) m_functions[0] )->enableRangeSliderHandles( true );
}

void iARangeSliderDiagramWidget::drawFunctions( QPainter &painter )
{
	size_t counter = 0;
	for (auto func: m_functions)
	{
		if (counter == m_selectedFunction)
		{
			func->draw(painter, QColor(255, 128, 0, 255), iAChartFunction::LineWidthSelected);
		}
		else
		{
			func->draw(painter);
		}
		++counter;
	}
}

void iARangeSliderDiagramWidget::mouseDoubleClickEvent( QMouseEvent * /*event*/ )
{
}

void iARangeSliderDiagramWidget::mousePressEvent(QMouseEvent* event)
{
	auto func = m_functions[m_selectedFunction];
	int mouseX = event->position().x() - leftMargin();
	int selectedPoint = func->selectPoint(mouseX, chartHeight() - event->position().y());

	if ( event->button() == Qt::RightButton )
	{
		func->removePoint( selectedPoint );
		m_addedHandles.removeAt( selectedPoint - 1);

		if (m_selectionDrawer)
		{
			removePlot(m_selectionDrawer);
		}

		emit deselected();
	}
	else if ( event->button() == Qt::LeftButton )
	{
		// don't do anything if outside of diagram region:
		if (selectedPoint == -1 && mouseX < 0)
		{
			return;
		}
		mouseX = clamp(1, chartWidth() - 1, mouseX);

		// mouse event plus CTRL above X-axis
		if ( ( event->modifiers() & Qt::ControlModifier ) == Qt::ControlModifier )
		{
			// TODO: check if outside range: getBin always clamps to valid bins!
			m_selectionOrigin = event->pos();
			m_selectionRubberBand->setGeometry( QRect( m_selectionOrigin, QSize() ) );
			m_selectionRubberBand->show();
			m_firstSelectedBin = getBin( event );

			if ( selectedPoint == -1 )
			{
				m_addedHandles.clear();
				func->removePoint( 1 );
				func->removePoint( 1 );
				selectedPoint = func->addPoint(dataBin2ScreenX(m_firstSelectedBin) - m_xMapper->srcToDst(m_xShift), 0);
				func->addColorPoint(dataBin2ScreenX(m_firstSelectedBin) - m_xMapper->srcToDst(m_xShift));
				m_addedHandles.append( m_firstSelectedBin );
			}
		}
		else if (event->position().y() > geometry().height() - bottomMargin() - m_translationY &&
			!( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			if ( m_addedHandles.size() < 2 )
			{
				selectedPoint = func->addPoint( mouseX, 0 );
				func->addColorPoint(mouseX);
				m_addedHandles.append( getBin( event ) );

				if ( m_addedHandles.size() == 1 )
					m_firstSelectedBin = getBin( event );
				else
					m_lastSelectedBin = getBin( event );
			}
		}
		else if ( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
		{
			iAChartWidget::changeMode( MOVE_VIEW_MODE, event );
		}
	}
}

void iARangeSliderDiagramWidget::mouseReleaseEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::RightButton )
	{
		m_functions[m_selectedFunction]->selectPoint(event->position().x() - leftMargin(), chartHeight() - event->position().y());
		update();
	}
	else if ( event->button() == Qt::LeftButton )
	{
		iAChartWithFunctionsWidget::mouseReleaseEvent( event );

		if ( m_selectionRubberBand->isVisible() )
		{
			m_selectionRubberBand->hide();
			m_lastSelectedBin = getBin( event );

			auto func = m_functions[m_selectedFunction];
			int mouseX = event->position().x() - leftMargin();
			int selectedPoint = func->selectPoint(mouseX, chartHeight() - event->position().y());

			// don't do anything if outside of diagram region:
			if ( selectedPoint == -1 && mouseX < 0 )
				return;

			if ( selectedPoint == -1 )
			{
				selectedPoint =	func->addPoint(dataBin2ScreenX(m_lastSelectedBin + 1) - m_xMapper->srcToDst(m_xShift), 0);
				func->addColorPoint(dataBin2ScreenX(m_lastSelectedBin + 1) - m_xMapper->srcToDst(m_xShift));
				m_addedHandles.append( m_lastSelectedBin );

				if ( m_firstSelectedBin > m_lastSelectedBin )
					std::swap( m_firstSelectedBin, m_lastSelectedBin );

				setupSelectionDrawer();

				//Handle snap
				func->moveSelectedPoint(dataBin2ScreenX(m_firstSelectedBin) - m_xMapper->srcToDst(m_xShift), 0);
				func->moveSelectedPoint(dataBin2ScreenX(m_lastSelectedBin + 1) - m_xMapper->srcToDst(m_xShift), 0);
			}
		}
		else if ( event->position().y() > geometry().height() - bottomMargin() - m_translationY )	// mouse event below X-axis
		{
			if ( m_addedHandles.size() == 2 )	// there are two handles draw a selection
			{
				if (m_firstSelectedBin > m_lastSelectedBin)
				{
					std::swap(m_firstSelectedBin, m_lastSelectedBin);
				}
				setupSelectionDrawer();
				emit selectionRelesedSignal();
				emit selected();
			}

			//Handle snap
			auto func = m_functions[m_selectedFunction];
			if (func->getSelectedPoint() == 1)
			{
				func->moveSelectedPoint(dataBin2ScreenX(m_firstSelectedBin) - m_xMapper->srcToDst(m_xShift), 0);
			}

			if (func->getSelectedPoint() == 2)
			{
				func->moveSelectedPoint(dataBin2ScreenX(m_lastSelectedBin + 1) - m_xMapper->srcToDst(m_xShift), 0);
			}
		}
	}
}

void iARangeSliderDiagramWidget::mouseMoveEvent( QMouseEvent *event )
{
	if ( event->buttons() == Qt::LeftButton )
	{
		if ( ( event->modifiers() & Qt::ControlModifier ) == Qt::ControlModifier )
		{
			if (!m_selectionRubberBand->isVisible() || getBin(event) == -1)
			{
				return;
			}
			m_selectionRubberBand->setGeometry( QRect( m_selectionOrigin, event->pos() ).normalized() );
		}
		else if ( event->position().y() > geometry().height() - bottomMargin() - m_translationY
		  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			auto func = m_functions[m_selectedFunction];
			int x = event->position().x() - leftMargin();
			int selectedPoint = func->getSelectedPoint();

			// don't do anything if not an added handle is selected
			if ( selectedPoint == -1 )
			{
				return;
			}
			else if ( selectedPoint == 1 || selectedPoint == 2 )	// update and draw selection of added handles
			{
				func->moveSelectedPoint( x, 0 );

				if ( m_addedHandles.size() == 2 )
				{
					if (selectedPoint == 1 && getBin(event) != m_firstSelectedBin)
					{
						m_firstSelectedBin = getBin(event);
					}
					if (selectedPoint == 2 && getBin(event) != m_lastSelectedBin)
					{
						m_lastSelectedBin = getBin(event);
					}
					if (m_firstSelectedBin >= m_lastSelectedBin)
					{
						std::swap(m_firstSelectedBin, m_lastSelectedBin);
					}
					setupSelectionDrawer();
					emit selected();
				}
				update();
			}
		}
		else if ( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
		{
			iAChartWidget::mouseMoveEvent( event );
		}
	}
}

void iARangeSliderDiagramWidget::contextMenuEvent( QContextMenuEvent * /*event*/ )
{

}

int iARangeSliderDiagramWidget::getBin( QMouseEvent *event )
{
	auto nthBin = m_data->nearestIdx(mouse2DataX(event->position().x() - leftMargin()));
	QString text( tr( "%1: %2 %\n%3: %4" )
				  .arg( m_yLabel )
				  .arg( m_data->yValue(nthBin) )
				  .arg( m_xLabel )
				  .arg( (m_data->spacing() * nthBin + xBounds()[0] ) ) );
	QToolTip::showText(event->globalPosition().toPoint(), text, this);
	return static_cast<int>(nthBin);
}

void iARangeSliderDiagramWidget::setupSelectionDrawer()
{
	m_selectedData = createFilteredPlotData( m_data, m_firstSelectedBin, m_lastSelectedBin );
	if (m_selectionDrawer)
	{
		removePlot(m_selectionDrawer);
	}
	m_selectionDrawer = std::make_shared<iAStepFunctionPlot>(m_selectedData, m_selectionColor);
	addPlot( m_selectionDrawer );
}

void iARangeSliderDiagramWidget::selectSlot()
{
	iARangeSliderDiagramWidget* diagram = static_cast<iARangeSliderDiagramWidget*>( QObject::sender() );
	int f = diagram->m_firstSelectedBin;
	int l = diagram->m_lastSelectedBin;

	if (f == -1 && l == -1)
	{
		return;
	}
	QMultiMap<double, QList<double> > map = *m_histogramMap;

	QSet<double> set;
	for ( int i = f; i <= l; ++i )
	{
		QList<double> tmp = map.find( i ).value();
		if (tmp.size())
		{
			set.unite(QSet<double>(tmp.begin(), tmp.end()));
		}
	}

	QList<double> rawTableRows(set.values());

	for(auto plot: m_histogramDrawerList)
	{
		removePlot(plot);
	}
	m_histogramDrawerList.clear();

	for (auto row: rawTableRows)
	{
		auto selectedData = createFilteredPlotData( m_data, row - 1, row - 1 );	//-1 cause of DiagramData
		auto selectionDrawer = std::make_shared<iAStepFunctionPlot>( selectedData, QColor( Qt::yellow ) );
		m_histogramDrawerList.append( selectionDrawer );
		addPlot( selectionDrawer );
	}
	update();
}

void iARangeSliderDiagramWidget::deleteSlot()
{
	for (auto plot: m_histogramDrawerList)
	{
		removePlot(plot);
	}
	m_histogramDrawerList.clear();
	update();
}

void iARangeSliderDiagramWidget::updateSelectedDiagrams()
{
	emit selected();
}

QList<int> iARangeSliderDiagramWidget::getSelectedRawTableRows()
{
	QList<int> selectedRawTableRows;

	if (m_firstSelectedBin == -1 || m_lastSelectedBin == -1)
	{
		return selectedRawTableRows;
	}
	QString parameterName = this->m_xLabel;
	int paramColumn = 0;
	for ( int j = 0; j < m_rawTable->columnCount(); ++j )
	{
		if ( m_rawTable->item( 0, j )->text() == parameterName )
		{
			paramColumn = j;
			break;
		}
	}

	double selMinValue = m_data->spacing() * m_firstSelectedBin + xBounds()[0];
	double selMaxValue = m_data->spacing() * m_lastSelectedBin + xBounds()[0];

	for ( int i = 1; i < m_rawTable->rowCount(); ++i )
	{
		if ( m_rawTable->item( i, paramColumn )->text().toDouble() <= selMaxValue &&
			 m_rawTable->item( i, paramColumn )->text().toDouble() >= selMinValue )
		{
			selectedRawTableRows.append( i - 1);
		}
	}

	return selectedRawTableRows;
}
