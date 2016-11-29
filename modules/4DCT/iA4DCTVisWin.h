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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#ifndef IA4DCTVISWIN_H
#define IA4DCTVISWIN_H
// Ui
#include "ui_iA4DCTVisWin.h"
// iA
#include "iAQTtoUIConnector.h"
#include "iAVisModulesCollection.h"
// vtk
#include <vtkOrientationMarkerWidget.h>
#include <vtkSmartPointer.h>
// Qt
#include <QDockWidget>
#include <QMainWindow>
#include <QSharedPointer>
#include <QTimer>

class QString;
class iA4DCTAllVisualizationsDockWidget;
class iA4DCTBoundingBoxDockWidget;
class iA4DCTDefectVisDockWidget;
class iA4DCTFileData;
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
						iA4DCTVisWin( iA4DCTMainWin * parent = 0 );
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
	vtkSmartPointer<vtkGenericOpenGLRenderWindow>	m_renderWindow;
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

#endif // IA4DCTVISWIN_H