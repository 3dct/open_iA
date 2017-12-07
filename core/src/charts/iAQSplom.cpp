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
#include "iAQSplom.h"
#include "iAScatterPlot.h"
#include "iALookupTable.h"
#include "iAMathUtility.h"
#include "iASPLOMData.h"

#include <vtkLookupTable.h>

#include <QAbstractTextDocumentLayout>
#include <QPropertyAnimation>
#include <QTableWidget>
#include <QWheelEvent>


iAQSplom::Settings::Settings()
	:plotsSpacing( 7 ),
	tickLabelsOffset( 5 ),
	maxRectExtraOffset( 20 ),
	tickOffsets( 45, 45 ),
	backgroundColor( Qt::white ),
	maximizedLinked( false ),
	popupBorderColor( QColor( 180, 180, 180, 220 )),
	popupFillColor(QColor( 250, 250, 250, 200 )),
	popupTextColor( QColor( 50, 50, 50 ) ),
	isAnimated( true ),
	animDuration( 100.0 ),
	animStart( 0.0 )
{
	popupTipDim[0] = 5; popupTipDim[1] = 10;
	popupSize[0] = 90; popupSize[1] = 50; 
}

void iAQSplom::setAnimIn( double anim )
{
	m_animIn = anim;
	update();
}

void iAQSplom::setAnimOut( double anim )
{
	m_animOut = anim;
	update();
}

const QList<int> & iAQSplom::getHighlightedPoints() const
{
	return m_highlightedPoints;
}

iAQSplom::iAQSplom( QWidget * parent /*= 0*/, const QGLWidget * shareWidget /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	:QGLWidget( parent, shareWidget, f ),
	settings(),
	m_lut( new iALookupTable() ),
	m_colorArrayName(),
	m_activePlot( 0 ),
	m_mode(ALL_PLOTS),
	m_splomData( new iASPLOMData() ),
	m_previewPlot( 0 ),
	m_maximizedPlot( 0 ),
	m_isIndexingBottomToTop( true ), //always true: maximizing will not work otherwise a proper layout needs to be implemented
	m_animIn( 1.0 ),
	m_animOut( 0.0 ),
	m_animationOut( new QPropertyAnimation( this, "m_animOut" ) ),
	m_animationIn( new QPropertyAnimation( this, "m_animIn" ) )
{
	setMouseTracking( true );
	setFocusPolicy( Qt::StrongFocus );

	m_animationIn->setDuration( settings.animDuration );
	m_animationOut->setDuration( settings.animDuration );
}

void iAQSplom::initializeGL()
{
	this->qglClearColor( settings.backgroundColor );
}

iAQSplom::~iAQSplom()
{
	delete m_maximizedPlot;
	delete m_animationIn;
	delete m_animationOut;
}

void iAQSplom::setData( const QTableWidget * data )
{
	m_splomData->import( data );
	clear();
	unsigned long numParams = m_splomData->numParams();
	for( unsigned long y = 0; y < numParams; ++y )
	{
		m_paramVisibility.push_back( true );
		QList<iAScatterPlot*> row;
		for( unsigned long x = 0; x < numParams; ++x )
		{
			iAScatterPlot * s = new iAScatterPlot(this, this);
			connect( s, SIGNAL( selectionModified() ), this, SLOT( selectionUpdated() ) );
			connect( s, SIGNAL( transformModified( double, QPointF ) ), this, SLOT( transformUpdated( double, QPointF ) ) );
			connect( s, SIGNAL( currentPointModified( int ) ), this, SLOT( currentPointUpdated( int ) ) );
			connect( s, SIGNAL( plotMaximized() ), this, SLOT( plotMaximized() ) );
			s->setData( x, y, m_splomData ); //we want first plot in lower left corner of the SPLOM
			if( m_lut->initialized() )
				s->setLookupTable( m_lut, m_colorArrayName );
			row.push_back( s );
		}
		m_matrix.push_back( row );
	}
	updateVisiblePlots();
}

void iAQSplom::setLookupTable( vtkLookupTable * lut, const QString & colorArrayName )
{
	m_lut->copyFromVTK( lut );
	m_colorArrayName = colorArrayName;
	applyLookupTable();
}

void iAQSplom::setLookupTable( iALookupTable &lut, const QString & colorArrayName ) //TODO: method not tested
{
	*m_lut = lut;
	m_colorArrayName = colorArrayName;
	applyLookupTable();
}

void iAQSplom::applyLookupTable()
{
	foreach( QList<iAScatterPlot*> row, m_matrix )
	{
		foreach( iAScatterPlot* s, row )
		{
			s->setLookupTable( m_lut, m_colorArrayName );
		}
	}
	if( m_maximizedPlot )
		m_maximizedPlot->setLookupTable( m_lut, m_colorArrayName );
	update();
}

void iAQSplom::setParameterVisibility( const QString & paramName, bool isVisible )
{
	int paramIndex = -1;
	for( unsigned long i = 0; i < m_splomData->numParams(); ++i )
	{
		if( m_splomData->parameterName(i) == paramName )
			paramIndex = i;
	}
	setParameterVisibility( paramIndex, isVisible );
}

void iAQSplom::setParameterVisibility( int paramIndex, bool isVisible )
{
	if( paramIndex < 0 || paramIndex >= m_paramVisibility.size() )
		return;
	m_paramVisibility[paramIndex] = isVisible;
	updateVisiblePlots();
	update();
}

QVector<unsigned int> & iAQSplom::getSelection()
{
	return m_selInds;
}

void iAQSplom::setSelection( const QVector<unsigned int> * selInds )
{
	m_selInds = *selInds;
	update();
}

void iAQSplom::getActivePlotIndices( int * inds_out )
{
	if( !m_activePlot )
	{
		inds_out[0] = inds_out[1] = -1;
	}
	else
	{
		inds_out[0] = m_activePlot->getIndices()[0];
		inds_out[1] = m_activePlot->getIndices()[1];
	}
}

void iAQSplom::clear()
{
	removeMaximizedPlot();
	m_activePlot = 0;
	foreach( QList<iAScatterPlot*> row, m_matrix )
	{
		foreach( iAScatterPlot* s, row )
		{
			s->disconnect();
			delete s;
		}
		row.clear();
	}
	m_matrix.clear();
	m_paramVisibility.clear();
}

void iAQSplom::selectionUpdated()
{
	update();
	emit selectionModified( &m_selInds );
}

void iAQSplom::transformUpdated( double scale, QPointF deltaOffset )
{
	iAScatterPlot * sender = dynamic_cast<iAScatterPlot*>( QObject::sender() );

	if (m_maximizedPlot != 0 && settings.maximizedLinked)
	{
		m_maximizedPlot->setTransform(sender->getScale(),
									  QPointF(sender->getOffset().x() / sender->getRect().width() * m_maximizedPlot->getRect().width(),
											  sender->getOffset().y() / sender->getRect().height() * m_maximizedPlot->getRect().height()));

		if (sender == m_maximizedPlot)
			deltaOffset = QPointF(deltaOffset.x() / m_maximizedPlot->getRect().width() * m_previewPlot->getRect().width(),
								  deltaOffset.y() / m_maximizedPlot->getRect().height() * m_previewPlot->getRect().height());

		/*if (sender == m_maximizedPlot)
		{
			m_previewPlot->setTransform(sender->getScale(), deltaOffset);
			sender = m_previewPlot;
		}*/
	}

	if( !sender )
		return;
	const int * ind = sender->getIndices();
	foreach( QList<iAScatterPlot*> row, m_matrix )
	{
		foreach( iAScatterPlot* s, row )
		{
			if( s != sender )
			{
				if (s != m_previewPlot )
				{
					if( s->getIndices()[0] == ind[0] )
						s->setTransformDelta( scale, QPointF( deltaOffset.x(), 0.0f ) );
					else if( s->getIndices()[1] == ind[1] )
						s->setTransformDelta( scale, QPointF( 0.0f, deltaOffset.y() ) );
					else
						s->setTransformDelta( scale, QPointF( 0.0f, 0.0f ) );
				}
				else
				{
					s->setTransformDelta( scale, deltaOffset );
				}
			}
		}
	}

	update();
}

void iAQSplom::currentPointUpdated( int index )
{
	foreach( QList<iAScatterPlot*> row, m_matrix )
	{
		foreach( iAScatterPlot* s, row )
		{
			if( s != QObject::sender() )
			{
				s->setCurrentPoint( index );
			}
		}
	}
	if( m_maximizedPlot && ( m_maximizedPlot != QObject::sender() ) )
		m_maximizedPlot->setCurrentPoint( index );
	
	//animation
	if( settings.isAnimated && m_activePlot )
	{
		int curPt = m_activePlot->getCurrentPoint();
		int prePt = m_activePlot->getPreviousIndex();
		if( prePt >= 0 && curPt != prePt )
		{
			//Out
			m_animationOut->setStartValue( m_animIn );
			m_animationOut->setEndValue( 0.0 );
			m_animationOut->start();
		}
		if( curPt >= 0 )
		{
			//In
			m_animationIn->setStartValue( settings.animStart );
			m_animationIn->setEndValue( 1.0 );
			m_animationIn->start();
		}
	}

	update();
	if( index >= 0 )
		emit currentPointModified( index );
}

void iAQSplom::addHighlightedPoint( int index )
{
	if( !m_highlightedPoints.contains( index ) )
		m_highlightedPoints.push_back( index );
	update();
}

void iAQSplom::removeHighlightedPoint( int index )
{
	int pos = m_highlightedPoints.indexOf( index );
	if( pos >= 0 )
		m_highlightedPoints.removeAt( pos );
	update();
}

void iAQSplom::plotMaximized()
{
	iAScatterPlot * senderPlot = dynamic_cast<iAScatterPlot *>( QObject::sender() );
	if( !senderPlot )
		return;
	
	if( m_previewPlot )
		m_previewPlot->setPreviewState( false );
	senderPlot->setPreviewState( true );
	m_previewPlot = senderPlot;
	
	m_mode = UPPER_HALF;

	//hide lower triangle
	QList<QList<iAScatterPlot*>> newVisPlots;
	int visParamCnt = getVisibleParametersCount();
	for( int y = 0; y < visParamCnt; ++y )
	{
		QList<iAScatterPlot*> row;
		for( int x = 0; x < visParamCnt; ++x )
		{
			if( x <=  y )
				row.push_back( m_visiblePlots[y][x] );
			else
				row.push_back( 0 );
		}
		newVisPlots.push_back( row );
	}
	m_visiblePlots = newVisPlots;

	//create main plot
	delete m_maximizedPlot;
	m_maximizedPlot = new iAScatterPlot( this, this, 11, true );
	connect( m_maximizedPlot, SIGNAL( selectionModified() ), this, SLOT( selectionUpdated() ) );
	connect( m_maximizedPlot, SIGNAL( currentPointModified( int ) ), this, SLOT( currentPointUpdated( int ) ) );
	connect( m_maximizedPlot, SIGNAL( plotMaximized() ), this, SLOT( plotMinimized() ) );

	if (settings.maximizedLinked)
		connect( m_maximizedPlot, SIGNAL( transformModified(double,QPointF) ), this, SLOT( transformUpdated(double,QPointF) ) );

	const int * plotInds = senderPlot->getIndices();
	m_maximizedPlot->setData( plotInds[0], plotInds[1], m_splomData ); //we want first plot in lower left corner of the SPLOM
	if( m_lut->initialized() )
		m_maximizedPlot->setLookupTable( m_lut, m_colorArrayName );
	//rectangle
	updateMaxPlotRect();
	//transform
	QPointF ofst = senderPlot->getOffset();
	double scl[2] = {
		( (double)m_maximizedPlot->getRect().width() ) / senderPlot->getRect().width(),
		( (double)m_maximizedPlot->getRect().height() ) / senderPlot->getRect().height()
	};
	m_maximizedPlot->setTransform( senderPlot->getScale(), QPointF( ofst.x() * scl[0], ofst.y() * scl[1] ) );
	
	//final update
	update();
}

void iAQSplom::plotMinimized()
{
	removeMaximizedPlot();
	updateVisiblePlots();
	update();
}

void iAQSplom::removeMaximizedPlot()
{
	delete m_maximizedPlot;
	m_maximizedPlot = 0;
	m_activePlot = 0;
	if( m_previewPlot )
	{
		m_previewPlot->setPreviewState( false );
		m_previewPlot = 0;
	}
	m_mode = ALL_PLOTS;
}

int iAQSplom::invert( int val ) const
{
	return ( getVisibleParametersCount() - val - 1 );
}

void iAQSplom::paintEvent( QPaintEvent * event )
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.setRenderHint( QPainter::HighQualityAntialiasing );
	painter.beginNativePainting();
	glClear( GL_COLOR_BUFFER_BIT );
	painter.endNativePainting();
	if( !getVisibleParametersCount() )
		return;
	drawTicks( painter );
	foreach( const QList<iAScatterPlot*> & row, m_visiblePlots )
	{
		foreach( iAScatterPlot * s, row )
		{
			if( !s )
				continue;
			s->paintOnParent( painter );
		}
	}
	if( m_maximizedPlot )		
		m_maximizedPlot->paintOnParent( painter );

	drawPopup( painter );
}

bool iAQSplom::drawPopup( QPainter& painter )
{
	if( !m_activePlot )
		return false;
	int curInd = m_activePlot->getCurrentPoint();
	double anim = 1.0;
	if( curInd < 0 )
	{
		if( m_animOut > 0.0 && m_animIn >= 1.0)			
		{
			anim = m_animOut;
			curInd = m_activePlot->getPreviousPoint();
		}
		return false;
	}
	else if( -1 == m_activePlot->getPreviousIndex() )
			anim = m_animIn;
	const int * pInds = m_activePlot->getIndices();

	painter.save();
	QPointF popupPos = m_activePlot->getPointPosition( curInd );
	double pPM = m_activePlot->settings.pickedPointMagnification;
	double ptRad = m_activePlot->getPointRadius();
	popupPos.setY( popupPos.y() -  pPM * ptRad ); //popupPos.setY( popupPos.y() - ( 1 + ( pPM - 1 )*m_anim ) * ptRad );
	QColor col = settings.popupFillColor; col.setAlpha( col.alpha()* anim ); painter.setBrush( col );
	col = settings.popupBorderColor; col.setAlpha( col.alpha()* anim ); painter.setPen( col );
	//painter.setBrush( settings.popupFillColor );
	//painter.setPen( settings.popupBorderColor );
	painter.translate( popupPos );

	double * tipDim = settings.popupTipDim;
	double * popupSize = settings.popupSize;
	QPointF points[7] = {
		QPointF( 0, 0 ),
		QPointF( -tipDim[0], -tipDim[1] ),
		QPointF( -popupSize[0], -tipDim[1] ),
		QPointF( -popupSize[0], -popupSize[1] - tipDim[1] ),
		QPointF( popupSize[0], -popupSize[1] - tipDim[1] ),
		QPointF( popupSize[0], -tipDim[1] ),
		QPointF( tipDim[0], -tipDim[1] ),
	};
	painter.drawPolygon( points, 7 );

	//render popup text
	QRectF rect( points[3], points[5] );
	QString text = "<center><b>#" + QString::number( curInd ) + "</b><br> " + \
		m_splomData->parameterName( pInds[0] ) + ": " + \
		QString::number( m_splomData->paramData( pInds[0] )[curInd] ) + "<br>" + \
		m_splomData->parameterName( pInds[1] ) + ": " + \
		QString::number( m_splomData->paramData( pInds[1] )[curInd] ) + "</center>";
	QTextDocument doc;
	doc.setHtml( text );
	doc.setTextWidth( rect.width() );
	painter.translate( -popupSize[0], -popupSize[1] - tipDim[1] );

	QAbstractTextDocumentLayout::PaintContext ctx;
	col = settings.popupTextColor; col.setAlpha( col.alpha()* anim );
	ctx.palette.setColor( QPalette::Text, col );
	ctx.palette.setColor( QPalette::Text, settings.popupTextColor );
	doc.documentLayout()->draw( &painter, ctx ); //doc.drawContents( &painter );
	
	painter.restore();
	return true;
}

iAScatterPlot * iAQSplom::getScatterplotAt( QPoint pos )
{
	if( m_visiblePlots.isEmpty() )
		return 0;
	QPoint offsetPos = pos - settings.tickOffsets;		
	QPoint grid( m_scatPlotSize.x() + settings.plotsSpacing, m_scatPlotSize.y() + settings.plotsSpacing );
	if (grid.x() == 0 || grid.y() == 0)
		return 0;	// to avoid division by 0
	int ind[2] = { offsetPos.x() / grid.x(), offsetPos.y() / grid.y() };
	//boundary checks
	for( int i = 0; i < 2; ++i )		
	{
		ind[i] = clamp(0, getVisibleParametersCount() - 1, ind[i]);
	}
	//are we between plots due to the spacing?
	bool isBetween = false;
	if( offsetPos.x() - ind[0] * grid.x() >= m_scatPlotSize.x() ) isBetween = true;
	if( offsetPos.y() - ind[1] * grid.y() >= m_scatPlotSize.y() ) isBetween = true;
	//if indexing is bottom to top invert index Y
	if( m_isIndexingBottomToTop )
		ind[1] = invert( ind[1] );
	//get the resulting plot
	iAScatterPlot * s = isBetween ? 0 : m_visiblePlots[ind[1]][ind[0]];
	//check if we hit the maximized plot if necessary
	if( ( UPPER_HALF == m_mode ) && !s )
	{
		if( m_maximizedPlot )			
		{
			QRect r = m_maximizedPlot->getRect();
			if( r.contains( pos ) )
				s = m_maximizedPlot;
		}
	}

	return s;
}

void iAQSplom::resizeEvent( QResizeEvent * event )
{
	updateSPLOMLayout();
	QGLWidget::resizeEvent( event );
}

void iAQSplom::updatePlotGridParams()
{
	long plotsRect[2] = { 
		width() - settings.tickOffsets.x(), 
		height() - settings.tickOffsets.y() };
	long visParamCnt = getVisibleParametersCount();
	int spc = settings.plotsSpacing;
	int wSz[2] = {
		static_cast<int>(( plotsRect[0] - ( visParamCnt - 1 ) * spc ) / ( (double)visParamCnt )),
		static_cast<int>(( plotsRect[1] - ( visParamCnt - 1 ) * spc ) / ( (double)visParamCnt )),
	};
	m_scatPlotSize = QPoint( wSz[0], wSz[1] );
}

QRect iAQSplom::getPlotRectByIndex( int x, int y )
{
	if( m_isIndexingBottomToTop )
		y = invert( y );
	int spc = settings.plotsSpacing;
	int xpos = settings.tickOffsets.x() + x * ( m_scatPlotSize.x() + spc );
	int ypos = settings.tickOffsets.y() + y * ( m_scatPlotSize.y() + spc );
	QRect res( xpos, ypos, m_scatPlotSize.x(), m_scatPlotSize.y() );
	return res;
}

void iAQSplom::updateMaxPlotRect()
{
	if( !m_maximizedPlot )
		return;
	long visParamCnt = getVisibleParametersCount();
	int tl_ind = visParamCnt / 2 + 1;
	int br_ind = visParamCnt - 1;

	if( !( visParamCnt % 2 ) )
		tl_ind--;

	QRect tl_rect, br_rect;
	if( m_isIndexingBottomToTop )
	{
		int tl_ind_inv = invert( tl_ind );
		int br_ind_inv = invert( br_ind );
		tl_rect = getPlotRectByIndex( tl_ind, tl_ind_inv );
		br_rect = getPlotRectByIndex( br_ind, br_ind_inv );
	}
	else
	{
		tl_rect = getPlotRectByIndex( tl_ind, tl_ind ); 
		br_rect = getPlotRectByIndex( br_ind, br_ind );
	}
	QRect r = QRect( tl_rect.topLeft(), br_rect.bottomRight() );

	if( !( visParamCnt % 2 ) )
	{
		int extraOffset = settings.tickLabelsOffset + settings.maxRectExtraOffset;
		r.setTopLeft( r.topLeft() + settings.tickOffsets + QPoint( extraOffset, extraOffset ) );
	}

	m_maximizedPlot->setRect( r );
}

void iAQSplom::updateSPLOMLayout()
{
	long visParamCnt = getVisibleParametersCount();
	if( !visParamCnt )
		return;
	
	updatePlotGridParams();
	for( int yind = 0; yind < visParamCnt; ++yind )
	{
		QList<iAScatterPlot*> * row = &m_visiblePlots[yind];
		for( int xind = 0; xind < row->size(); ++xind )
		{
			iAScatterPlot * s = m_visiblePlots[yind][xind];
			if( s )
				s->setRect( getPlotRectByIndex( xind, yind ) );
		}
	}
	updateMaxPlotRect();
}

void iAQSplom::updateVisiblePlots()
{
	removeMaxPlotIfHidden();
	m_visiblePlots.clear();
	unsigned long numParams = m_splomData->numParams();
	for( unsigned long y = 0; y < numParams; ++y )
	{
		if( !m_paramVisibility[y] )
			continue;
		QList<iAScatterPlot*> row;
		for( unsigned long x = 0; x < numParams; ++x )
		{
			if( !m_paramVisibility[x] )
				continue;
			iAScatterPlot * plot = m_matrix[y][x];	
			if( UPPER_HALF == m_mode )
			{
				//hide lower triangle
				if( x > y )
					plot = 0;
			}
			row.push_back( plot );
		}
		m_visiblePlots.push_back( row );
	}
	updateSPLOMLayout();
}

void iAQSplom::removeMaxPlotIfHidden()
{
	if( m_maximizedPlot )
	{
		const int * inds = m_maximizedPlot->getIndices();
		if( !m_paramVisibility[inds[0]] || !m_paramVisibility[inds[1]] || ( getVisibleParametersCount() <= 1 ) )
			removeMaximizedPlot();
	}
}

void iAQSplom::resetTransform()
{
	foreach( const QList<iAScatterPlot*> & row, m_matrix )
	{
		foreach( iAScatterPlot * s, row )
		{
			s->setTransform( 1.0, QPointF( 0.0f, 0.0f ) );
		}
	}
	if(m_maximizedPlot)
		m_maximizedPlot->setTransform( 1.0, QPointF( 0.0f, 0.0f ) );
	update();
}

void iAQSplom::wheelEvent( QWheelEvent * event )
{
	iAScatterPlot * s = getScatterplotAt( event->pos() );
	if( s )		
	{
		s->SPLOMWheelEvent( event );
		if( !event->angleDelta().isNull() )
			update();
	}
	event->accept();
}

void iAQSplom::mousePressEvent( QMouseEvent * event )
{
	if( m_activePlot )
		m_activePlot->SPLOMMousePressEvent( event );
}

void iAQSplom::mouseReleaseEvent( QMouseEvent * event )
{
	if( m_activePlot )
		m_activePlot->SPLOMMouseReleaseEvent( event );
}

void iAQSplom::mouseMoveEvent( QMouseEvent * event )
{
	iAScatterPlot * s = getScatterplotAt( event->pos() );
	
	//make sure that if a button is pressed another plot will not hijack the event handling
	if( event->buttons()&Qt::RightButton || event->buttons()&Qt::LeftButton )
		s = m_activePlot;
	else
		changeActivePlot( s );

	if( s )
		s->SPLOMMouseMoveEvent( event );
}

void iAQSplom::keyPressEvent( QKeyEvent * event )
{
	switch (event->key())
	{
	case Qt::Key_R: //if R is pressed, reset all the applied transformation as offset and scaling
		resetTransform();
		break;
	}
}

void iAQSplom::mouseDoubleClickEvent( QMouseEvent * event )
{
	resetTransform();
}

void iAQSplom::changeActivePlot( iAScatterPlot * s )
{
	if( s != m_activePlot )
	{
		if( m_activePlot )
			m_activePlot->leave();
		if( s )
			s->enter();
		m_activePlot = s;
		update();
	}
}

void iAQSplom::drawTicks( QPainter & painter )
{
	//prepare painter
	painter.save();
	//collect info
	QList<double> ticksX, ticksY; QList<QString> textX, textY;
	for( int i = 0; i < m_visiblePlots.size(); ++i )		
	{
		m_visiblePlots[i][i]->printTicksInfo( &ticksX, &ticksY, &textX, &textY );
	}
	//draw ticks text
	painter.setPen( m_visiblePlots[0][0]->settings.tickLabelColor );
	QPoint * tOfs = &settings.tickOffsets;
	long tSpc = settings.tickLabelsOffset;
	for( long i = 0; i < ticksY.size(); ++i )
	{
		double t = ticksY[i]; QString text = textY[i];
		painter.drawText( QRectF( 0, t - tOfs->y(), tOfs->x() - tSpc, 2 * tOfs->y() ), Qt::AlignRight | Qt::AlignVCenter, text );
	}
	painter.rotate( -90 );
	for( long i = 0; i < ticksX.size(); ++i )
	{
		double t = ticksX[i]; QString text = textX[i];
		painter.drawText( QRectF( -tOfs->y() + tSpc, t - tOfs->x(), tOfs->y() - tSpc, 2 * tOfs->x() ), Qt::AlignLeft | Qt::AlignVCenter, text );
	}
	//restore painter
	painter.restore();
}