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

#include <QMainWindow>

#include "ui_dream_caster4.h"
#include "ui_LogWnd.h"
#include "ui_results_dialog.h"
#include "ui_settings.h"

#include "raycast/include/common.h"

class RenderFromPosition;
//VTK
class vtkPolyDataMapper;
class vtkDataSetMapper;
class vtkActor;
class vtkRenderer;
class vtkSTLReader;
struct ParamWidget;
struct ParametersView;
struct CombinedParametersView;
class CutFigList;
class Plot3DVtk;
class dlg_histogram_simple;
class vtkDepthSortPolyData;


//class rotation_t;
//class parameters_t;
class PaintWidget;
class StabilityWidget;
/**	\class DreamCaster.
	\brief Application main window class.

	Contains GUI and some global classes instances.	
*/
class ScreenBuffer;
class Engine;
const QString windowStateStr = "dreamcaster_wnd_state";

class DreamCaster : public QMainWindow
{
	Q_OBJECT

public:
	/**
	* A DreamCaster consturctor.
	* @param parent parent widget pointer. Default is 0.
	*/
	DreamCaster(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	/**
	* A DreamCaster destructor.
	*/
	~DreamCaster();
	/**
	* Loging function. Adds string to log window.
	* @param text string containing logging message.
	* @param text appendToPrev Append message to previous string or add as new string.
	*/
	void log(QString text, bool appendToPrev=false);
	/**
	* Recreates rendering surface and rendering engine.
	* Loads object. Prepares renderer.
	*/
	void initRaycast();
	/**
	* Setting default parameters to histograms plot.
	*/
	void initHistograms();
	/**
	* Update 3D representation of cutting AABBs
	*/
	int UpdateCutAABVtk();
	/**
	* Find triangles that are belong to selected features
	*/
	int findSelectedTriangles();
	/**
	* Handler of child-widgets events
	*/
	bool eventFilter(QObject *obj, QEvent *event);
	/**
	* Set values to the rotation range scroll boxes
	*/
	void setRangeSB(float minX, float maxX, float minZ, float maxZ);
	/**
	* Open .STL model by specified file name. Used to be called from outside.
	*/
	void loadFile(const QString filename);
public:
	SETTINGS stngs;
private:
	unsigned int indices[3];
	int isOneWidgetMaximized;	///< Flage, determining whether one of the widgets is maximized
	bool isStopped;
	QPixmap *renderPxmp;		///< Pixmap for raycasted image, stores image data
	QPixmap *viewsPxmp;			///< Pixmap for colormap image.
	QPainter * formPainter;		///< Painter. Used to draw on pixmaps (outside QWidget redraw event).
	unsigned int * viewsBuffer;
	Ui::DCFormInterface ui;		///< Contains GUI of main window.
	Ui::LogWindow logsUi;		///< Contains GUI of logging window.
	Ui::ResultsDialog resUi;	///< Contains GUI of results dialog.
	Ui::Settings settingsUi;	///< Contains GUI of settings dialog.
	//histogram stuff
	dlg_histogram_simple * hist;
	//Ui::Histograms histUi; 
	///QwtPlotGrid plotGrid;
	///HistogramItem histogram;///< Histogram item.
	//
	//QWidget logs;				///< Qt widget for logging window GUI
	QWidget settings;			///< Qt widget for settings window GUI
	QDialog res;				///< Qt dialog for results dialog GUI
	PaintWidget *RenderFrame;	///< Widget for raycasted image  visualization 
	PaintWidget *ViewsFrame;	///< Widget for colormap visualization 
	ParametersView *comparisonTab;///< Class containing widgets for all of parameters user for comparison
	CombinedParametersView* weightingTab;///< Class containing widgets for all of parameters user for weighting
	StabilityWidget *stabilityView; ///< Widget representing stability of current specimen orientation
	///Plot *plot;					///< 3D plot item. For average parameter value func.
	QString modelFileName;		///< filename of .stl file containing object
	QString setFileName;		///< filename of file containing current set of renderings
	//VTK classes instances for interactive 3D view
	vtkPolyDataMapper *mapper, *originMapper, *planeMapper, *raysMapper, *raysProjMapper, *plateMapper, *cutAABMapper;
	vtkActor *actor, *originActor, *planeActor, *raysActor, *raysProjActor, *plateActor, *cutAABActor;
	vtkDepthSortPolyData *depthSort;
	vtkRenderer* ren;
	vtkSTLReader* stlReader;
	//
	CutFigList * cutFigList;
	float set_pos[3];
	rotation_t ***rotations;
	parameters_t *** rotationsParams;	///< Data about av. parameter of every rendering
	parameters_t ** placementsParams;	///< Data about av. parameter of every object's placement(sum of all elements in column)
	double ** weightedParams;			///< Data about weighted parameter of every object's placement(sum of all elements in column)
	int cntX;					///< number of renderings by X axis
	int cntY;					///< number of renderings by Y axis
	int cntZ;					///< number of renderings by Z axis
	double paramMin;			///< minimum value of current parameter
	double paramMax;			///< maximum value of current parameter
	double stabilitySensitivity;///< sensitivity of stability widget
	unsigned int curIndX;		///< x index of current placement
	unsigned int curIndY;		///< y index of current placement
	unsigned int curIndZ;		///< z index of current placement
	bool modelOpened;			///< is model opened indicator
	bool datasetOpened;			///< is dataset opened indicator
	int curParamInd;			///< index of current parameter
	std::vector<int> trisInsideAoI;///< indices of triangles that belong to selected features(ionside AABBs)
	//
	ScreenBuffer * scrBuffer;
	float * cuda_avpl_buff;///<float buffer used by cuda to store the results
	float * cuda_dipang_buff;///<float buffer used by cuda to store the results
	Engine* tracer;
	/* The device ptr of the nodes and triangles */
	bool CutFigParametersChangedOFF;
	ModelData mdata;
	Plot3DVtk * plot3d;
	Plot3DVtk * plot3dWeighting;
	void  Pick(int pickPos[2]);
private:
	/**
	*  Update all the views and labels corresponding to the picked placement
	*/
	void setPickedPlacement(int indX, int indY, int indZ);
	/**
	*  Change visibility of widgets that can maximize
	*/
	void changeVisibility(int isVisible);
	/**
	*  Bind model data to cuda textures
	*/
	void SetupGPUBuffers();
	/**
	* Fill dest buffer with colorcoded param values.
	*/
	void fillParamBuffer(unsigned int* dest, int paramInd);
	/**
	* Reads single render data from current renderings file by indices.
	* @param x x-index of render
	* @param y y-index of render
	* @param z z-index of render
	* @param[out] rend rendering pointer for data storing.
	*/
	void readRenderFromBinaryFile(unsigned int x, unsigned int y, unsigned int z, RenderFromPosition * rend);
	/**
	* Read object from stl file.
	*/
	void loadModel();
	/**
	* Setup VTK params and fill vtk-objs with data
	*/
	void setup3DView();
	void RenderFrameMouseReleaedEvent(QMouseEvent * event);
	/**
	* Read settings from config file. Setup settings window
	*/
	int SetupSettingsFromConfigFile();
	/**
	* Update staibility widget values
	*/
	int UpdateStabilityWidget();
	/**
	* Free memory allocated previously for plotData,rotations,plotColumnData, weightedParams
	*/
	void ClearPrevData();
	/**
	* Allocate memory for plotData, rotations, plotColumnData, weightedParams
	*/
	void AllocateData();
	/**
	* Update view when another rendering selected
	*/
	void UpdateView();
	/**
	* Update labels, containing informaiton about picked rotation/rendering
	*/
	void UpdateInfoLabels();
	/**
	* Resize views pixmap
	*/
	void ViewsReset();
	/**
	* Calculate percentage of bad surface, using Radon space analysis
	*/
	double RandonSpaceAnalysis();
	/**
	* Init raytracer engine and clear screen buffer
	*/
	void InitRender(iAVec3 * vp_corners, iAVec3 * vp_delta, iAVec3 * o);
	/**
	* Position the specimen according to the
	*/
	void PositionSpecimen();

	/**
	* Render single frame
	*/
	void Render(const iAVec3 * vp_corners, const iAVec3 * vp_delta, const iAVec3 * o, bool saveData);
protected:
	virtual void closeEvent ( QCloseEvent * event );
public slots:
	/**
	* Slot that updates histogram with parameters specified by user.
	*/
	virtual void UpdateHistogramSlot();
	/**
	* Slot that renders single view with specified parameters.
	*/
	virtual void RenderSingleViewSlot();
	/**
	* Slot that renders set of renderings with specified parameters.
	*/
	virtual void RenderViewsSlot();
	/**
	* Slot that stopo rendering set of renderings with specified parameters.
	*/
	virtual void StopRenderingSlot();
	/**
	* Slot that updates pixmap based widgets.
	*/
	virtual void UpdateSlot();
	/**
	* Empty. Not used.
	*/
	virtual void SaveSlot();
	/**
	* Slot that reads object from stl-file and prepares renderer considering new data.
	*/
	virtual void OpenModelSlot();
	/**
	* Slot that specify new set of renderings filename.
	*/
	virtual void NewSetSlot();
	/**
	* Slot opening new set of renderings file. Also updates 3d plot and colormap.
	*/
	virtual void OpenSetSlot();
	/**
	* Slot that shows rays/triangles with certain parameter range. Rendering indices and parameter range 
	are specified by user.
	*/
	virtual void ShowRangeRays();//TODO: rename(not only rays)
	/**
	* Slot that hiding rays/triangles picked by user before.
	* @see ShowRangeRays()
	*/
	virtual void HideRays();
	/**
	* Set single rendering's position sliders to same values as set's of renderings position sliders.
	*/
	virtual void pbSetPositionSlot();
	/**
	* Set single rendering's position sliders to values corresponding current 
	* object orientation on interactive 3D view.
	*/
	virtual void pbGrab3DSlot();
	/**
	* Updates 3d plot and colormap data with consideration of parameters specified by user.
	*/
	virtual void UpdatePlotSlot();
	/**
	* Save current KD tree in file named as .stl file with extension .kdtree
	*/
	virtual void SaveTree();
	/**
	* When mouse released on ViewsFrame
	*/
	virtual void RenderFrameMouseReleasedSlot();
	/**
	* Show optimal positon
	*/
	virtual void ShowResultsSlot();
	/**
	* Show logs window
	*/
	virtual void ShowLogsSlot();
	/**
	* Save results to file "results.txt" in workind dir
	*/
	virtual void SaveResultsSlot();
	/**
	* Slot that shows colors object, corresponding to angles btw tri's norm and ray to middle of tri
	*/
	virtual void ShowDipAnglesSlot();
	/**
	* Hide coloring of object corresponding to dip angles of triangles
	*/
	virtual void HideDipAnglesSlot();
	/**
	* Read settings from config file and open settings dialog with that data
	*/
	virtual void ConfigureSettingsSlot();
	/**
	* Save settings to config file 
	*/
	virtual void SaveSettingsSlot();
	/**
	* Read settings from config file
	*/
	virtual void ResetSettingsSlot();
	/**
	* Update stability sensitivity
	*/
	virtual void SensitivityChangedSlot();
	/**
	* Update stability resoultion
	*/
	virtual void StabilityResolutionChangedSlot();
	/**
	* Update stability on mouse move mode
	*/
	virtual void UpdateStabilityOnMouseMoveCheckedSlot();
	/**
	* When mouse moved on ViewsFrame
	*/
	virtual void ViewsMouseMoveSlot();
	/**
	* When new current parameter picked from combobox
	*/
	virtual void CurrentParameterChangedSlot();
	/**
	* When current projection selected using slider
	*/
	virtual void ProjectionChangedSlot();
	/**
	* Top placements to be shown on heightmap is changed
	*/
	virtual void TopPlacementsChangedSlot();
	/**
	* When some placement picked on one of the comparison tab's height maps
	*/
	virtual void ComparisonTabPlacementPickedSlot(int, int);
	/**
	* Low cut of height map of param1 on weighting tab is changed
	*/
	virtual void LowCutParam1Slot();
	/**
	* Low cut of height map of param2 on weighting tab is changed
	*/
	virtual void LowCutParam2Slot();
	/**
	* Low cut of height map of param3 on weighting tab is changed
	*/
	virtual void LowCutParam3Slot();
	/**
	* Update results of weighted combination of parameters on weighting tab
	*/
	virtual void UpdateWeightingResultsSlot();
	/**
	* When some placement picked on one of the weighted results on weighting tab
	*/
	virtual void WeightingResultsPlacementPickedSlot(int, int);
	/**
	* Low cut of height map of weighted results on weighting tab is changed
	*/
	virtual void LowCutWeightingResSlot();
	/**
	* New cut figure is added to the list
	*/
	virtual void AddCutFigSlot();
	/**
	* Cut figure removed from the list
	*/
	virtual void RemoveCutFigSlot();
	/**
	* Cut figure picked in the list
	*/
	virtual void CutFigPicked();
	/**
	* Current cut figure's parameters are changed
	*/
	virtual void CutFigParametersChanged();
	/**
	* Color show triangles with bad dip angle
	*/
	virtual void ColorBadAngles();
	/**
	* Hide coloring of bad triangles
	*/
	virtual void HideColoring();
	/**
	* Maximize one area/show all areas
	*/
	virtual void maximize3DView();
	/**
	* Maximize one area/show all areas
	*/
	virtual void maximizeStability();
	/**
	* Maximize one area/show all areas
	*/
	virtual void maximizeRC();
	/**
	* Maximize one area/show all areas
	*/
	virtual void maximizePlacements();
	/**
	* Maximize one area/show all areas
	*/
	virtual void maximizeBottom();
};
