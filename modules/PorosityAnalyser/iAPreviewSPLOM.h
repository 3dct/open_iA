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
 
#pragma once

#include <QGLWidget>
#include <QPixmap>

class iAPreviewSPLOM : public QGLWidget
{
	Q_OBJECT

public:
	iAPreviewSPLOM( QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0 );
	~iAPreviewSPLOM();
	void SetPixmap( QPixmap * pxmp );
	void SetMask( const QPixmap * mask );
	void SetROI( const QRectF & roi );
	void ResetROI();
	QRectF GetROI() const;

protected:
	virtual void paintEvent( QPaintEvent * event );				//!< Draws SPLOM. Re-implements QGLWidget.
	void resizeEvent( QResizeEvent * event );

	void roiFromLocal();

	void mousePressEvent( QMouseEvent * event );
	void CropPosByRect( QPoint & pos );
	void UpdateROI();
	void mouseReleaseEvent( QMouseEvent * event );
	void emitLocalROI();

	void updateLocRoi();

	void mouseMoveEvent( QMouseEvent * event );
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