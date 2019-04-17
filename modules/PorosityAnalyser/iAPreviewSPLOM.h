/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <QtGlobal>
#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif
#include <QPixmap>

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
class iAPreviewSPLOM : public QOpenGLWidget
#else
class iAPreviewSPLOM : public QGLWidget
#endif
{
	Q_OBJECT

public:
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
	iAPreviewSPLOM(QWidget * parent = 0, Qt::WindowFlags f = 0);
#else
	iAPreviewSPLOM(QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0);
#endif
	~iAPreviewSPLOM();
	void SetPixmap( QPixmap * pxmp );
	void SetMask( const QPixmap * mask );
	void SetROI( const QRectF & roi );
	void ResetROI();
	QRectF GetROI() const;

protected:
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
