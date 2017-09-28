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

class vtkRenderWindow;
class vtkTextActor;

typedef iAQTtoUIConnector<QDockWidget, Ui_dlg_DatasetComparator>  DatasetComparatorConnector;
typedef iAQTtoUIConnector<QDockWidget, Ui_Multi3DRendererView> multi3DRendererView;

class dlg_DatasetComparator : public DatasetComparatorConnector
{
	Q_OBJECT

public:
	dlg_DatasetComparator(QWidget *parent, QDir datasetsDir, Qt::WindowFlags f = 0);
	~dlg_DatasetComparator();

	QDir m_datasetsDir;
	QList<QPair <QString, QList<icData> >> m_DatasetIntensityMap;

public slots:
	void mousePress(QMouseEvent*);
	void mouseMove(QMouseEvent*);
	void setFbpTransparency(int);
	void showFBPGraphs();
	void showLinePlots();
	void updateDatasetComparator();
	void updateFBPView();
	void visualizePath();
	void selectionChangedByUser();
	void legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*);

private:
	MdiChild *m_mdiChild;
	QCustomPlot *m_customPlot;
	QCPItemText *m_dataPointInfo;
	QList<vtkSmartPointer<vtkImageData>> m_imgDataList;
	multi3DRendererView *m_MultiRendererView;
	vtkSmartPointer<vtkRenderWindow> m_mrvRenWin;
	vtkSmartPointer<vtkRenderer> m_mrvBGRen;
	vtkSmartPointer<vtkTextActor> m_mrvTxtAct;
	QSharedPointer<iAVolumeRenderer> m_volRen;
	QList<QCPPlottableLegendItem*> m_selLegendItemList;
	
	void generateHilbertIdx();
	void createFBPGraphs(iAFunctionalBoxplot<unsigned int, double> *fbpData);
	void setupQCustomPlot();
	void setupGUIConnections();
	void setupMultiRendererView();
};
