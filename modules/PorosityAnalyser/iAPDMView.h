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

#include "ui_PDMView.h"
#include "iAQTtoUIConnector.h"

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QList>
#include <QMap>


struct iABPMData;
struct iAHMData;

class QCustomPlot;
class QModelIndex;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class QVTKOpenGLWidget;
#else
class QVTKWidget;
#endif
class vtkChartBox;
class vtkContextView;
class vtkLookupTable;
class vtkRenderer;
class vtkScalarBarActor;

struct iABoxPlotData;
struct iAHistogramPlotData;
class iAPDMSettings;

typedef iAQTtoUIConnector<QDockWidget, Ui_PDMView>  PorosityAnalyzerPDMConnector;

class iAPDMView : public PorosityAnalyzerPDMConnector
{
	Q_OBJECT

public:
	iAPDMView( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iAPDMView();
	QModelIndexList SelectedIndices() const { return m_selectedIndices; }

public slots:
	void SetData( const iABPMData * bpmData, const iAHMData * hmData );
	void setSelection( QModelIndexList selInds );

protected:
	void UpdateTableBoxPlot();
	void UpdateTableHistogram();
	void UpdateTableDeviation();
	virtual bool eventFilter( QObject * obj, QEvent * event );
	void addWidgetToTable( int r, int c, QWidget * plot );
	void ShowDeviationControls( bool visible );
	void ShowPorosityRangeControls( bool visible );
	void HighlightSelected( QObject * obj );

protected slots:
	void UpdateTable();
	void FitTable();
	void UpdateRepresentation();
	void UpdateColormapSettings( double range );

signals:
	void selectionModified(QModelIndexList selInds);

protected:
	const QStringList * m_filters;
	const QStringList * m_datasets;
	const QList< QList< iABoxPlotData > > * m_boxPlots;
	const QList< QList< iAHistogramPlotData > > * m_histogramPlots;
	const QMap<QString, double> * m_gtPorosityMap;
	QMap<QObject*, QModelIndex> m_indices;
	QModelIndexList m_selectedIndices;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QVTKOpenGLWidget * m_sbWiget;
#else
	QVTKWidget * m_sbWiget;
#endif
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkRenderer> m_sbRen;
	vtkSmartPointer<vtkScalarBarActor> m_sbActor;
};
