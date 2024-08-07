// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADropPipelineWidget.h"

#include "dlg_ParamSpaceSampling.h"
#include "FeatureAnalyzerHelpers.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QSettings>
#include <QTextDocument>

namespace
{
	const QStringList customDropMimeType = QStringList() << "application/x-dnditemdatafilter"
														 << "application/x-dnditemdatadataset"
														 << "application/x-dnditeminternaldatafilter"
														 << "application/x-dnditeminternaldatadataset";

	const QString pipelinePresetsPath = "FeatureAnalyzer/PipelinePresets/";
}

template<class T>
void resizeList( QList<T> & list, int newSize )
{
	int diff = newSize - list.size();
	T t = T();
	if ( diff > 0 )
	{
		list.reserve(newSize);
		while (diff--)
		{
			list.append(t);
		}
	}
	else if ( diff < 0 )
	{
		list.erase(list.end() + diff, list.end());
	}
}

iADropPipelineWidget::iADropPipelineWidget(int imageSize, int totalPipelineSlots, QString datasetDir, QWidget *parent ):
	QWidget(parent),
	m_datasetDir(datasetDir),
	m_imageSize(imageSize),
	m_totalPipelineSlots(totalPipelineSlots)
{
	setAcceptDrops( true );
	setFixedSize( m_imageSize, pieceSize() );
	resizePieceLists();
}

void iADropPipelineWidget::clearAllSlots()
{
	for ( int i = 0; i < m_totalPipelineSlots; ++i )
	{
		pieceName[i] = QString();
		pieceParams[i] = QStringList();
		pieceLastPos[i] = std::numeric_limits<int>::min();
		piecePixmaps[i] = QPixmap();
		pieceRects[i] = QRect();
		pieceDescription[i] = QString();
	}
	update();
}

void iADropPipelineWidget::dragEnterEvent( QDragEnterEvent *event )
{
	if ( event->mimeData()->hasFormat( customDropMimeType[0] )
		 || event->mimeData()->hasFormat( customDropMimeType[1] )
		 || event->mimeData()->hasFormat( customDropMimeType[2] )
		 || event->mimeData()->hasFormat( customDropMimeType[3] ) )
		 event->accept();
	else
		event->ignore();
}

void iADropPipelineWidget::dragLeaveEvent( QDragLeaveEvent *event )
{
	QRect updateRect = highlightedRect;
	highlightedRect = QRect();
	update( updateRect );
	event->accept();
}

void iADropPipelineWidget::dragMoveEvent( QDragMoveEvent *event )
{
	QRect updateRect = highlightedRect.united( targetSquare( event->position().toPoint() ) );
	QRect square = targetSquare( event->position().toPoint() );
	int pipePos = square.x() / pieceSize();

	if ( ( event->mimeData()->hasFormat( customDropMimeType[1] )
		|| event->mimeData()->hasFormat( customDropMimeType[3] ) )
		&& pipePos == 0 )
	{
		highlightedRect = targetSquare( event->position().toPoint() );
		event->setDropAction( Qt::MoveAction );
		event->accept();
	}
	else if ( ( event->mimeData()->hasFormat( customDropMimeType[0] )
		|| event->mimeData()->hasFormat( customDropMimeType[2] ) )
		&& pipePos > 0
		&& pipePos <= ( imageSize() / pieceSize() ) )
	{
		highlightedRect = targetSquare( event->position().toPoint() );
		event->setDropAction( Qt::MoveAction );
		event->accept();
	}
	else
	{
		highlightedRect = QRect();
		event->ignore();
	}
	update( updateRect );
}

void iADropPipelineWidget::dropEvent( QDropEvent *event )
{
	if ( !( event->mimeData()->hasFormat( customDropMimeType[0] )
		|| event->mimeData()->hasFormat( customDropMimeType[1] )
		|| event->mimeData()->hasFormat( customDropMimeType[2] )
		|| event->mimeData()->hasFormat( customDropMimeType[3] ) ) )
	{
		highlightedRect = QRect();
		event->ignore();
		return;
	}

	QString currentFormat = event->mimeData()->formats()[0];
	QByteArray pieceData = event->mimeData()->data( currentFormat );
	QDataStream dataStream( &pieceData, QIODevice::ReadOnly );
	QRect square = targetSquare( event->position().toPoint() );
	QPixmap pixmap;
	int lastPiecePos;
	QString name;
	QStringList params;
	QString description;
	dataStream >> pixmap >> lastPiecePos >> name >> description >> params;

	int dropPiecePos = square.x() / pieceSize();
	if ( dropPiecePos == lastPiecePos )
	{
		piecePixmaps.replace( dropPiecePos, pixmap );
		highlightedRect = QRect();
		update( square );
		return;
	}

	pieceName.replace( dropPiecePos, name );
	pieceParams.replace( dropPiecePos, params );
	pieceLastPos.replace( dropPiecePos, lastPiecePos );
	piecePixmaps.replace( dropPiecePos, pixmap );
	pieceRects.replace( dropPiecePos, square );
	pieceDescription.replace( dropPiecePos, description );

	highlightedRect = QRect();
	update( square );

	if ( event->mimeData()->hasFormat( customDropMimeType[0] )
		 || event->mimeData()->hasFormat( customDropMimeType[1] ) )
		 event->setDropAction( Qt::CopyAction );
	else
		event->setDropAction( Qt::MoveAction );

	event->accept();
}

int iADropPipelineWidget::findPiece( const QRect &pieceRect ) const
{
	for ( int i = 0; i < pieceRects.size(); ++i )
	{
		if ( pieceRect == pieceRects[i] )
			return i;
	}

	return -1;
}

void iADropPipelineWidget::mousePressEvent( QMouseEvent *event )
{
	QRect square = targetSquare( event->pos() );
	int found = findPiece( square );
	int pipePos = square.x() / pieceSize();

	if ( found == -1 )
		return;

	if ( ( event->modifiers() & Qt::ControlModifier )	// edit piece parameters
		 && event->button() == Qt::LeftButton )
	{
		if ( pipePos == 0 )
			return;

		QList<PorosityFilterID> filterIds = parseFiltersFromString( pieceName[found] );
		QList<ParamNameType> paramsNameType;
		for (PorosityFilterID fid: filterIds)
		{
			paramsNameType.append(FilterIdToParamList[fid]);
		}

		QSettings settings;
		QString filterName = pieceName[found],
			dialogWindowName = paramsNameType.size() > 0
			? pieceName[found] + " Parameters"
			: pieceName[found] + " Parameter";
		filterName.remove( " " );
		QStringList inList, unionList;
		QList<QVariant> inPara;
		inList.append( "$Random Sampling" );
		inPara.append( false );

		for ( int i = 0; i < paramsNameType.size(); ++i )
		{
			QString filterParam = "#" + paramsNameType[i].name() + " Start";
			inList.append( filterParam );
			QString nosnows_filterParam = filterParam;
			nosnows_filterParam.remove( " " );
			inPara.append( settings.value( pipelinePresetsPath +
				filterName + "/" + nosnows_filterParam ).toDouble() );

			filterParam = "#" + paramsNameType[i].name() + " End";
			inList.append( filterParam );
			nosnows_filterParam = filterParam;
			nosnows_filterParam.remove( " " );
			inPara.append( settings.value( pipelinePresetsPath +
				filterName + "/" + nosnows_filterParam ).toDouble() );

			filterParam = "#" + paramsNameType[i].name() + " Samples";
			inList.append( filterParam );
			nosnows_filterParam = filterParam;
			nosnows_filterParam.remove( " " );
			inPara.append( settings.value( pipelinePresetsPath +
				filterName + "/" + nosnows_filterParam ).toDouble() );
		}

		// Dialog Param Space Sampling preparations
		QTextDocument algoDoc;
		algoDoc.setHtml( pieceDescription[found] );

		// Find datasetName in the pieceName list
		int datasetNameIdx = -1;
		for ( int i = 0; i < pieceName.size(); ++i )
		{
			if ( pieceName[i].left( 8 ) == "dataset_" )
				datasetNameIdx = i;
		}

		// No parameter setting without a dataset
		if ( datasetNameIdx == -1 )
		{
			QMessageBox msgBox;
			msgBox.setText( "No dataset. First pipeline position must be a dataset." );
			msgBox.setWindowTitle( "FeatureAnalyzer" );
			msgBox.exec();
			return;
		}

		QString datasetName = pieceName[datasetNameIdx];
		datasetName.remove( "dataset_" );
		QStringList datasetInfo = getDatasetInfo( m_datasetDir, datasetName );
		QVector<double> bin, freq;
		getDatasetHistogramValues( m_datasetDir, datasetName, bin, freq );

		dlg_ParamSpaceSampling dlg( this, dialogWindowName, inList.size(), inList,
									inPara, &algoDoc, m_datasetDir, datasetName,
									datasetInfo, bin, freq, filterName );

		if ( dlg.exec() != QDialog::Accepted )
			return;

		for ( int j = 0; j < inList.size(); ++j )
		{
			QString nosnows_filterParam = inList[j];
			nosnows_filterParam.remove( " " );
			double value = dlg.getValue(j);
			unionList.append( inList[j] + " " + QString::number( value ) );
			settings.setValue( pipelinePresetsPath + filterName + "/" + nosnows_filterParam, value );
		}

		pieceParams[found] = unionList;
	}
	else if ( event->button() == Qt::LeftButton )	// start click position drag piece
	{
		startPos = event->pos();
	}
	else if ( event->button() == Qt::RightButton )	//remove piece from pipeline
	{
		pieceName[found] = QString();
		pieceParams[found] = QStringList();
		pieceLastPos[found] = std::numeric_limits<int>::min();
		piecePixmaps[found] = QPixmap();
		pieceRects[found] = QRect();
		pieceDescription[found] = QString();
		highlightedRect = QRect();
		update( square );
	}
	QWidget::mousePressEvent( event );
}

void iADropPipelineWidget::mouseMoveEvent( QMouseEvent *event )
{
	if ( event->buttons() & Qt::LeftButton )
	{
		QRect square = targetSquare( event->pos() );
		int found = findPiece( square );
		if (found == -1)
		{
			return;
		}
		int distance = ( event->pos() - startPos ).manhattanLength();
		if ( distance >= QApplication::startDragDistance() )
		{
			QString name = pieceName[found];
			QStringList params = pieceParams[found];
			int piecePos = found;
			QPixmap pixmap = piecePixmaps[found];
			QString description = pieceDescription[found];

			QByteArray itemData;
			QDataStream dataStream( &itemData, QIODevice::WriteOnly );
			dataStream << pixmap << piecePos << name << description << params;
			QMimeData *mimeData = new QMimeData;

			if ( name.startsWith("dataset_") )
				mimeData->setData( customDropMimeType[3], itemData );
			else
				mimeData->setData( customDropMimeType[2], itemData );

			QDrag *drag = new QDrag( this );
			drag->setMimeData( mimeData );
			drag->setPixmap( pixmap );
			piecePixmaps[found] = QPixmap();	// removes icon-piece while drag
			if ( drag->exec( Qt::MoveAction ) == Qt::MoveAction )
			{
				pieceName[found] = QString();
				pieceParams[found] = QStringList();
				pieceLastPos[found] = std::numeric_limits<int>::min();
				piecePixmaps[found] = QPixmap();
				pieceRects[found] = QRect();
				pieceDescription[found] = QString();
				update( square );
			}
		}
	}
	QWidget::mouseMoveEvent( event );
}

void iADropPipelineWidget::paintEvent( QPaintEvent *event )
{
	QPainter painter;
	painter.begin( this );
	painter.fillRect( event->rect(), Qt::lightGray );

	if ( highlightedRect.isValid() )
	{
		painter.setBrush( QColor( "#e6f5d0" ) );
		painter.setPen( Qt::SolidLine );
		painter.drawRect( highlightedRect.adjusted( 0, 0, -1, -1 ) );
	}

	for ( int i = 0; i < pieceRects.size(); ++i )
		painter.drawPixmap( pieceRects[i], piecePixmaps[i] );

	painter.end();
}

const QRect iADropPipelineWidget::targetSquare( const QPoint &position ) const
{
	return QRect( position.x() / pieceSize() * pieceSize(),
				  position.y() / pieceSize() * pieceSize(),
				  pieceSize(), pieceSize() );
}

int iADropPipelineWidget::pieceSize() const
{
	return m_imageSize / m_totalPipelineSlots;
}

int iADropPipelineWidget::imageSize() const
{
	return m_imageSize;
}

QList<QList<QString>> iADropPipelineWidget::getPipeline()
{
	QList<QList<QString>> pipeline;
	for ( int i = 0; i < pieceName.size(); ++i )
	{
		QList<QString> pipelineSlot;
		pipelineSlot.append( pieceName[i] );
		pipelineSlot.append( pieceParams[i] );
		pipeline.append( pipelineSlot );
	}

	return pipeline;
}

void iADropPipelineWidget::updatePipelineSlots( int pipelineSlotsCount, int pipelineSlotsIconWidth )
{
	m_totalPipelineSlots = pipelineSlotsCount;
	m_imageSize = pipelineSlotsCount * pipelineSlotsIconWidth;
	setFixedWidth( m_imageSize );
	resizePieceLists();
}

bool iADropPipelineWidget::isLastPipelineSlotEmpty()
{
	for ( int i = 0; i < pieceRects.size(); ++i )
	{
		if ( pieceRects[i].x() == m_imageSize - pieceSize() )
			return false;
	}

	return true;
}

void iADropPipelineWidget::resizePieceLists()
{
	resizeList( pieceName, m_totalPipelineSlots );
	resizeList( pieceParams, m_totalPipelineSlots );
	resizeList( piecePixmaps, m_totalPipelineSlots );
	resizeList( pieceRects, m_totalPipelineSlots );
	resizeList( pieceLastPos, m_totalPipelineSlots );
	resizeList( pieceDescription, m_totalPipelineSlots );
}
