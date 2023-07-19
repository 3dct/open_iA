// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QMultiMap>
#include <QSharedPointer>
#include <QWidget>

class iAHistogramData;
class iABarGraphPlot;
class iARangeSliderDiagramWidget;

class QComboBox;
class QFrame;
class QHBoxLayout;
class QLabel;
class QTableWidget;
class QVBoxLayout;

class vtkPiecewiseFunction;
class vtkColorTransferFunction;
class vtkIdTypeArray;

class iARangeSliderDiagramView : public QWidget
{
	Q_OBJECT

public:
	iARangeSliderDiagramView(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

public slots:
	void setData( const QTableWidget * data );
	void updateDiagrams();
	void loadSelectionToSPMView();
	void clearOldRSDView();

signals:
	void selectionModified( vtkIdTypeArray * );

private:
	std::shared_ptr<iAHistogramData> m_rangeSliderData;
	std::shared_ptr<iABarGraphPlot> m_rangeSliderDiagramDrawer;
	QList<double> m_data;
	QList<vtkSmartPointer<vtkPiecewiseFunction> > m_oTFList;
	QList<vtkSmartPointer<vtkColorTransferFunction> > m_cTFList;
	QList<iARangeSliderDiagramWidget *> m_widgetList;

	QWidget * m_mainContainer;
	QWidget * m_comboBoxContainer;
	QWidget * m_histoContainer;
	QVBoxLayout * m_layoutVBMainContainer;
	QHBoxLayout * m_layoutHBComboBoxes;
	QVBoxLayout * m_layoutVBHistoContainer;
	QComboBox * m_cbPorDev;
	QComboBox * m_cbStatisticMeasurements;
	QLabel * m_title;
	QLabel * m_input;
	QLabel * m_output;
	QFrame * m_separator;
	const QTableWidget * m_rawTable;
	QMultiMap<double, QList<double>> m_histogramMap;

	QMultiMap<QString, QList<double> > prepareData( const QTableWidget * tableData, bool porOrDev, bool statisticMeasurements );
	void addTitleLabel();
	void addComboBoxes();
	void addOutputLabel();
	void deleteOutdated();
	void setupHistogram();
	void setupDiagrams();
};
