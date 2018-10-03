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

#include "iASelectionInteractorStyle.h" // for iASelectionProvider

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>

#include <vector>

class iAFiberResultsCollection;
class iAFiberCharUIData;
class iAJobListView;

class iA3DCylinderObjectVis;

class iAChartWidget;
class iAColorTheme;
class iAQSplom;
class iARendererManager;
class iARefDistCompute;
class iASPLOMData;
class MainWindow;

#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class QVTKOpenGLWidget;
typedef QVTKOpenGLWidget iAVtkWidgetClass;
#else
class QVTKWidget2;
typedef QVTKWidget2 iAVtkWidgetClass;
#endif

class vtkTable;

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
//class QWebEngineView;
class QSlider;
class QSpinBox;
class QStandardItemModel;
class QTimer;
class QTreeView;

class iAFiberOptimizationExplorer : public QMainWindow, public iASelectionProvider
{
	Q_OBJECT
public:
	iAFiberOptimizationExplorer(MainWindow* mainWnd);
	void start(QString const & path, QString const & configName);
	~iAFiberOptimizationExplorer();
	std::vector<std::vector<size_t> > & selection() override;
private slots:
	void toggleVis(int);
	void toggleBoundingBox(int);
	void referenceToggled(bool);
	void miniMouseEvent(QMouseEvent* ev);
	void timeSliderChanged(int);
	void mainOpacityChanged(int);
	void contextOpacityChanged(int);
	void selection3DChanged();
	void selectionSPLOMChanged(std::vector<size_t> const & selection);
	void selectionTimeStepChartChanged(std::vector<size_t> const & selection);
	void splomLookupTableChanged();
	void changeReferenceDisplay();
	void playPauseTimeSteps();
	void playTimer();
	void playDelayChanged(int);
	void refDistAvailable();
	void timeErrorDataChanged(int);
	void resultsLoaded();
	void resultsLoadFailed(QString const & path);
	void visualizeCylinderSamplePoints();
	void showReferenceToggled();
	void showReferenceCountChanged(int);
	void showReferenceMeasureChanged(int);
private:
	QColor getResultColor(int resultID);
	void getResultFiberIDFromSplomID(size_t splomID, size_t & resultID, size_t & fiberID);
	void clearSelection();
	void sortCurrentSelection();
	void showCurrentSelectionInPlot();
	void showCurrentSelectionIn3DViews();
	void showCurrentSelectionInSPLOM();
	bool isAnythingSelected() const;
	void loadStateAndShow();
	void addInteraction(QString const & interaction);

	//! all data about the fiber characteristics optimization results that are analyzed
	QSharedPointer<iAFiberResultsCollection> m_results;
	std::vector<iAFiberCharUIData> m_resultUIs;

	QSharedPointer<iARendererManager> m_renderManager;
	vtkSmartPointer<iASelectionInteractorStyle> m_style;
	iAColorTheme const * m_colorTheme;
	MainWindow* m_mainWnd;
	int m_referenceID;
	std::vector<std::vector<size_t> > m_selection;
	vtkSmartPointer<vtkTable> m_refVisTable;

	iAVtkWidgetClass* m_mainRenderer;
	QLabel* m_currentTimeStepLabel, * m_defaultOpacityLabel, * m_contextOpacityLabel;
	QSlider* m_defaultOpacitySlider, * m_contextOpacitySlider;
	QButtonGroup* m_defaultButtonGroup;
	iAQSplom* m_splom;
	iAChartWidget* m_timeStepChart;
	QCheckBox* m_chkboxShowReference;
	QSpinBox* m_spnboxReferenceCount;
	QComboBox* m_cmbboxDistanceMeasure;

	QSharedPointer<iA3DCylinderObjectVis> m_nearestReferenceVis;

	vtkSmartPointer<vtkActor> m_sampleActor;
	QString m_configName;
	QTimer * m_playTimer;
	QSlider* m_timeStepSlider;
	iARefDistCompute* m_refDistCompute;

//	QWebEngineView*  m_browser;
	QString m_html;
	iAJobListView * m_jobs;
	QDockWidget* m_jobDockWidget;
	QTreeView* m_interactionProtocolView;
	QStandardItemModel* m_interactionProtocolModel;
};
