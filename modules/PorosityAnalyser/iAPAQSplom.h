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
#pragma once

#include <charts/iAQSplom.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QStringList>

class QMenu;
class QAction;
class Mainwindow;

class iAPAQSplom : public iAQSplom
{
	Q_OBJECT
public:
	iAPAQSplom(MainWindow *mainWind,  QWidget * parent = 0, Qt::WindowFlags f = 0 );
public:
	void setData( const QTableWidget * data ) override;
	void setPreviewSliceNumbers( QList<int> sliceNumber );
	void setROIList( QList<QRectF> roi );
	void setSliceCounts( QList<int> sliceCnts );
	void setDatasetsDir( QString datsetsDir );
	void setDatasetsByIndices ( QStringList selDatasets, QList<int> indices );
	void reemitFixedPixmap();

	int getDatasetIndexFromPointIndex(size_t pointIndex);

protected:
	bool drawPopup( QPainter& painter ) override;	//!< Draws popup on the splom with mask preview
	void keyPressEvent( QKeyEvent * event ) override;
	void updatePreviewPixmap();

signals:
	void previewSliceChanged( int sliceNumber );
	void sliceCountChanged( int sliceCount );
	void maskHovered( const QPixmap * mask, int datasetIndex = -1 );

public slots:
	void removeFixedPoint();
	void startFeatureScout();

private slots:
	void currentPointUpdated( size_t index ) override;
	void fixPoint();

	//send labeled image and csv from PA to FeatureScout
	void sendToFeatureScout();
	void getFilesLabeledFromPoint(QString & fileName, QString & mhdName);


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
	QAction * m_fixAction, * m_removeFixedAction;
	size_t m_fixedPointInd;

	//connecting to feature scout
	QAction * m_detailsToFeatureScoutAction;

	QPoint m_rightPressPos;
	QString m_currPrevDatasetName;
	QString m_currPrevPipelineName;

private:
	MainWindow * m_mainWnd;
	MdiChild * m_mdiChild;
	QString m_csvName;

};
