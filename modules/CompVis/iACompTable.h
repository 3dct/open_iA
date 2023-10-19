// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompVisOptions.h"
#include "iACompHistogramTableData.h"
#include "iACompTableInteractorStyle.h"

//vtk
#include <vtkSmartPointer.h>

//C++
#include <map>
 
//CompVis
class iACompHistogramVis;

//vtk
class vtkColorTransferFunction;
class vtkDoubleArray;
class vtkLookupTable;
class vtkPoints;
class vtkRenderer;
class vtkPlaneSource;
class vtkTextActor;
class vtkActor;
class vtkUnsignedCharArray;


//because of vtk this method has to be placed outside the class
void buildGlyphRepresentation(void* arg);

class iACompTable
{
public:
	iACompTable(iACompHistogramVis* vis);

	vtkSmartPointer<vtkRenderer> getRenderer();
	iACompHistogramVis* getHistogramVis();

	/*** Rendering ***/
	void addRendererToWidget();
	void addLegendRendererToWidget();
	void setInteractorStyleToWidget(vtkSmartPointer<iACompTableInteractorStyle> interactorStyle);
	void renderWidget();
	void clearRenderer();
	
	//get the length/range of the bin, which is depending on the bin storing the highest number of objects
	double getBinRangeLength();
	//set the length/range of the bin, which is depending on the bin storing the highest number of objects
	void setBinRangeLength(double binRangeLength);

	//set the visualization is active (it will be drawn)
	virtual void setActive() = 0;
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive() = 0;

	//returns the rendering view: the mainrenderer of the table has only 0.85 space available, the other space belongs to the legend
	int getRenderingView();

	/*** Ordering/Ranking ***/
	//draw Histogram table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder() = 0;
	//draw Histogram table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder() = 0;
	//draw Histogram table with rows ordered according to loading the datasets
	virtual void drawHistogramTableInOriginalOrder() = 0;

	//get the boolean indicating that the bar chart visulaiztion showing the number of objects for each dataset is active
	bool getBarChartAmountObjectsActive();
	//remove bars from the visualization
	void removeBarCharShowingAmountOfObjects();

	/*** Update THIS****/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType) = 0;
	virtual void removeSelectionOfCorrelationMap() = 0;

	/*** Interaction ***/
	virtual void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId) = 0;
	//remove the selected cells with an outline
	//(necessary that the renderer only contains the datarows for further calculations)
	virtual void removeHighlightedCells();

	std::vector<int>* getIndexOfPickedRows();

protected:

	virtual void initializeTable() = 0;
	virtual void initializeInteraction() = 0;

	//create the color lookuptable
	virtual void makeLUTFromCTF();
	virtual void makeLUTDarker();

	//define the range of the color map bins for the visualization
	virtual void calculateBinRange() = 0;
	

	//initialize the camera. The camera set by vtk in iACompUniformTable and will be given to all other tables.
	virtual void initializeCamera() = 0;

	virtual std::vector<vtkSmartPointer<vtkActor>>* getOriginalRowActors() = 0;

	/*** Legend Initialization ***/
	//create correct label format
	std::string initializeLegendLabels(std::string input);
	//create the legend
	void initializeLegend();

	/*** Renderer Initialization ***/
	//setup rendering environment
	void initializeRenderer();
	//add the name of the dataset left beside its correpsonding row
	void addDatasetName(int currDataset, double* position);
	//draw a x-axis descripition, but only when univariate datasets are drawn
	void drawXAxis(double drawingDimensions[4]);
	//draw tick labels
	void addTickLabels(
		double minVal, double maxVal, vtkSmartPointer<vtkPoints> tickPoints);
	//draw ticks for every dataset
	vtkSmartPointer<vtkPoints> drawXTicks(
		double drawingDimensions[4], double yheigth, double tickLength);
	
	/*** Rendering ***/
	virtual void constructBins(iACompHistogramTableData* data, bin::BinType* currRowData,
		vtkSmartPointer<vtkDoubleArray> originArray,
		vtkSmartPointer<vtkDoubleArray> point1Array, vtkSmartPointer<vtkDoubleArray> point2Array, vtkSmartPointer<vtkUnsignedCharArray> colorArray, int currDataInd,
		int currentColumn, double offset);
	
	/*** Ordering/Ranking ***/
	//draws the bar chart for showing the number of objects for each dataset
	virtual void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset) = 0;

	//creates the bar actors for showing the number of objects for each dataset
	void createBarChart(vtkSmartPointer<vtkPolyData> currPolyData, int currAmountObjects, int maxAmountObjects);

	//creates the bar actors for showing the number of objects for each dataset
	void createBarChart(double* points, int currAmountObjects, int maxAmountObjects);

	/***  Interaction  ***/
	//get the selected dataset with its MDS values
	// and the selected dataset with its object IDs
	virtual std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> getSelectedData(
		Pick::PickedMap* map) = 0;

	//round the value to a certain decimal
	double round_up(double value, int decimal_places);

	
	iACompHistogramVis* m_vis;

	/*** Coloring ***/
	vtkSmartPointer<vtkLookupTable> m_lut;
	//stores the darker color lookup table
	vtkSmartPointer<vtkLookupTable> m_lutDarker;
	//defines whether the table should be drawn darker
	//is set true during drawing the bar chart to show the number of objects for each dataset
	bool m_useDarkerLut;
	//amount of colors
	int m_tableSize;

	
	/*** Rendering ***/
	//renderer for the color legend at the right side of the widget
	vtkSmartPointer<vtkRenderer> m_rendererColorLegend;
	
	//renderer for the color legend at the right side of the widget
	vtkSmartPointer<vtkRenderer> m_mainRenderer;

	//stores the state the visualization is in
	//as soon as showEvent() has finished the first time, the state is set to defined
	iACompVisOptions::lastState m_lastState;

	//number of ticks of the x-axis
	double m_numberOfTicks;
	
	/*** Ordering ***/
	//stores the bar actors drawn to show the number of objects for each dataset
	std::vector<vtkSmartPointer<vtkActor>>* m_barActors;
	//stores the text actors drawn to show the number of objects for each dataset
	std::vector<vtkSmartPointer<vtkTextActor>>* m_barTextActors;

	
	/*** Interaction ***/
	//stores the actors added to display the border of the selected cells
	//have to be removed before any calculation for zooming can take place!
	std::vector<vtkSmartPointer<vtkActor>>* m_highlighingActors;

	//stores the order of the row which was picked
	std::vector<int>* m_indexOfPickedRow;
	//stores for each picked actor/row the id of the cells that were picked
	//for each id stored in m_indexOfPickedRow, here the ids of the picked cells are stored
	std::map<int, std::vector<vtkIdType>*>* m_pickedCellsforPickedRow;

	//store bin data of selected rows that will be zoomed
	//each entry in the list represents a row, where any cell(or several) were selected.
	//the first entry is the most upper row that was selected, the ordering is then descending.
	//each entry has as many bins as cells were selected for this row
	QList<bin::BinType*>* m_zoomedRowData;

	//stores for each row which dataset is currently drawn inside
	std::map<vtkSmartPointer<vtkActor>, int>* m_rowDataIndexPair;

	double m_renderingView;

	//number of elements per color
	//is depending on the binning techique, especially on the bin containing the highest number of objects
	double m_BinRangeLength;

private:
	
	//creates the planes for the bar chart
	//positions contains the positions [x_min, x_max, y_min, y_max] (in exactly this order) of the related dataset row
	void createBars(double* positions);
	//creates the text actors for showing the number of objects for each dataset
	//positions contains the positions [x_min, y_min, y_max] (in exactly this order) of the related dataset row
	//currAmountObjects contains the number of objects/features in the dataset
	void createAmountOfObjectsText(double positions[3], int currAmountObjects);
	
};
