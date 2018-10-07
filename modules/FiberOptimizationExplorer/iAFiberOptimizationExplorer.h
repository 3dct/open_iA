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
class iAStackedBarChart;

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

class QActionGroup;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QListView;
class QModelIndex;
class QSlider;
class QSpinBox;
class QStandardItemModel;
class QTimer;
class QTreeView;
class QVBoxLayout;
//class QWebEngineView;

class iAFiberOptimizationExplorer : public QMainWindow, public iASelectionProvider
{
	Q_OBJECT
public:
	typedef std::vector<std::vector<size_t> > SelectionType;
	iAFiberOptimizationExplorer(MainWindow* mainWnd);
	void start(QString const & path, QString const & configName);
	~iAFiberOptimizationExplorer();
	std::vector<std::vector<size_t> > & selection() override;
private slots:
	void toggleVis(int);
	void toggleBoundingBox(int);
	void referenceToggled(bool);
	void miniMouseEvent(QMouseEvent* ev);
	void optimStepSliderChanged(int);
	void mainOpacityChanged(int);
	void contextOpacityChanged(int);
	void selection3DChanged();
	void selectionSPLOMChanged(std::vector<size_t> const & selection);
	void selectionOptimStepChartChanged(std::vector<size_t> const & selection);
	void splomLookupTableChanged();
	void changeReferenceDisplay();
	void playPauseOptimSteps();
	void playTimer();
	void playDelayChanged(int);
	void refDistAvailable();
	void optimDataToggled(int);
	void resultsLoaded();
	void resultsLoadFailed(QString const & path);
	void visualizeCylinderSamplePoints();
	void hideSamplePoints();
	void showReferenceToggled();
	void showReferenceCountChanged(int);
	void showReferenceMeasureChanged(int);
	void selectionFromListActivated(QModelIndex const &);
	void selectionDetailsItemClicked(QModelIndex const &);
	void showSpatialOverviewButton();
	// result view:
	void stackedColSelect();
	void switchStackMode(bool mode);
private:
	QColor getResultColor(int resultID);
	void getResultFiberIDFromSplomID(size_t splomID, size_t & resultID, size_t & fiberID);
	void clearSelection();
	void newSelection(QString const & source);
	size_t selectionSize() const;
	void sortCurrentSelection(QString const & source);
	void showCurrentSelectionInPlots();
	void showCurrentSelectionInPlot(int chartID);
	void showCurrentSelectionIn3DViews();
	void showCurrentSelectionInSPLOM();
	bool isAnythingSelected() const;
	void loadStateAndShow();
	void addInteraction(QString const & interaction);
	void toggleOptimStepChart(int index, bool visible);
	QString diffName(int chartID) const;
	QString resultName(size_t resultID) const;
	QString stackedBarColName(int index) const;
	void setOptimStep(int optimStep);
	void showCurrentSelectionDetail();
	void hideSamplePointsPrivate();
	void showSpatialOverview();

	//! all data about the fiber characteristics optimization results that are analyzed
	QSharedPointer<iAFiberResultsCollection> m_data;
	std::vector<iAFiberCharUIData> m_resultUIs;

	QSharedPointer<iARendererManager> m_renderManager;
	vtkSmartPointer<iASelectionInteractorStyle> m_style;
	iAColorTheme const * m_colorTheme;
	MainWindow* m_mainWnd;
	size_t m_referenceID;
	SelectionType m_selection;
	vtkSmartPointer<vtkTable> m_refVisTable;

	QSharedPointer<iA3DCylinderObjectVis> m_nearestReferenceVis;

	vtkSmartPointer<vtkActor> m_sampleActor;
	QString m_configName;
	QTimer * m_playTimer;
	iARefDistCompute* m_refDistCompute;

	// Elements of the different views:

	// Main Renderer:
	iAVtkWidgetClass* m_mainRenderer;
	QLabel * m_defaultOpacityLabel, *m_contextOpacityLabel;
	QSlider* m_defaultOpacitySlider, *m_contextOpacitySlider;
	QCheckBox* m_chkboxShowReference;
	QSpinBox* m_spnboxReferenceCount;
	QComboBox* m_cmbboxDistanceMeasure;

	// Results List:
	void addStackedBar(int index);
	void removeStackedBar(int index);
	QButtonGroup* m_defaultButtonGroup;
	iAStackedBarChart* m_stackedBarsHeaders;

	// SPLOM:
	iAQSplom* m_splom;

	// Optimization Steps:
	QLabel* m_currentOptimStepLabel;
	std::vector<iAChartWidget*> m_optimStepChart;
	QSlider* m_optimStepSlider;
	QVBoxLayout* m_optimChartLayout;
	std::vector<QCheckBox*> m_chartCB;
	size_t ChartCount;

	// Jobs:
	QDockWidget* m_jobDockWidget;
	iAJobListView * m_jobs;

	// Interaction Protocol:
	QTreeView* m_interactionProtocol;
	QStandardItemModel* m_interactionProtocolModel;

	// Selections:
	QListView* m_selectionList;
	QTreeView* m_selectionDetailsTree;
	QStandardItemModel* m_selectionListModel;
	QStandardItemModel* m_selectionDetailModel;
	std::vector<SelectionType> m_selections;

//	QWebEngineView*  m_browser;
//	QString m_html;
};
