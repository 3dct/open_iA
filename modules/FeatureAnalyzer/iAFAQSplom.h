// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAQSplom.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <QStringList>

class QMenu;
class QAction;
class Mainwindow;

class QTableWidget;

class iAFAQSplom : public iAQSplom
{
	Q_OBJECT
public:
	iAFAQSplom(iAMainWindow * mainWind,  QWidget * parent = nullptr);
public:
	void setData( const QTableWidget * data );
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
	iAMainWindow * m_mainWnd;
	iAMdiChild * m_mdiChild;
	QString m_csvName;

};
