/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <QMainWindow>

#include "ui_dream_caster4.h"
#include "ui_LogWnd.h"
#include "ui_results_dialog.h"
#include "ui_settings.h"

#include "raycast/include/iADreamCasterCommon.h"

#include "iAVtkWidgetFwd.h"

class vtkActor;
class vtkDataSetMapper;
class vtkDepthSortPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkSTLReader;

struct iACombinedParametersView;
class iACutFigList;
class dlg_histogram_simple;
class iAEngine;
class iAPaintWidget;
struct iAParamWidget;
struct iAParametersView;
class iAPlot3DVtk;
class iARenderFromPosition;
class iAScreenBuffer;
class iAStabilityWidget;

//! DreamCaster - Application main window class. Contains GUI and some global classes instances.
class iADreamCaster : public QMainWindow
{
	Q_OBJECT
public:
	//! A DreamCaster consturctor.
	//! @param parent parent widget pointer. Default is 0.
	//! @param flags window flags
	iADreamCaster(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
	~iADreamCaster();
	//! Loging function. Adds string to log window.
	//! @param text string containing logging message.
	//! @param appendToPrev Append message to previous string or add as new string.
	void log(QString text, bool appendToPrev=false);
	//! Recreates rendering surface and rendering engine. Loads object. Prepares renderer.
	void initRaycast();
	//! Setting default parameters to histograms plot.
	void initHistograms();
	//! Update 3D representation of cutting AABBs
	int updateCutAABVtk();
	//! Find triangles that are belong to selected features
	int findSelectedTriangles();
	//! Handler of child-widgets events
	bool eventFilter(QObject *obj, QEvent *event) override;
	//! Set values to the rotation range scroll boxes
	void setRangeSB(float minX, float maxX, float minZ, float maxZ);
	//! Open .STL model by specified file name. Used to be called from outside.
	void loadFile(const QString filename);
public:
	iADreamCasterSettings stngs;
private:
	unsigned int indices[3];
	int isOneWidgetMaximized;           //!< Flage, determining whether one of the widgets is maximized
	bool isStopped;
	QPixmap *renderPxmp;                //!< Pixmap for raycasted image, stores image data
	QPixmap *viewsPxmp;                 //!< Pixmap for colormap image.
	QPainter * formPainter;             //!< Painter. Used to draw on pixmaps (outside QWidget redraw event).
	unsigned int * viewsBuffer;
	Ui::DCFormInterface ui;             //!< Contains GUI of main window.
	Ui::LogWindow logsUi;               //!< Contains GUI of logging window.
	Ui::ResultsDialog resUi;            //!< Contains GUI of results dialog.
	Ui::Settings settingsUi;            //!< Contains GUI of settings dialog.
	//histogram stuff
	dlg_histogram_simple * hist;
	QWidget settings;                   //!< Qt widget for settings window GUI
	QDialog res;                        //!< Qt dialog for results dialog GUI
	iAPaintWidget *RenderFrame;           //!< Widget for raycasted image  visualization
	iAPaintWidget *ViewsFrame;            //!< Widget for colormap visualization
	iAParametersView *comparisonTab;      //!< Class containing widgets for all of parameters user for comparison
	iACombinedParametersView* weightingTab;//!< Class containing widgets for all of parameters user for weighting
	iAStabilityWidget *stabilityView;     //!< Widget representing stability of current specimen orientation
	QString modelFileName;              //!< filename of .stl file containing object
	QString setFileName;                //!< filename of file containing current set of renderings

	// VTK classes instances for interactive 3D view
	vtkPolyDataMapper *mapper, *originMapper, *planeMapper, *raysMapper, *raysProjMapper, *plateMapper, *cutAABMapper;
	vtkActor *actor, *originActor, *planeActor, *raysActor, *raysProjActor, *plateActor, *cutAABActor;
	vtkDepthSortPolyData *depthSort;
	vtkRenderer* ren;
	vtkSTLReader* stlReader;
	iACutFigList * cutFigList;
	float set_pos[3];
	iArotation_t ***rotations;
	iAparameters_t *** rotationsParams;   //!< Data about av. parameter of every rendering
	iAparameters_t ** placementsParams;   //!< Data about av. parameter of every object's placement(sum of all elements in column)
	double ** weightedParams;           //!< Data about weighted parameter of every object's placement(sum of all elements in column)
	int cntX;                           //!< number of renderings by X axis
	int cntY;                           //!< number of renderings by Y axis
	int cntZ;                           //!< number of renderings by Z axis
	double paramMin;                    //!< minimum value of current parameter
	double paramMax;                    //!< maximum value of current parameter
	double stabilitySensitivity;        //!< sensitivity of stability widget
	unsigned int curIndX;               //!< x index of current placement
	unsigned int curIndY;               //!< y index of current placement
	unsigned int curIndZ;               //!< z index of current placement
	bool modelOpened;                   //!< is model opened indicator
	bool datasetOpened;                 //!< is dataset opened indicator
	int curParamInd;                    //!< index of current parameter
	std::vector<int> trisInsideAoI;     //!< indices of triangles that belong to selected features(ionside AABBs)
	iAScreenBuffer * scrBuffer;
	float * cuda_avpl_buff;             //!< float buffer used by cuda to store the results
	float * cuda_dipang_buff;           //!< float buffer used by cuda to store the results
	iAEngine* tracer;                     //!< The device ptr of the nodes and triangles
	bool CutFigParametersChangedOFF;
	iAModelData mdata;
	iAPlot3DVtk * plot3d;
	iAPlot3DVtk * plot3dWeighting;

	iAVtkOldWidget *qvtkWidget, *qvtkPlot3d, *qvtkWeighing;
	void  Pick(int pickPos[2]);
	//! Update all the views and labels corresponding to the picked placement
	void setPickedPlacement(int indX, int indY, int indZ);
	//! Change visibility of widgets that can maximize
	void changeVisibility(int isVisible);
	//! Bind model data to cuda textures
	void SetupGPUBuffers();
	//! Fill dest buffer with colorcoded param values.
	void fillParamBuffer(unsigned int* dest, int paramInd);
	//! Reads single render data from current renderings file by indices.
	//! @param x x-index of render
	//! @param y y-index of render
	//! @param z z-index of render
	//! @param[out] rend rendering pointer for data storing.
	void readRenderFromBinaryFile(unsigned int x, unsigned int y, unsigned int z, iARenderFromPosition * rend);
	//! Read object from stl file.
	void loadModel();
	//! Setup VTK params and fill vtk-objs with data
	void setup3DView();
	//! Read settings from config file. Setup settings window
	int SetupSettingsFromConfigFile();
	//! Update staibility widget values
	int UpdateStabilityWidget();
	//! Free memory allocated previously for plotData,rotations,plotColumnData, weightedParams
	void ClearPrevData();
	//! Allocate memory for plotData, rotations, plotColumnData, weightedParams
	void AllocateData();
	//! Update view when another rendering selected
	void UpdateView();
	//! Update labels, containing informaiton about picked rotation/rendering
	void UpdateInfoLabels();
	//! Resize views pixmap
	void ViewsReset();
	//! Calculate percentage of bad surface, using Radon space analysis
	double RandonSpaceAnalysis();
	//! Init raytracer engine and clear screen buffer
	void InitRender(iAVec3f * vp_corners, iAVec3f * vp_delta, iAVec3f * o);
	//! Position the specimen according to the
	void PositionSpecimen();
	//! Render single frame
	void Render(const iAVec3f * vp_corners, const iAVec3f * vp_delta, const iAVec3f * o, bool saveData);
	//! opens the result set file with the given fileName
	void OpenSetFile(QString const & fileName);
protected:
	void closeEvent ( QCloseEvent * event ) override;
public slots:
	//! Slot that updates histogram with parameters specified by user.
	void UpdateHistogramSlot();
	//! Slot that renders single view with specified parameters.
	void RenderSingleViewSlot();
	//! Slot that renders set of renderings with specified parameters.
	void RenderViewsSlot();
	//! Slot that stopo rendering set of renderings with specified parameters.
	void StopRenderingSlot();
	//! Slot that updates pixmap based widgets.
	void UpdateSlot();
	//! Empty. Not used.
	void SaveSlot();
	//! Slot that reads object from stl-file and prepares renderer considering new data.
	void OpenModelSlot();
	//! Slot that specify new set of renderings filename.
	void NewSetSlot();
	//! Slot opening new set of renderings file. Also updates 3d plot and colormap.
	void OpenSetSlot();
	//! Slot that shows rays/triangles with certain parameter range. Rendering indices and parameter range are specified by user.
	void ShowRangeRays();//TODO: rename(not only rays)
	//! Slot that hiding rays/triangles picked by user before.
	//! @see ShowRangeRays()
	void HideRays();
	//! Set single rendering's position sliders to same values as set's of renderings position sliders.
	void pbSetPositionSlot();
	//! Set single rendering's position sliders to values corresponding current object orientation on interactive 3D view.
	void pbGrab3DSlot();
	//! Updates 3d plot and colormap data with consideration of parameters specified by user.
	void UpdatePlotSlot();
	//! Save current KD tree in file named as .stl file with extension .kdtree
	void SaveTree();
	//! When mouse released on ViewsFrame
	void RenderFrameMouseReleasedSlot();
	//! Show optimal positon
	void ShowResultsSlot();
	//! Show logs window
	void ShowLogsSlot();
	//! Save results to .result file in same directory as data stl file
	void SaveResultsSlot();
	//! Slot that shows colors object, corresponding to angles btw tri's norm and ray to middle of tri
	void ShowDipAnglesSlot();
	//! Hide coloring of object corresponding to dip angles of triangles
	void HideDipAnglesSlot();
	//! Read settings from config file and open settings dialog with that data
	void ConfigureSettingsSlot();
	//! Save settings to config file
	void SaveSettingsSlot();
	//! Read settings from config file
	void ResetSettingsSlot();
	//! Update stability sensitivity
	void SensitivityChangedSlot();
	//! Update stability resoultion
	void StabilityResolutionChangedSlot();
	//! Update stability on mouse move mode
	void UpdateStabilityOnMouseMoveCheckedSlot();
	//! When mouse moved on ViewsFrame
	void ViewsMouseMoveSlot();
	//! When new current parameter picked from combobox
	void CurrentParameterChangedSlot();
	//! When current projection selected using slider
	void ProjectionChangedSlot();
	//! Top placements to be shown on heightmap is changed
	void TopPlacementsChangedSlot();
	//! When some placement picked on one of the comparison tab's height maps
	void ComparisonTabPlacementPickedSlot(int, int);
	//! Low cut of height map of param1 on weighting tab is changed
	void LowCutParam1Slot();
	//! Low cut of height map of param2 on weighting tab is changed
	void LowCutParam2Slot();
	//! Low cut of height map of param3 on weighting tab is changed
	void LowCutParam3Slot();
	//! Update results of weighted combination of parameters on weighting tab
	void UpdateWeightingResultsSlot();
	//! When some placement picked on one of the weighted results on weighting tab
	void WeightingResultsPlacementPickedSlot(int, int);
	//! Low cut of height map of weighted results on weighting tab is changed
	void LowCutWeightingResSlot();
	//! New cut figure is added to the list
	void AddCutFigSlot();
	//! Cut figure removed from the list
	void RemoveCutFigSlot();
	//! Cut figure picked in the list
	void CutFigPicked();
	//! Current cut figure's parameters are changed
	void CutFigParametersChanged();
	//! Color show triangles with bad dip angle
	void ColorBadAngles();
	//! Hide coloring of bad triangles
	void HideColoring();
	//! Maximize one area/show all areas
	void maximize3DView();
	//! Maximize one area/show all areas
	void maximizeStability();
	//! Maximize one area/show all areas
	void maximizeRC();
	//! Maximize one area/show all areas
	void maximizePlacements();
	//! Maximize one area/show all areas
	void maximizeBottom();
};
