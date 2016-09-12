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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#pragma once

#include "iAQSplom.h"
#include <QStringList>

class QMenu;
class QAction;

class iAPAQSplom : public iAQSplom
{
	Q_OBJECT
public:
	iAPAQSplom( QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0 );
public:
	virtual void setData( const QTableWidget * data );
	void setPreviewSliceNumbers( QList<int> sliceNumber );
	void setROIList( QList<QRectF> roi );
	void setSliceCounts( QList<int> sliceCnts );
	void setDatasetsDir( QString datsetsDir );
	void setDatasetsByIndices ( QStringList selDatasets, QList<int> indices );
	void reemitFixedPixmap();

	int getDatasetIndexFromPointIndex(int pointIndex);

protected:
	virtual bool drawPopup( QPainter& painter );	//!< Draws popup on the splom with mask preview
	virtual void keyPressEvent( QKeyEvent * event );
	virtual void mouseReleaseEvent( QMouseEvent * event );
	virtual void mousePressEvent( QMouseEvent * event );
	void updatePreviewPixmap();

signals:
	void previewSliceChanged( int sliceNumber );
	void sliceCountChanged( int sliceCount );
	void maskHovered( const QPixmap * mask, int datasetIndex = -1 );

public slots:
	void removeFixedPoint();

protected slots:
	virtual void currentPointUpdated( int index );
	void fixPoint();

protected:
	QStringList m_maskNames;
	QList<int> m_sliceNumPopupLst;
	QList<int> m_sliceCntLst;
	QList<QRectF> m_roiLst;
	QPixmap m_maskPxmp, m_maskPxmpRoi;
	QPixmap m_curSlicePxmp;
	QString m_datasetsDir;
	QStringList m_datasets;
	QList<int> m_dsIndices;
	QList<int> m_datasetIndices;
	QMenu * m_contextMenu;
	QAction * m_fixAction, *m_removeFixedAction;
	int m_fixedPointInd;
	QPoint m_rightPressPos;
	QString m_currPrevDatasetName;
	QString m_currPrevPipelineName;
};