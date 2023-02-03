// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_PDMView.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QList>
#include <QMap>
#include <QMultiMap>


class QCustomPlot;
class QModelIndex;

class vtkChartBox;
class vtkContextView;
class vtkLookupTable;
class vtkRenderer;
class vtkScalarBarActor;

struct iABPMData;
struct iAHistogramPlotData;
struct iAHMData;

struct iABoxPlotData;
class iAQVTKWidget;

typedef iAQTtoUIConnector<QDockWidget, Ui_PDMView>  PorosityAnalyzerPDMConnector;

class iAPDMView : public PorosityAnalyzerPDMConnector
{
	Q_OBJECT

public:
	iAPDMView(QWidget* parent = nullptr);
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
	const QMultiMap<QString, double> * m_gtPorosityMap;
	QMap<QObject*, QModelIndex> m_indices;
	QModelIndexList m_selectedIndices;
	vtkSmartPointer<vtkLookupTable> m_lut;
	iAQVTKWidget * m_sbWidget;
	vtkSmartPointer<vtkRenderer> m_sbRen;
	vtkSmartPointer<vtkScalarBarActor> m_sbActor;
};
