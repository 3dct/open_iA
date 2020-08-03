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
#include "iARangeSliderDiagramWidget.h"

#include <charts/iAChartFunctionTransfer.h>
#include <iACSVToQTableWidgetConverter.h>

#include <cassert>


class iAFilteringDiagramData : public iAPlotData
{
public:
	iAFilteringDiagramData(QSharedPointer<iAPlotData> other, int min, int max):
		m_data(new double[other->numBin()]),
		m_size(other->numBin()),
		m_other(other)
	{
		assert(other->numBin() < std::numeric_limits<int>::max());
		for (int i = 0; i < static_cast<int>(other->numBin()); ++i)
		{
			m_data[i] = (i >= min && i <= max) ? other->rawData()[i] : 0;
		}
	}

	DataType const* rawData() const override
	{
		return m_data;
	}

	size_t numBin() const override
	{
		return m_size;
	}
	double spacing() const override
	{
		return m_other->spacing();
	}
	double const* xBounds() const override
	{
		return m_other->xBounds();
	}
	DataType const* yBounds() const override
	{
		return m_other->yBounds();
	}
private:
	DataType* m_data;
	size_t m_size;
	QSharedPointer<iAPlotData> m_other;
};


iARangeSliderDiagramWidget::iARangeSliderDiagramWidget( QWidget *parent, MdiChild *mdiChild,
	vtkPiecewiseFunction* oTF, vtkColorTransferFunction* cTF, QSharedPointer<iARangeSliderDiagramData> data,
	QMap<double, QList<double> > *histogramMap, const QTableWidget *rawTable,
	QString const & xlabel, QString const & yLabel):
	iAChartWithFunctionsWidget(parent, mdiChild, xlabel, yLabel),
	m_data(data),
	m_selectionOrigin(QPoint(0, 0)),
	m_selectionRubberBand(new QRubberBand(QRubberBand::Rectangle, this)),
	m_xLabel(xlabel),
	m_yLabel(yLabel),
	m_selectionColor(qRgb(255, 0, 127)),
	m_firstSelectedBin(-1),
	m_lastSelectedBin(-1),
	m_histogramMap(histogramMap),
	m_rawTable(rawTable)
{
	setTransferFunctions(cTF, oTF);
	addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(m_data, QColor(70, 70, 70, 255))));
	m_selectionRubberBand->hide();
	( (iAChartTransferFunction*) m_functions[0] )->enableRangeSliderHandles( true );
}

void iARangeSliderDiagramWidget::drawFunctions( QPainter &painter )
{
	size_t counter = 0;
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	while ( it != m_functions.end() )
	{
		iAChartFunction *func = ( *it );

		if (counter == m_selectedFunction)
		{
			func->draw(painter, QColor(255, 128, 0, 255), iAChartFunction::LineWidthSelected);
		}
		else
		{
			func->draw(painter);
		}
		++it;
		++counter;
	}
}

void iARangeSliderDiagramWidget::mouseDoubleClickEvent( QMouseEvent * /*event*/ )
{
}

void iARangeSliderDiagramWidget::mousePressEvent( QMouseEvent *event )
{
	std::vector<iAChartFunction*>::iterator it = m_functions.begin();
	iAChartFunction *func = *( it + m_selectedFunction );
	int x = event->x() - leftMargin();
	int selectedPoint = func->selectPoint( event, &x );

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
		if (selectedPoint == -1 && x < 0)
		{
			return;
		}

		// disallow removal and reinsertion of first point; instead, insert a point after it:
		if (selectedPoint == -1 && x == 0)
		{
			x = 1;
		}

		// mouse event plus CTRL above X-axis
		if ( ( event->modifiers() & Qt::ControlModifier ) == Qt::ControlModifier )
		{
			if (getBin(event) == -1)
			{
				return;
			}

			m_selectionOrigin = event->pos();
			m_selectionRubberBand->setGeometry( QRect( m_selectionOrigin, QSize() ) );
			m_selectionRubberBand->show();
			m_firstSelectedBin = getBin( event );

			if ( selectedPoint == -1 )
			{
				m_addedHandles.clear();
				func->removePoint( 1 );
				func->removePoint( 1 );
				selectedPoint = func->addPoint( dataBin2ScreenX( m_firstSelectedBin ) + m_translationX, 0 );
				func->addColorPoint( dataBin2ScreenX( m_firstSelectedBin ) + m_translationX );
				m_addedHandles.append( m_firstSelectedBin );
			}
		}
		else if ( event->y() > geometry().height() - bottomMargin() - m_translationY  &&
				  !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			if ( m_addedHandles.size() < 2 )
			{
				selectedPoint = func->addPoint( x, 0 );
				func->addColorPoint( x );
				m_addedHandles.append( getBin( event ) );

				if ( m_addedHandles.size() == 1 )
					m_firstSelectedBin = getBin( event );
				else
					m_lastSelectedBin = getBin( event );
			}
		}
		else if ( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier )
		{
			m_translationStartX = m_translationX;
			m_translationStartY = m_translationY;
			iAChartWidget::changeMode( MOVE_VIEW_MODE, event );
		}
	}
}

void iARangeSliderDiagramWidget::mouseReleaseEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::RightButton )
	{
		std::vector<iAChartFunction*>::iterator it = m_functions.begin();
		iAChartFunction *func = *( it + m_selectedFunction );
		func->selectPoint( event, 0 );	// to not allow last end point get selected
		update();
	}
	else if ( event->button() == Qt::LeftButton )
	{
		iAChartWithFunctionsWidget::mouseReleaseEvent( event );

		if ( m_selectionRubberBand->isVisible() )
		{
			m_selectionRubberBand->hide();
			m_lastSelectedBin = getBin( event );

			std::vector<iAChartFunction*>::iterator it = m_functions.begin();
			iAChartFunction *func = *( it + m_selectedFunction );
			int x = event->x() - leftMargin();
			int selectedPoint = func->selectPoint( event, &x );

			// don't do anything if outside of diagram region:
			if ( selectedPoint == -1 && x < 0 )
				return;

			// disallow removal and reinsertion of first point; instead, insert a point after it:
			if ( selectedPoint == -1 && x == 0 )
				x = 1;

			if ( selectedPoint == -1 )
			{
				selectedPoint = func->addPoint( dataBin2ScreenX( m_lastSelectedBin + 1 ) + m_translationX, 0 );
				func->addColorPoint( dataBin2ScreenX( m_lastSelectedBin + 1 ) + m_translationX );
				m_addedHandles.append( m_lastSelectedBin );

				if ( m_firstSelectedBin > m_lastSelectedBin )
					std::swap( m_firstSelectedBin, m_lastSelectedBin );

				setupSelectionDrawer();

				//Handle snap
				func->moveSelectedPoint( dataBin2ScreenX( m_firstSelectedBin ) + m_translationX, 0 );
				func->moveSelectedPoint( dataBin2ScreenX( m_lastSelectedBin + 1 ) + m_translationX, 0 );
			}
		}
		else if ( event->y() > geometry().height() - bottomMargin() - m_translationY )	// mouse event below X-axis
		{
			if ( m_addedHandles.size() == 2 )	// there are two handles draw a selection
			{
				if ( m_firstSelectedBin > m_lastSelectedBin )
					std::swap( m_firstSelectedBin, m_lastSelectedBin );
				setupSelectionDrawer();
				emit selectionRelesedSignal();
				emit selected();
			}

			std::vector<iAChartFunction*>::iterator it = m_functions.begin();
			iAChartFunction *func = *( it + m_selectedFunction );

			//Handle snap
			if ( func->getSelectedPoint() == 1 )
				func->moveSelectedPoint( dataBin2ScreenX( m_firstSelectedBin ) + m_translationX, 0 );

			if ( func->getSelectedPoint() == 2 )
				func->moveSelectedPoint( dataBin2ScreenX( m_lastSelectedBin + 1 ) + m_translationX, 0 );
		}
	}
}

void iARangeSliderDiagramWidget::mouseMoveEvent( QMouseEvent *event )
{
	if ( event->buttons() == Qt::LeftButton )
	{
		if ( ( event->modifiers() & Qt::ControlModifier ) == Qt::ControlModifier )
		{
			if ( !m_selectionRubberBand->isVisible() || getBin( event ) == -1 )
				return;

			m_selectionRubberBand->setGeometry( QRect( m_selectionOrigin, event->pos() ).normalized() );
		}
		else if ( event->y() > geometry().height() - bottomMargin() - m_translationY
				  && !( ( event->modifiers() & Qt::ShiftModifier ) == Qt::ShiftModifier ) )	// mouse event below X-axis
		{
			std::vector<iAChartFunction*>::iterator it = m_functions.begin();
			iAChartFunction *func = *( it + m_selectedFunction );
			int x = event->x() - leftMargin();
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
					if ( selectedPoint == 1 && getBin( event ) != m_firstSelectedBin )
						m_firstSelectedBin = getBin( event );

					if ( selectedPoint == 2 && getBin( event ) != m_lastSelectedBin )
						m_lastSelectedBin = getBin( event );

					if ( m_firstSelectedBin >= m_lastSelectedBin )
						std::swap( m_firstSelectedBin, m_lastSelectedBin );

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
	iAPlotData::DataType const * rawData = m_data->rawData();
	if ( !rawData )	return -1;
	int nthBin =  screenX2DataBin(event->x());
	QString text( tr( "%1: %2 %\n%3: %4" )
				  .arg( m_yLabel )
				  .arg( rawData[nthBin] )
				  .arg( m_xLabel )
				  .arg( (m_data->spacing() * nthBin + xBounds()[0] ) ) );
	QToolTip::showText( event->globalPos(), text, this );
	return nthBin;
}

void iARangeSliderDiagramWidget::setupSelectionDrawer()
{
	m_selectedData  = QSharedPointer<iAPlotData>( new iAFilteringDiagramData( m_data, m_firstSelectedBin, m_lastSelectedBin ) );

	if ( m_selectionDrawer )
		removePlot( m_selectionDrawer );

	m_selectionDrawer = QSharedPointer<iAStepFunctionPlot>( new iAStepFunctionPlot( m_selectedData, m_selectionColor ) );
	addPlot( m_selectionDrawer );
}

void iARangeSliderDiagramWidget::selectSlot()
{
	iARangeSliderDiagramWidget* diagram = static_cast<iARangeSliderDiagramWidget*>( QObject::sender() );
	int f = diagram->m_firstSelectedBin;
	int l = diagram->m_lastSelectedBin;

	if ( f == -1 && l == -1 )
		return;

	QMap<double, QList<double> > map = *m_histogramMap;

	QSet<double> set;
	for ( int i = f; i <= l; ++i )
	{
		QList<double> tmp = map.find( i ).value();
		if (tmp.size())
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			set.unite(QSet<double>(tmp.begin(), tmp.end()));
#else
			set.unite(tmp.toSet());
#endif
		}
	}

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	QList<double> rawTableRows(set.values());
#else
	QList<double> rawTableRows = set.toList();
#endif

	if ( m_histogramDrawerList.size() )
	{
		QListIterator<QSharedPointer<iAStepFunctionPlot> > it( m_histogramDrawerList );
		while ( it.hasNext() )
			removePlot( it.next() );

		m_histogramDrawerList.clear();
	}

	QListIterator<double> it( rawTableRows );
	while ( it.hasNext() )
	{
		int row = it.next();

		QSharedPointer<iAPlotData> selectedData = QSharedPointer<iAPlotData>(
			new iAFilteringDiagramData( m_data, row - 1, row - 1 ) );	//-1 cause of DiagramData

		QSharedPointer<iAStepFunctionPlot> selectionDrawer = QSharedPointer<iAStepFunctionPlot>(
			new iAStepFunctionPlot( selectedData, QColor( Qt::yellow ) ) );

		m_histogramDrawerList.append( selectionDrawer );
		addPlot( selectionDrawer );
	}
	update();
}

void iARangeSliderDiagramWidget::deleteSlot()
{
	if ( m_histogramDrawerList.size() )
	{
		QListIterator<QSharedPointer<iAStepFunctionPlot> > it( m_histogramDrawerList );
		while ( it.hasNext() )
			removePlot( it.next() );

		m_histogramDrawerList.clear();
	}
	update();
}

void iARangeSliderDiagramWidget::updateSelectedDiagrams()
{
	emit selected();
}

QList<int> iARangeSliderDiagramWidget::getSelectedRawTableRows()
{
	QList<int> selectedRawTableRows;

	if ( m_firstSelectedBin == -1 || m_lastSelectedBin == -1 )
		return selectedRawTableRows;

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