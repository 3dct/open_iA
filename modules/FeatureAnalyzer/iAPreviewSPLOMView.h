// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_PreviewSPLOM.h"

#include <iAQTtoUIConnector.h>

#include <QDockWidget>
#include <QScopedPointer>

class iAPreviewSPLOM;
class QPixmap;

typedef iAQTtoUIConnector<QDockWidget, Ui_PreviewSPLOM>  PreviewSPLOMConnector;

class iAPreviewSPLOMView : public PreviewSPLOMConnector
{
	Q_OBJECT
public:
	iAPreviewSPLOMView(QWidget* parent = nullptr);
	void SetDatasetsDir( const QString & datasetsFolder );
	~iAPreviewSPLOMView();

public slots:
	void SetDatasets( QStringList datasets );
	void SliceNumberChanged();
	void SetSliceCount( int sliceCnt );
	void LoadDatasets();
	void clear();
	void SetMask( const QPixmap * mask, int datasetIndex = -1 );
	void SetSlice( int slice );

protected slots:
	void ROIChangedSlot( QRectF roi );
	void UpdatePixmap();
	void DatasetChanged();

signals:
	void roiChanged( QList<QRectF> roi );
	void sliceCountsChanged( QList<int> sliceCntLst );
	void sliceNumbersChanged( QList<int> sliceNumberLst );

protected:
	QString m_datasetsDir;
	QStringList m_datasets;
	QList<int> m_sliceNumberLst;
	QList<int> m_sliceCntLst;
	iAPreviewSPLOM * m_preview;
	QScopedPointer<QPixmap> m_slicePxmp;
	QList<QRectF> m_roiList;
	bool m_datasetsLoaded;
};
