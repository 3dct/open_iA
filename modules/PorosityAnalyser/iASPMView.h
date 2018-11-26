/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QDockWidget>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include "ui_SPMView.h"
#include "qthelper/iAQTtoUIConnector.h"

class QAction;
class QCheckBox;
class QTableWidget;
class QVBoxLayout;

#include <QtGlobal>
#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
class QVTKOpenGLWidget;
#else
class QVTKWidget;
#endif
class vtkColorTransferFunction;
class vtkCommand;
class vtkContextView;
class vtkIdTypeArray;
class vtkLookupTable;
class vtkRenderer;
class vtkScalarsToColors;
class vtkScalarBarActor;
class vtkScatterPlotMatrix;
class vtkSelection;
class vtkTable;

class iAPAQSplom;
struct iASelection;
class iASPMSettings;
class MainWindow; 

typedef iAQTtoUIConnector<QDockWidget, Ui_SPMView>  PorosityAnalyzerSPMConnector;

class iASPMView : public PorosityAnalyzerSPMConnector
{
	Q_OBJECT

public:
	iASPMView(MainWindow *mWnd, QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iASPMView();
	void setSelection( iASelection * sel );
	void setDatasetsDir( QString datasetsDir );

public slots:
	void setData( const QTableWidget * data );
	void setRSDSelection( vtkIdTypeArray * );
	void setSPLOMPreviewSliceNumbers( QList<int> sliceNumberLst );
	void setSPLOMPreviewSize( int percent );
	void setROIList( QList<QRectF> roi);
	void setSliceCnts( QList<int> sliceCnts );
	void setDatasetsByIndices( QStringList selDatasets, QList<int> indices );
	void reemitFixedPixmap();

protected:
	void updateLUT();

	/** Set selection to SPLOM from VTK array of id-s */
	void setSPLOMSelection( vtkIdTypeArray * ids );

	/** Initialize scalar bar widget which shows color coding */
	void initScalarBar();

	/** Active plot indices */
	vtkVector2i getActivePlotIndices();

protected slots:
	
	/** Apply lookup table to all the plots in the SPM */
	void applyLookupTable();

	/** When selection of the SPLOM is modified */
	void selectionUpdated( std::vector<size_t> const & selInds );

signals:
	void selectionModified( vtkVector2i, vtkIdTypeArray* );
	void previewSliceChanged( int sliceNumber );
	void sliceCountChanged( int sliceCount );
	void maskHovered( const QPixmap * mask, int datasetIndex = -1 );

protected:
	iAPAQSplom * m_splom;
	vtkSmartPointer<vtkIdTypeArray> m_SPLOMSelection;
	vtkSmartPointer<vtkLookupTable> m_lut;
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
	QVTKOpenGLWidget * m_SBQVTKWidget;
#else
	QVTKWidget * m_SBQVTKWidget;
#endif
	vtkSmartPointer<vtkRenderer> m_sbRen;
	vtkSmartPointer<vtkScalarBarActor> m_sbActor;
};
