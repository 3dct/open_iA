// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPreviewSPLOM.h"

#include <QMouseEvent>
#include <QPainter>

#include <cmath>

const double penWidth = 2.0;
const QColor bgrCol( 50, 50, 50 );
const QColor roiCol( 255, 0, 0, 240 );

iAPreviewSPLOM::iAPreviewSPLOM(QWidget * parent, Qt::WindowFlags f) :
	QOpenGLWidget(parent),
	m_pxmp( nullptr ),
	m_mousePressed( false ),
	m_maskPtrExt( nullptr )
{
	setWindowFlags(f);
}

iAPreviewSPLOM::~iAPreviewSPLOM()
{}

void iAPreviewSPLOM::SetPixmap( QPixmap * pxmp )
{
	m_pxmp = pxmp;
	Scale();
	update();
}

void iAPreviewSPLOM::SetMask( const QPixmap * mask )
{
	m_maskPtrExt = mask;
	ScaleMask();
	update();
}

void iAPreviewSPLOM::SetROI( const QRectF & roi )
{
	m_locRoi = roi;
	roiFromLocal();
	update();
}

void iAPreviewSPLOM::ResetROI()
{
	m_roi = rect();
	updateLocRoi();
}

QRectF iAPreviewSPLOM::GetROI() const
{
	return m_locRoi;
}

void iAPreviewSPLOM::initializeGL()
{
	initializeOpenGLFunctions();
}

void iAPreviewSPLOM::paintGL( )
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.fillRect( rect(), bgrCol );
	if( !m_pxmp )
		return;

	painter.drawPixmap( m_origin, m_scaledPxmp );
	if( m_maskPtrExt )
		painter.drawPixmap( m_origin, m_scaledMask );
	QPen p( roiCol ); p.setWidthF( penWidth ); p.setJoinStyle( Qt::MiterJoin );
	painter.setPen( p );
	QRectF rect;
	rect.setLeft( m_roi.left() + 0.5*penWidth );
	rect.setRight( m_roi.right() - 0.5*penWidth );
	rect.setTop( m_roi.top() + 0.5*penWidth );
	rect.setBottom( m_roi.bottom() - 0.5*penWidth );
	painter.drawRect( rect );
}

void iAPreviewSPLOM::resizeEvent( QResizeEvent * event )
{
	QOpenGLWidget::resizeEvent(event);
	if( !m_pxmp )
		return;
	Scale();
	roiFromLocal();
	emitLocalROI();
	update();
}

void iAPreviewSPLOM::roiFromLocal()
{
	m_roi.setTopLeft( pxmpPosToPos( m_locRoi.topLeft() ) );
	m_roi.setBottomRight( pxmpPosToPos( m_locRoi.bottomRight() ) );
}

void iAPreviewSPLOM::CropPosByRect( QPoint & pos )
{
	if( pos.x() < m_pxmpRect.left() ) pos.setX( m_pxmpRect.left() );
	if( pos.x() > m_pxmpRect.right() ) pos.setX( m_pxmpRect.right() );
	if( pos.y() < m_pxmpRect.top() ) pos.setY( m_pxmpRect.top() );
	if( pos.y() > m_pxmpRect.bottom() ) pos.setY( m_pxmpRect.bottom() );
}

void iAPreviewSPLOM::UpdateROI()
{
	m_roi.setLeft( std::fmin( posStart.x(), posEnd.x() ) );
	m_roi.setTop( std::fmin( posStart.y(), posEnd.y() ) );
	m_roi.setRight( std::fmax( posStart.x(), posEnd.x() ) );
	m_roi.setBottom( std::fmax( posStart.y(), posEnd.y() ) );
	updateLocRoi();
	update();
}

void iAPreviewSPLOM::mousePressEvent( QMouseEvent * event )
{
	if( event->button() == Qt::LeftButton )//selection
	{
		m_mousePressed = true;
		posStart = event->pos();
		CropPosByRect( posStart );
		posEnd = posStart;
		UpdateROI();
	}
}

void iAPreviewSPLOM::mouseReleaseEvent( QMouseEvent * event )
{
	if( event->button() == Qt::LeftButton )//selection
	{
		m_mousePressed = false;
		posEnd = event->pos();
		CropPosByRect( posEnd );
		UpdateROI();
		emitLocalROI();
	}
}

void iAPreviewSPLOM::emitLocalROI()
{
	emit roiChanged( m_locRoi );
}

void iAPreviewSPLOM::updateLocRoi()
{
	m_locRoi.setTopLeft( posToPxmpPos( m_roi.topLeft() ) );
	m_locRoi.setBottomRight( posToPxmpPos( m_roi.bottomRight() ) );
}

void iAPreviewSPLOM::mouseMoveEvent( QMouseEvent * event )
{
	if( m_mousePressed )
	{
		posEnd = event->pos();
		CropPosByRect( posEnd );
		UpdateROI();
	}
}

void iAPreviewSPLOM::Scale()
{
	ScaleMask();
	ScalePixmap();
	if( !m_pxmp )
		return;
	updateOrigin();
	m_pxmpRect = QRectF( m_origin, QSizeF( m_scaledPxmp.size() ) );
}

void iAPreviewSPLOM::updateOrigin()
{
	m_origin = QPointF( 0.0, 0.0 );
	m_origin.setX( 0.5*( width() - m_scaledPxmp.width() ) );
	m_origin.setY( 0.5*( height() - m_scaledPxmp.height() ) );
}

void iAPreviewSPLOM::ScalePixmap()
{
	if( !m_pxmp )
		return;
	m_scaledPxmp = m_pxmp->copy().scaled( size(), Qt::KeepAspectRatio );
}

void iAPreviewSPLOM::ScaleMask()
{
	if( !m_maskPtrExt )
		return;
	m_scaledMask = m_maskPtrExt->copy().scaled( size(), Qt::KeepAspectRatio );
}

QPointF iAPreviewSPLOM::posToPxmpPos( QPointF pos )
{
	QPointF res = pos - m_origin;
	if( !m_pxmp )
		return res;
	double scale = 1.0;
	double aspectPxmp = m_pxmp->width() / ( (double)m_pxmp->height() );
	double aspectWgt = width() / ( (double)height() );
	if( aspectPxmp < aspectWgt )
		scale = m_pxmp->height() / ( (double)height() );
	else
		scale = m_pxmp->width() / ( (double)width() );
	res *= scale;
	if( res.x() < 0.0 ) res.setX( 0.0 );
	if( res.y() < 0.0 ) res.setY( 0.0 );
	if( res.x() > m_pxmp->width() ) res.setX( m_pxmp->width() );
	if( res.y() > m_pxmp->height() ) res.setY( m_pxmp->height() );
	return res;
}

QPointF iAPreviewSPLOM::pxmpPosToPos( QPointF pxmpPos )
{
	double scale = 1.0;
	double aspectPxmp = m_pxmp->width() / ( (double)m_pxmp->height() );
	double aspectWgt = width() / ( (double)height() );
	if( aspectPxmp < aspectWgt )
		scale = m_pxmp->height() / ( (double)height() );
	else
		scale = m_pxmp->width() / ( (double)width() );
	QPointF res = pxmpPos;
	res /= scale;
	res += m_origin;
	return res;
}
