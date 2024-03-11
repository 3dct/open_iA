// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTVisWin.h"
// iA
#include "iAVisModulesCollection.h"
// vtk
#include <vtkOrientationMarkerWidget.h>
#include <vtkSmartPointer.h>
// Qt
#include <QMainWindow>
#include <QTimer>

class QString;
class iA4DCTAllVisualizationsDockWidget;
class iA4DCTBoundingBoxDockWidget;
class iA4DCTDefectVisDockWidget;
struct iA4DCTFileData;
class iA4DCTFractureVisDockWidget;
class iA4DCTMainWin;
class iA4DCTPlaneDockWidget;
class iA4DCTRegionViewDockWidget;
class iA4DCTToolsDockWidget;
class iABoundingBoxVisModule;
class iADefectVisModule;
class iAVisModule;
class vtkOrientationMarkerWidget;
class vtkRenderer;
class vtkRendererCollection;

const float SCENE_SCALE = 0.01;

class iA4DCTVisWin : public QMainWindow, public Ui::VisWin
{
	Q_OBJECT
public:
						iA4DCTVisWin(iA4DCTMainWin* parent = nullptr);
						~iA4DCTVisWin( );
	void				setImageSize( double * size );
	void				setNumberOfStages( int number );

	bool				showDialog( iA4DCTFileData & fileData );

public slots:
	void				updateRenderWindow( );
	void				selectedVisModule( iAVisModule * visModule );
	void				updateVisualizations( );
	void				changeBackground( QColor col );


private:
	void				setToolsDockWidgetsEnabled( bool enabled );

	vtkSmartPointer<vtkRenderer>			m_mainRen;	// ToDo: renderer into iAFast3DMagicLensWidget?
	vtkSmartPointer<vtkRendererCollection>	m_renList;
	vtkGenericOpenGLRenderWindow*	m_renderWindow;
	vtkSmartPointer<vtkOrientationMarkerWidget>		m_orientWidget;
	vtkRenderer *							m_magicLensRen;
	double									m_size[3];
	int										m_currentStage;
	QTimer									m_timer;
	iA4DCTMainWin *							m_mainWin;
	iAVisModulesCollection					m_visModules;

	// dock widgets (prefix: dw)
	iA4DCTFractureVisDockWidget *			m_dwFractureVis;
	iA4DCTPlaneDockWidget *					m_dwPlane;
	iA4DCTAllVisualizationsDockWidget *		m_dwAllVis;
	iA4DCTRegionViewDockWidget *			m_dwRegionVis;
	iA4DCTBoundingBoxDockWidget *			m_dwBoundingBox;
	iA4DCTDefectVisDockWidget *				m_dwDefectVis;
	iA4DCTToolsDockWidget *					m_dwTools;

	bool									m_isVirgin;


private slots:
	// GUI
	void				onStageSliderValueChanged( int val );
	void				onFirstButtonClicked( );
	void				onPreviousButtonClicked( );
	void				onNextButtonClicked( );
	void				onLastButtonClicked( );
	void				onPlayButtonClicked( bool checked );
	void				onIntervalValueChanged( int val );
	void				enableSideBySideView( bool enabled );

	// camera
	void				resetCamera( );
	void				setXYView( );
	void				setXZView( );
	void				setYZView( );
	void				setXYBackView( );
	void				setXZBackView( );
	void				setYZBackView( );

	void				setOrientationWidgetEnabled( bool enabled );

	// fracture vis
	void				onLoadButtonClicked( );
	void				onExtractButtonClicked( );
	// defect density maps
	void				addSurfaceVis( );
	// bounding box
	void				addBoundingBox( );
	// defect view
	void				addDefectView( );
	void				addDefectVis( );
	// magic lens
	void				enableMagicLens( bool enable );
};
