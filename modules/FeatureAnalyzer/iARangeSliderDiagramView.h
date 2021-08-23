/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	iARangeSliderDiagramView( QWidget * parent = 0, Qt::WindowFlags f = 0 );
#else
	iARangeSliderDiagramView(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
#endif

public slots:
	void setData( const QTableWidget * data );
	void updateDiagrams();
	void loadSelectionToSPMView();
	void clearOldRSDView();

signals:
	void selectionModified( vtkIdTypeArray * );

private:
	QSharedPointer<iAHistogramData> m_rangeSliderData;
	QSharedPointer<iABarGraphPlot> m_rangeSliderDiagramDrawer;
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
