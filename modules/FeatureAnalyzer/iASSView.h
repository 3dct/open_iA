// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_SSView.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QColor>

class vtkColorTransferFunction;
class vtkImageData;

class iARendererViewSync;
class iASegm3DView;
class iASSSlicer;
class iASSViewSettings;

class QHBoxLayout;
class QTableWidget;

typedef iAQTtoUIConnector<QDockWidget, Ui_SSView>  FeatureAnalyzerSSConnector;

class iASSView : public FeatureAnalyzerSSConnector
{
	Q_OBJECT

public:
	iASSView(QWidget* parent = nullptr);
	~iASSView();
	void attachSegm3DView( iASegm3DView * m_segm3DView );

protected slots:
	void setSliceSpinBox( int sn );
	void setSliceScrollBar( int sn );
	void setSlicerDirection( int cbIndex );
	void setShowMasks( int state );
	void setMasksOpacity( int sliderVal );
	void setShowGT( int state );
	void setGTOpacity( int sliderVal );
	void setShowContours( int state );
	void showSettings();
	void setShowVolume( int state );
	void setShowSurface( int state );
	void setShowWireframe( int state );
	void setDeviationMode( int mode );
	void setSensitivity( int val );
	void updateSettings();

public slots:
	void SetData( const QTableWidget * data, QString selText );
	void SetDataTo3D();
	void SetCompareData( const QList< QPair<QTableWidget *, QString> > * dataList );
	void setRunsOffset( int offset );

protected:
	void BuildDefaultTF( vtkSmartPointer<vtkImageData> & imgData, vtkSmartPointer<vtkColorTransferFunction> & tf, QColor color = Qt::white );
	void InitializeGUI();
	void LoadDataToSlicer( iASSSlicer * slicer, const QTableWidget * data );
	void UpdatePolyData();
	bool NeedsPolyData();
	bool NeedsDistances();

protected:
	iASSViewSettings * m_SSViewSettings;
	vtkSmartPointer<vtkColorTransferFunction> m_slicerTF;
	int m_modeInd;
	int m_deviationMode;
	QString m_datasetFolder;
	QString m_resultsFolder;
	QString m_datasetFile;

	QList<iASSSlicer*> m_slicerViews;
	QScopedPointer<iARendererViewSync> m_sliceMgr;
	vtkSmartPointer<vtkImageData> m_imgData;
	QScopedPointer<QHBoxLayout> m_slicerViewsLayout;
	iASegm3DView * m_segm3DViewExtrnl;
	int m_runsOffset;
};
