// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_SPMView.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <QDockWidget>

class QAction;
class QCheckBox;
class QTableWidget;
class QVBoxLayout;

class vtkColorTransferFunction;
class vtkContextView;
class vtkIdTypeArray;
class vtkLookupTable;
class vtkRenderer;
class vtkScalarsToColors;
class vtkScalarBarActor;
class vtkTable;

class iAFAQSplom;
struct iASelection;

class iAMainWindow;
class iAQVTKWidget;

typedef iAQTtoUIConnector<QDockWidget, Ui_SPMView> iAPorosityAnalyzerSPMConnector;

class iASPMView : public iAPorosityAnalyzerSPMConnector
{
	Q_OBJECT

public:
	iASPMView(iAMainWindow* mWnd, QWidget* parent = nullptr);
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

	//! Set selection to SPLOM from VTK array of id-s.
	void setSPLOMSelection( vtkIdTypeArray * ids );

	//! Initialize scalar bar widget which shows color coding.
	void initScalarBar();

	//! Active plot indices.
	vtkVector2i getActivePlotIndices();

protected slots:

	//! Apply lookup table to all the plots in the SPM.
	void applyLookupTable();

	//! When selection of the SPLOM is modified.
	void selectionUpdated( std::vector<size_t> const & selInds );

signals:
	void selectionModified( vtkVector2i, vtkIdTypeArray* );
	void previewSliceChanged( int sliceNumber );
	void sliceCountChanged( int sliceCount );
	void maskHovered( const QPixmap * mask, int datasetIndex = -1 );

protected:
	iAFAQSplom * m_splom;
	vtkSmartPointer<vtkIdTypeArray> m_SPLOMSelection;
	vtkSmartPointer<vtkLookupTable> m_lut;
	iAQVTKWidget * m_SBQVTKWidget;
	vtkSmartPointer<vtkRenderer> m_sbRen;
	vtkSmartPointer<vtkScalarBarActor> m_sbActor;
};
