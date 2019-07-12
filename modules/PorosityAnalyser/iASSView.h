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

#include "ui_SSView.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QColor>

class vtkTransform;
class vtkColorTransferFunction;
class vtkImageData;
class QTableWidget;
class iASSViewSettings;
class iASSSlicer;
class QHBoxLayout;
class iASegm3DView;
class vtkPolyData;
class iAVTKRendererManager;

typedef iAQTtoUIConnector<QDockWidget, Ui_SSView>  PorosityAnalyzerSSConnector;

class iASSView : public PorosityAnalyzerSSConnector
{
	Q_OBJECT

public:
	iASSView( QWidget * parent = 0, Qt::WindowFlags f = 0 );
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
	vtkSmartPointer<vtkTransform> m_slicerTransform;
	vtkSmartPointer<vtkColorTransferFunction> m_slicerTF;
	int m_modeInd;
	int m_deviationMode;
	QString m_datasetFolder;
	QString m_resultsFolder;
	QString m_datasetFile;

	QList<iASSSlicer*> m_slicerViews;
	QScopedPointer<iAVTKRendererManager> m_sliceMgr;
	vtkSmartPointer<vtkImageData> m_imgData;
	QScopedPointer<QHBoxLayout> m_slicerViewsLayout;
	iASegm3DView * m_segm3DViewExtrnl;
	int m_runsOffset;
};
