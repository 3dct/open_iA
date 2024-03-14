// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompVisOptions.h"
#include "iACompHistogramTableData.h"
#include "ui_CompHistogramTable.h"

//vtk
#include <vtkSmartPointer.h>

//Qt
#include <QDockWidget>

//CompVis
class iAMainWindow;
class iACompUniformBinningData;
class iACompUniformTable;
class iACompVariableTable;
class iACompHistogramTable;
class iACompCurve;
class iACompCombiTable;
class iACsvDataStorage;

class iAQVTKWidget;

//vtk
class vtkInteractorObserver;
class vtkRenderer;
class vtkCamera;


class iACompHistogramVis : public QDockWidget, public Ui_CompHistogramTable
{

public:
	iACompHistogramVis(iACompHistogramTable* table, iAMainWindow* parent, int amountDatasets, bool MDSComputedFlag);

	void showEvent(QShowEvent* event) override;
	void reinitialize();

	/*** Rendering ****/
	//calculate the height and width each row can have to fit into the screen
	void calculateRowWidthAndHeight(double width, double heigth, double numberOfDatasets);

	//add renderer to widget
	void addRendererToWidget(vtkSmartPointer<vtkRenderer> renderer);
	void removeAllRendererFromWidget();
	//re-render the widget/visualization
	void renderWidget();
	//assign the actual interactionstyle to the widget
	void setInteractorStyleToWidget(vtkSmartPointer<vtkInteractorObserver> style);

	//render the uniform histogram table
	void showUniformTable();
	//render the variable histogram table
	void showVariableTable();
	//render the transition between the histogram table and the curve
	void showCombiTable();
	//render the curve representation
	void showCurve();
	//render the curve representation only with white lines
	void showWhiteCurve();

	/*** Getter & Setter ****/
	int getAmountDatasets();
	double getWindowHeight();
	double getWindowWidth();
	double getRowSize();
	double getColSize();
	iACompVisOptions::binningType getActiveBinning();
	iACompVisOptions::activeVisualization getActiveVisualization();

	iACsvDataStorage* getDataStorage();

	std::vector<int>*  getOrderOfIndicesDatasets();
	void setOrderOfIndicesDatasets(std::vector<int>* newOrderOfIndicesDatasets);

	std::vector<int>* getNewOrderOfIndicesDatasets();
	std::vector<int>* getOriginalOrderOfIndicesDatasets();

	std::map<int, std::vector<double>>* getDrawingPositionForRegions();

	std::vector<int>* getAmountObjectsEveryDataset();

	vtkSmartPointer<vtkCamera> getCamera();
	void setCamera(vtkSmartPointer<vtkCamera> newCamera);

	bool getXAxis();

	/*** Update Table Visualization ****/
	void drawUniformTable();
	void drawBayesianBlocksTable();
	void drawNaturalBreaksTable();
	void drawCurveTable();
	void drawCombiTable();
	void drawWhiteCurveTable();

	/*** Recalculate Data Binning ****/
	iACompHistogramTableData* recalculateBinning(iACompVisOptions::binningType binningType, int numberOfBins);
	iACompHistogramTableData* calculateSpecificBins(
		iACompVisOptions::binningType binningType, bin::BinType* data, int currBin, int amountOfBins);
	void recomputeKernelDensityCurveUB();

	/*** Update OTHERS ****/
	void resetOtherCharts();
	void updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);

	/*** Update THIS ****/
	void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType);
	void removeSelectionOfCorrelationMap();

	/*** Ordering/Ranking ****/
	//draw the datasets with rows ordered ascending to its amount of objects
	void drawDatasetsInAscendingOrder();
	//draw the datasets with rows ordered descending to its amount of objects
	void drawDatasetsInDescendingOrder();
	//draw the datasets with rows ordered according to loading the datasets
	void drawDatasetsInOriginalOrder();
	//deactivates the ordering button in the menu
	void deactivateOrderingButton();
	//activates the ordering button in the menu
	void activateOrderingButton();

	//sorts the input vector according to the given orderStyle ascending(0) or descending(1)
	std::vector<int>* sortWithMemory(std::vector<int> input, int orderStyle);
	std::vector<int>* sortWithMemory(std::vector<double> input, int orderStyle);
	std::vector<int>* reorderAccordingTo(std::vector<int>* newPositions);

private:

	//show is called the first time after starting the application
	void showInitally();
	//show is called after minimizing etc.
	void showAFresh();

	//create the histogramTable visualization
	void initializeVisualization();

	//initialize in which drawing area the a row with specified y coordinates belongs to
	//stores for each y-area = [minY,maxY] to which drawing position it belongs to
	void determineRowAreas();
	//initialize the order of the datasets from the last to the first
	void initializeOrderOfIndices();

	/*************** Rendering ****************************/
	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkCamera> mainCamera;

	//amount of datasets
	int m_amountDatasets;
	//number of planes
	double m_colSize;
	//number of subdivisions
	double m_rowSize;
	//stores the screen ratio
	double m_screenRatio;

	/*** Coloring/Representation of Curves in Visualizations ***/
	double m_AreaOpacity;  //polygons should have a certain opacity between [0,1]
	double m_lineWidth;  //thickness of curve

	/*************** initialization ****************************/
	//true when first rendering, otherwise false
	bool m_initialRendering;
	//initial window width of drawing area
	double m_windowWidth;
	//initial window height of drawing area
	double m_windowHeight;
	//is true, when univariate distirbutions are visualized, is false when MDS data is visualized
	bool xAxis;

	/*************** arrays storing the different dataset orderings ****************************/
	//stores the order of the indices of each dataset
	//each dataset has an index between 0 and amountOfDatasets-1
	//initially the last dataset is stored first, descendingly
	//the position in the vector determines the drawingPosition --> 0 is bottom, m_amountDatasets-1 is top
	std::vector<int>* m_orderOfIndicesDatasets;

	//stores the ORIGINAL order of the indices of each dataset
	std::vector<int>* m_originalOrderOfIndicesDatasets;

	//stores the order of the indices of each dataset during the manual repositioning of an actor
	std::vector<int>* m_newOrderOfIndicesDatasets;

	//stores for each drawing position which y-area it covers
	std::map<int, std::vector<double>>* m_drawingPositionForRegions;

	/*************** Visualizations ****************************/
	iACompUniformTable* m_uniformTable;
	iACompVariableTable* m_variableTable;
	iACompCurve* m_curveTable;
	iACompCombiTable* m_combiTable;

	iACompHistogramTable* m_main;

	iACompVisOptions::activeVisualization m_activeVis;
	iACompVisOptions::binningType m_activeBinning;
};
