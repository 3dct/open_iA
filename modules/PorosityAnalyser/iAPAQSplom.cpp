/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAPAQSplom.h"

#include "PorosityAnalyserHelpers.h"

#include "iAMathUtility.h"
#include "charts/iASPLOMData.h"
#include "charts/iAScatterPlot.h"

#include <QDir>
#include <QKeyEvent>
#include <QMenu>
#include <QAbstractTextDocumentLayout>

//openMP
#ifndef __APPLE__
#ifndef __MACOSX
#include <omp.h>///TODO: gcc include omp //omp.h works with gcc 4.6
#endif
#endif

const int maskOpacity = 127;

iAPAQSplom::iAPAQSplom( QWidget * parent /*= 0*/, const QGLWidget * shareWidget /*= 0*/, Qt::WindowFlags f /*= 0 */ )
	: iAQSplom( parent, shareWidget, f ),
	m_fixAction( 0 ),
	m_removeFixedAction( 0 ),
	m_fixedPointInd( iAScatterPlot::NoPointIndex )
{
	m_fixAction = m_contextMenu->addAction( "Fix Point", this, SLOT( fixPoint() ) );
	m_removeFixedAction = m_contextMenu->addAction( "Remove Fixed Point", this, SLOT( removeFixedPoint() ) );
	m_fixAction->setVisible( false );
	m_removeFixedAction->setVisible( false );
}

void iAPAQSplom::setData( const QTableWidget * data )
{
	QTableWidget newData;
	newData.setRowCount( data->rowCount() );
	int maskCol = data->columnCount() - 1;
	newData.setColumnCount( maskCol );
	m_maskNames.clear();
	m_datasetIndices.clear();
	if( data->rowCount() )
	{
		newData.setUpdatesEnabled( false );  //for faster processing of large lists
		int datasetIndexCol = -1;
		for( int c = 0; c < maskCol; ++c ) //header		
		{
			QString s = data->item( 0, c )->text();
			if( s == "Dataset Index" )
				datasetIndexCol = c;
			newData.setItem( 0, c, new QTableWidgetItem( s ) );
		}
		for( int r = 1; r < data->rowCount(); ++r ) //points
		{
			for( int c = 0; c < maskCol; ++c )
				newData.setItem( r, c, new QTableWidgetItem( data->item( r, c )->text() ) );
			m_maskNames.push_back( data->item( r, maskCol )->text() );
			if( datasetIndexCol >= 0 )
				m_datasetIndices.push_back( data->item( r, datasetIndexCol )->text().toInt() );
		}
		newData.setUpdatesEnabled( true );  //done with load
	}

	iAQSplom::setData( &newData );
}

void iAPAQSplom::setPreviewSliceNumbers( QList<int> sliceNumberLst )
{
	m_sliceNumPopupLst = sliceNumberLst;
	updatePreviewPixmap();
	update();
}

void iAPAQSplom::setROIList( QList<QRectF> roiLst )
{
	m_roiLst = roiLst;
	updatePreviewPixmap();
	update();
}

void iAPAQSplom::setSliceCounts( QList<int> sliceCnts )
{
	m_sliceCntLst = sliceCnts;
	m_sliceNumPopupLst.clear();
	foreach( const int & sc, m_sliceCntLst )
		m_sliceNumPopupLst.push_back( sc*0.5 );
}

void iAPAQSplom::setDatasetsDir( QString datsetsDir )
{
	m_datasetsDir = datsetsDir;
}

void iAPAQSplom::setDatasetsByIndices( QStringList selDatasets, QList<int> indices )
{
	m_dsIndices = indices;
	m_datasets.clear();
	foreach( const QString & d, selDatasets )
		m_datasets.push_back( m_datasetsDir + "/" + QFileInfo( d ).baseName() );
}

void iAPAQSplom::reemitFixedPixmap()
{
	if( m_fixedPointInd != iAScatterPlot::NoPointIndex )
	{
		int dsInd = getDatasetIndexFromPointIndex( m_fixedPointInd );
		QString sliceFilename = getSliceFilename( m_maskNames[m_fixedPointInd], m_sliceNumPopupLst[dsInd] );
		QImage fixedMaskImg;
		if( !fixedMaskImg.load( sliceFilename, "PNG" ) )
			return;
		fixedMaskImg.setColor( 0, qRgba( 0, 0, 0, 0 ) );
		fixedMaskImg.setColor( 1, qRgba( 0, 255, 0, maskOpacity ) );

		m_maskPxmp = QPixmap::fromImage( fixedMaskImg );
		emit maskHovered( &m_maskPxmp, dsInd );
	}
	else
		emit maskHovered( nullptr, -1 );
}

int iAPAQSplom::getDatasetIndexFromPointIndex(size_t pointIndex)
{
	if (pointIndex == iAScatterPlot::NoPointIndex)
		return -1;
	int absInd = m_datasetIndices[pointIndex];
	int relInd = m_dsIndices.indexOf( absInd );
	return relInd;
}

bool iAPAQSplom::drawPopup( QPainter& painter )
{
	if( !iAQSplom::drawPopup( painter ) )
		return false;
	if( m_maskPxmp.isNull() )
		return false;
	size_t curInd = m_activePlot->getCurrentPoint();
	double anim = 1.0;
	if( curInd == iAScatterPlot::NoPointIndex )
	{
		if( m_animOut > 0.0 && m_animIn >= 1.0 )
		{
			anim = m_animOut;
			curInd = m_activePlot->getPreviousPoint();
		}
		else
			return false;
	}
	else if( m_activePlot->getPreviousIndex() == iAScatterPlot::NoPointIndex )
		anim = m_animIn;

	painter.save();

	QPointF popupPos = m_activePlot->getPointPosition( curInd );
	double pPM = m_activePlot->settings.pickedPointMagnification;
	double ptRad = m_activePlot->getPointRadius();
	popupPos.setY( popupPos.y() - pPM * ptRad );
	painter.translate( popupPos );
	
	
	int dsInd = getDatasetIndexFromPointIndex( curInd );
	const QRectF & roi = m_roiLst[dsInd];
	double scaledH = settings.popupWidth / roi.width() * roi.height();
	painter.setOpacity( anim*1.0 );

	//draw current dataset name, pipeline name, slice number
	QString text =
		"<table width =\"100%\" cellspacing = \"0\" cellpadding= \"0px\" border =\"0\">"
		"<tr><td width =\"40%\" align =\"left\"><div align =\"left\"><u>Dataset:</u></div></td>"
		"<td width =\"60%\" align =\"right\"><div align =\"right\">Z-Slice: " + QString::number( m_sliceNumPopupLst[dsInd] ) + "</div></td></tr>"
		"<tr><td width = \"100%\" align =\"left\"><div align =\"left\">" + m_currPrevDatasetName + "</div></td></tr>"
		"<tr></tr>"
		"<tr><td width = \"100%\" align =\"left\"><div align =\"left\"><u>Pipeline:</u></div></td></tr>"
		"<tr><td width = \"100%\" align =\"left\"><div align =\"left\">" + m_currPrevPipelineName + "</div></td></tr>"
		"</table>";
	QTextDocument doc;
	doc.setHtml( text );
	doc.setTextWidth( settings.popupWidth);
	QColor col = settings.popupFillColor; col.setAlpha( col.alpha()* anim ); painter.setBrush( col );
	col = settings.popupBorderColor; col.setAlpha( col.alpha()* anim ); painter.setPen( col );

	//draw dataset preview
	QPointF pixOrigin(-(settings.popupWidth/2), - m_popupHeight - settings.popupTipDim[1] - scaledH);
	painter.drawPixmap(pixOrigin, m_curSlicePxmp);
	painter.drawPixmap(pixOrigin, m_maskPxmpRoi);

	painter.drawRect( pixOrigin.x(), pixOrigin.y() - doc.size().rheight() - 3, settings.popupWidth, doc.size().rheight() + 3 );
	painter.translate( pixOrigin.x(), pixOrigin.y() - doc.size().rheight() - 3 );
	QAbstractTextDocumentLayout::PaintContext ctx;
	col = settings.popupTextColor; col.setAlpha( col.alpha()* anim );
	ctx.palette.setColor( QPalette::Text, col );
	ctx.palette.setColor( QPalette::Text, settings.popupTextColor );
	doc.documentLayout()->draw( &painter, ctx ); //doc.drawContents( &painter );

	painter.restore();
	return true;
}

void iAPAQSplom::keyPressEvent( QKeyEvent * event )
{
	int dsInd = getDatasetIndexFromPointIndex( m_activePlot->getCurrentPoint() );
	if (dsInd != -1)
	{
		switch (event->key())
		{
		case Qt::Key_Plus: //if plus is pressed increment slice number for the popup
			m_sliceNumPopupLst[dsInd] = clamp<int>( 0, m_sliceCntLst[dsInd] - 1, ++m_sliceNumPopupLst[dsInd] );
			update();
			updatePreviewPixmap();
			emit previewSliceChanged( m_sliceNumPopupLst[dsInd] );
			break;
		case Qt::Key_Minus: //if minus is pressed decrement slice number for the popup
			m_sliceNumPopupLst[dsInd] = clamp<int>( 0, m_sliceCntLst[dsInd] - 1, --m_sliceNumPopupLst[dsInd] );
			update();
			updatePreviewPixmap();
			emit previewSliceChanged( m_sliceNumPopupLst[dsInd] );
			break;
		}
	}
	iAQSplom::keyPressEvent( event );
}

void iAPAQSplom::updatePreviewPixmap()
{
	if( !m_activePlot )
		return;

	size_t curInd = m_activePlot->getCurrentPoint();
	if( curInd == iAScatterPlot::NoPointIndex && m_fixedPointInd == iAScatterPlot::NoPointIndex )
	{
		emit maskHovered( 0, -1 );
		return;
	}	

	QImage fixedMaskImg;
	int dsInd = -1;
	if( m_fixedPointInd >= 0 )
	{
		dsInd = getDatasetIndexFromPointIndex( m_fixedPointInd );
		QString sliceFilename = getSliceFilename( m_maskNames[m_fixedPointInd], m_sliceNumPopupLst[dsInd] );
		if( !fixedMaskImg.load( sliceFilename, "PNG" ) )
			return;
		fixedMaskImg.setColor( 0, qRgba( 0, 0, 0, 0 ) );
		fixedMaskImg.setColor( 1, qRgba( 0, 255, 0, maskOpacity ) );
	}

	QImage maskImg;
	if( curInd == iAScatterPlot::NoPointIndex )
		maskImg = fixedMaskImg;
	else
	{
		dsInd = getDatasetIndexFromPointIndex(curInd);
		QString sliceFilename = getSliceFilename( m_maskNames[curInd], m_sliceNumPopupLst[dsInd] );
		if( !maskImg.load( sliceFilename, "PNG" ) )
			return;

		// Extract dataset and pipeline string
		m_currPrevDatasetName = sliceFilename.section( '/', -5, -5 ).section( '_', -1, -1 ).append( ".mhd" );
		QString mix = sliceFilename.section( '/', -5, -5 );
		mix.truncate( mix.lastIndexOf( '_' ) );
		m_currPrevPipelineName = mix;

		maskImg.setColor( 0, qRgba( 0, 0, 0, 0 ) );
		maskImg.setColor( 1, qRgba( 255, 0, 0, maskOpacity ) );
		maskImg.setColor( 2, qRgba( 0, 0, 255, maskOpacity ) );
		maskImg.setColor( 3, qRgba( 255, 255, 0, maskOpacity ) );

		if( m_fixedPointInd >= 0 && getDatasetIndexFromPointIndex(m_fixedPointInd) == dsInd )
		{
			uchar * maskPtr = maskImg.bits();
			uchar * fixedPtr = fixedMaskImg.bits();
			const long size = maskImg.height()*maskImg.width();
			long i;
			#pragma omp parallel for private(i)
			for( i = 0; i < size; ++i )
			{
				if( maskPtr[i] && !fixedPtr[i] )
					maskPtr[i] = 1;
				else if( !maskPtr[i] && fixedPtr[i] )
					maskPtr[i] = 2;
				else if( maskPtr[i] && fixedPtr[i] )
					maskPtr[i] = 3;
			}
		}
	}

	m_maskPxmp = QPixmap::fromImage( maskImg );
	QRect roi = m_roiLst[dsInd].toRect();
	m_maskPxmpRoi = m_maskPxmp.copy( roi ).scaledToWidth(settings.popupWidth);

	QString dataSliceFilename = getSliceFilename( m_datasets[dsInd], m_sliceNumPopupLst[dsInd] );
	QPixmap dataPixmap;
	if( !dataPixmap.load( dataSliceFilename, "PNG" ) )
		return;
	m_curSlicePxmp = dataPixmap.copy( roi ).scaledToWidth(settings.popupWidth);
	emit maskHovered( &m_maskPxmp, dsInd );
}

void iAPAQSplom::currentPointUpdated( int index )
{
	if( index >= 0 )
		m_fixAction->setVisible( true );
	else
		m_fixAction->setVisible( false );
	updatePreviewPixmap();
	iAQSplom::currentPointUpdated( index );
}

void iAPAQSplom::fixPoint()
{
	if( !m_activePlot )
		return;

	if( m_fixedPointInd != iAScatterPlot::NoPointIndex )
		removeFixedPoint();

	m_removeFixedAction->setVisible( true );
	m_fixedPointInd = m_activePlot->getCurrentPoint();
	addHighlightedPoint( m_fixedPointInd );
	updatePreviewPixmap();
}

void iAPAQSplom::removeFixedPoint()
{
	if( m_fixedPointInd != iAScatterPlot::NoPointIndex )
		removeHighlightedPoint( m_fixedPointInd );
	m_fixedPointInd = iAScatterPlot::NoPointIndex;
	m_removeFixedAction->setVisible( false );
	updatePreviewPixmap();
}
