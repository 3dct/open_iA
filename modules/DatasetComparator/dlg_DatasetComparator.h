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

#include "ui_dlg_DatasetComparator.h"
#include "mdichild.h"
#include "datasetComparatorHelpers.h"
#include "ui_Multi3DView.h"
#include "iAQTtoUIConnector.h"

class iAVolumeRenderer;
class iANonLinearAxisTicker;
class iAScalingWidget;

class vtkRenderWindow;
class vtkTextActor;
class vtkPoints;
class QVTKWidget;
class vtkRenderer;
class vtkLookupTable;
class vtkScalarBarActor;


typedef iAQTtoUIConnector<QDockWidget, Ui_dlg_DatasetComparator>  DatasetComparatorConnector;
typedef iAQTtoUIConnector<QDockWidget, Ui_Multi3DRendererView> multi3DRendererView;

class dlg_DatasetComparator : public DatasetComparatorConnector
{
	Q_OBJECT

public:
	dlg_DatasetComparator(QWidget* parent, QDir datasetsDir, Qt::WindowFlags f = 0);
	~dlg_DatasetComparator();

	QDir m_datasetsDir;
	QList<QPair <QString, QList<icData> >> m_DatasetIntensityMap;

public slots:
	void mousePress(QMouseEvent*);
	void mouseMove(QMouseEvent*);
	void setFbpTransparency(int);
	void showFBPGraphs();
	void visualize();
	void updateDatasetComparator();
	void updateFBPView();
	void visualizePath();
	void selectionChangedByUser();
	void legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*);
	void setSelectionFromRenderer(vtkPoints* selCellPoints);
	void showBkgrdThrLine();
	void syncLinearXAxis(QCPRange);
	void syncLinearYAxis(QCPRange);
	void syncNonlinearXAxis(QCPRange);
	void syncNonlinearYAxis(QCPRange);
	void changePlotVisibility();

protected:
	virtual bool eventFilter(QObject * obj, QEvent * event);

private:
	MdiChild *m_mdiChild;
	QCustomPlot *m_nonlinearScaledPlot;
	QCustomPlot *m_linearScaledPlot;
	QCustomPlot *m_debugPlot;
	QToolButton *m_nlVisibilityButton;
	QToolButton *m_lVisibilityButton;

	QCPItemText *m_nonlinearDataPointInfo;
	QCPItemText *m_linearDataPointInfo;
	QCPItemStraightLine *m_nonlinearIdxLine;
	QCPItemStraightLine *m_linearIdxLine;

	QList<vtkSmartPointer<vtkImageData>> m_imgDataList;
	multi3DRendererView *m_MultiRendererView;
	vtkSmartPointer<vtkRenderWindow> m_mrvRenWin;
	vtkSmartPointer<vtkRenderer> m_mrvBGRen;
	vtkSmartPointer<vtkTextActor> m_mrvTxtAct;
	QSharedPointer<iAVolumeRenderer> m_volRen;
	QList<QCPPlottableLegendItem*> m_selLegendItemList;

	QVector<double> m_nonlinearMappingVec;
	QVector<double> m_impFunctVec;
	QSharedPointer<QCPGraphDataContainer> m_impFuncPlotData;
	QSharedPointer<QCPGraphDataContainer> m_integralImpFuncPlotData;
	QSharedPointer<iANonLinearAxisTicker> m_nonlinearTicker;
	QList<QCPRange> m_bkgrdThrRangeList;
	iAScalingWidget *m_scalingWidget;
	vtkSmartPointer<vtkLookupTable> m_lut;
	
	void generateHilbertIdx();
	void setupFBPGraphs(iAFunctionalBoxplot<double, double>* fbpData);
	void setupNonlinearScaledPlot();
	void setupLinearScaledPlot();
	void setupDebugPlot();
	void setupGUIConnections();
	void setupMultiRendererView();
	void showDebugPlot();
	void calcNonLinearMapping();
	void showBkgrdThrRanges();
	void showCompressionLevel();
	void setupGUIElements();
	void setupScalingWidget();
};
