/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAQTtoUIConnector.h"

class QVBoxLayout;
class QCheckBox;
class QListWidgetItem;
class QAction;
class QTableWidget;

class QVTKWidget;
class vtkScatterPlotMatrix;
class vtkObject;
class vtkCommand;
class vtkTable;
class vtkIdTypeArray;
class vtkScatterPlotMatrix;
class vtkContextView;
class vtkLookupTable;
class vtkColorTransferFunction;
class vtkScalarsToColors;
class vtkScalarBarActor;
class vtkRenderer;
class vtkSelection;
struct iASelection;
class iASPMSettings;
class iAPAQSplom;

typedef iAQTtoUIConnector<QDockWidget, Ui_SPMView>  PorosityAnalyzerSPMConnector;

class iASPMView : public PorosityAnalyzerSPMConnector
{
	Q_OBJECT

public:
	iASPMView( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iASPMView();
	void setSelection( iASelection * sel );
	void setDatasetsDir( QString datasetsDir );

public slots:
	void SetData( const QTableWidget * data );
	void showSettings();
	void setRSDSelection( vtkIdTypeArray * );
	void setSPLOMPreviewSliceNumbers( QList<int> sliceNumberLst );
	void setSPLOMPreviewSize( int percent );
	void setROIList( QList<QRectF> roi);
	void setSliceCnts( QList<int> sliceCnts );
	void setDatasetsByIndices( QStringList selDatasets, QList<int> indices );
	void reemitFixedPixmap();

protected:
	void UpdateLUTOpacity();

	/** Set selection to SPLOM from VTK array of id-s */
	void setSPLOMSelection( vtkIdTypeArray * ids );

	/** Initialize Lookup Table */
	void InitLUT();

	/** Initialize scalar bar widget which shows color coding */
	void InitScalarBar();

	/** Active plot indices */
	vtkVector2i getActivePlotIndices();

protected slots:
	/** Show/hide a parameter in SPLOM when list widget item is clicked */
	void changeColumnVisibility( QListWidgetItem * item );
	
	/** Apply lookup table to all the plots in the SPM */
	void ApplyLookupTable();

	/** Apply color coding based on the parameter name */
	void SetParameterToColorcode( const QString & paramName);

	/** Update lookup table sensitivity */
	void UpdateLookupTable();

	/** When selection of the SPLOM is modified */
	void selectionUpdated( QVector<unsigned int>* selInds );

signals:
	void selectionModified( vtkVector2i, vtkIdTypeArray* );
	void previewSliceChanged( int sliceNumber );
	void sliceCountChanged( int sliceCount );
	void maskHovered( const QPixmap * mask, int datasetIndex = -1 );

protected:
	iASPMSettings * m_SPMSettings;
	iAPAQSplom * m_splom;
	vtkSmartPointer<vtkIdTypeArray> m_SPLOMSelection;
	vtkSmartPointer<vtkLookupTable> m_lut;
	QVTKWidget * m_SBQVTKWidget;
	vtkSmartPointer<vtkRenderer> m_sbRen;
	vtkSmartPointer<vtkScalarBarActor> m_sbActor;
	QString m_colorArrayName;
	bool m_updateColumnVisibility;
};
