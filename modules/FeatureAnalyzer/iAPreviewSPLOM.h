// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include <QPixmap>

class iAPreviewSPLOM : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT

public:
	iAPreviewSPLOM(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~iAPreviewSPLOM();
	void SetPixmap( QPixmap * pxmp );
	void SetMask( const QPixmap * mask );
	void SetROI( const QRectF & roi );
	void ResetROI();
	QRectF GetROI() const;

protected:
	void initializeGL() override;
	void paintGL( ) override;				//!< Draws SPLOM.
	void resizeEvent( QResizeEvent * event ) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent( QMouseEvent * event ) override;

	void roiFromLocal();


	void CropPosByRect( QPoint & pos );
	void UpdateROI();
	void emitLocalROI();

	void updateLocRoi();

	void Scale();
	void updateOrigin();
	void ScalePixmap();
	void ScaleMask();
	QPointF posToPxmpPos( QPointF pos );
	QPointF pxmpPosToPos( QPointF pxmpPos );

signals:
	void roiChanged( QRectF roi );
	void sliceCountsChanged( QList<int> sliceCntLst );

protected:
	const QPixmap * m_pxmp;
	QPixmap m_scaledPxmp;
	QPixmap m_scaledMask;
	QPointF m_origin;
	QRectF m_pxmpRect;
	QPoint posStart, posEnd;
	QRectF m_roi, m_locRoi;
	bool m_mousePressed;
	const QPixmap * m_maskPtrExt;
};
