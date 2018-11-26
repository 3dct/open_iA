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

#include "ui_dlg_DynamicVolumeLines.h"
#include "mdichild.h"
#include "iAScalingWidget.h"
#include "DynamicVolumeLinesHelpers.h"
#include "ui_Multi3DView.h"
#include "qthelper/iAQTtoUIConnector.h"

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND))
class QVTKOpenGLWidget;
class vtkGenericOpenGLRenderWindow;
#else
class QVTKWidget;
class vtkRenderWindow;
#endif

class iAVolumeRenderer;
class iANonLinearAxisTicker;
class iAOrientationWidget;
class iASegmentTree;
class vtkRenderWindow;
class vtkTextActor;
class vtkPoints;
class QVTKWidget;
class vtkRenderer;
class vtkLookupTable;
class vtkScalarBarActor;

typedef iAQTtoUIConnector<QDockWidget, Ui_dlg_DynamicVolumeLines>  DynamicVolumeLinesConnector;
typedef iAQTtoUIConnector<QDockWidget, Ui_Multi3DRendererView> multi3DRendererView;

class dlg_DynamicVolumeLines : public DynamicVolumeLinesConnector
{
	Q_OBJECT

public:
	dlg_DynamicVolumeLines(QWidget* parent, QDir datasetsDir, Qt::WindowFlags f = 0);
	~dlg_DynamicVolumeLines();

	QDir m_datasetsDir;
	QList<QPair <QString, QList<icData> >> m_DatasetIntensityMap;

public slots:
	void mousePress(QMouseEvent*);
	void mouseMove(QMouseEvent*);
	void mouseWheel(QWheelEvent*);
	void setFBPTransparency(int);
	void showFBPGraphs();
	void visualize();
	void updateDynamicVolumeLines();
	void updateFBPView();
	void visualizePath();
	void selectionChangedByUser();
	void legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*);
	void setSelectionForPlots(vtkPoints* selCellPoints);
	void setNoSelectionForPlots();
	void showBkgrdThrLine();
	void syncLinearXAxis(QCPRange);
	void syncNonlinearXAxis(QCPRange);
	void syncYAxis(QCPRange);
	void changePlotVisibility();
	void selectCompLevel();
	void setSubHistBinCntFlag();
	void updateHistColorMap(vtkSmartPointer<vtkLookupTable> LUT);
	void compLevelRangeChanged();

signals:
	void compLevelRangeChanged(QVector<double> compRange);

protected:
	virtual bool eventFilter(QObject * obj, QEvent * event);

private:
	MdiChild *m_mdiChild;
	QCustomPlot *m_nonlinearScaledPlot;
	QCustomPlot *m_linearScaledPlot;
	QCustomPlot *m_debugPlot;
	iAScalingWidget *m_scalingWidget;
	iAOrientationWidget * m_orientationWidget;
	QCPItemText *m_nonlinearDataPointInfo;
	QCPItemText *m_linearDataPointInfo;
	QCPItemStraightLine *m_nonlinearIdxLine;
	QCPItemStraightLine *m_linearIdxLine;
	QList<QCPGraph*> m_selGraphList;
	QVector<double> m_nonlinearMappingVec;
	QVector<double> m_impFunctVec;
	QSharedPointer<QCPGraphDataContainer> m_impFuncPlotData;
	QSharedPointer<QCPGraphDataContainer> m_integralImpFuncPlotData;
	QSharedPointer<iANonLinearAxisTicker> m_nonlinearTicker;
	QList<QCPRange> m_bkgrdThrRangeList;
	vtkSmartPointer<vtkLookupTable> m_compLvlLUT;
	vtkSmartPointer<vtkLookupTable> m_histLUT;
	double m_minEnsembleIntensity, m_maxEnsembleIntensity;
	QList<iASegmentTree*> m_segmTreeList;
	bool m_subHistBinCntChanged;
	bool m_histVisMode;
	QVector<double> m_histBinImpFunctAvgVec;
	QVector<double> m_linearHistBinBoarderVec;
	double m_stepSize;

	QList<vtkSmartPointer<vtkImageData>> m_imgDataList;
	multi3DRendererView *m_MultiRendererView;

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND))
	QVTKOpenGLWidget *wgtContainer;
#else
	QVTKWidget *wgtContainer;
#endif

	vtkSmartPointer<vtkRenderer> m_mrvBGRen;
	vtkSmartPointer<vtkTextActor> m_mrvTxtAct;
	QSharedPointer<iAVolumeRenderer> m_volRen;
		
	void generateHilbertIdx();
	void setupFBPGraphs(QCustomPlot* qcp, iAFunctionalBoxplot<double, double>* fbpData);
	void setupScaledPlot(QCustomPlot* qcp);
	void setupDebugPlot();
	void setupGUIConnections();
	void setupMultiRendererView();
	void showDebugPlot();
	void calcNonLinearMapping();
	void showBkgrdThrRanges(QCustomPlot* qcp);
	void showCompressionLevel();
	void setupGUIElements();
	void setupScalingWidget();
	void setupPlotConnections(QCustomPlot* qcp);
	void setSelectionForRenderer(QList<QCPGraph *> visSelGraphList);
	void generateSegmentTree();
	void checkHistVisMode(int lowerIdx, int upperIdx);
};
