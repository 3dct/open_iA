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

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
#include "iACompHistogramTableInteractorStyle.h"
#include "iACompHistogramTableData.h"
#include "ui_CompHistogramTable.h"

//Qt
#include <QDockWidget>

#include <vtkSmartPointer.h>

class iACsvDataStorage;
class iACompVisMain;
class iACompHistogramTableData;
class iAMainWindow;
class iAQVTKWidget;

class vtkLookupTable;
class vtkDataObject;
class vtkActor;
class vtkTextActor;
class vtkUnsignedCharArray;
class vtkPlaneSource;
class vtkRenderer;
class vtkPoints;

class iACompHistogramTable : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT

   public:
	iACompHistogramTable(iAMainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* m_dataStorage, iACompVisMain* main);
	void showEvent(QShowEvent* event);

	void reinitializeHistogramTable(iAMultidimensionalScaling* newMds);

	//draw Histogram table according to the similiarity values calculated for a picked row
	void drawHistogramTableAccordingToSimilarity(vtkSmartPointer<vtkActor> referenceData);
	//draw Histogram table according to the similiarity values calculated for a picked row AND CELL
	void drawHistogramTableAccordingToCellSimilarity(Pick::PickedMap* m_picked);
	//draw Histogram table with rows ordered ascending to its amount of objects
	void drawHistogramTableInAscendingOrder();
	//draw Histogram table with rows ordered descending to its amount of objects
	void drawHistogramTableInDescendingOrder();
	//draw Histogram table with rows ordered according to loading the datasets
	void drawHistogramTableInOriginalOrder();
	//draw Histogram table after manual repositioning is finsihed
	void drawReorderedHistogramTable();

	//draw initial Histogram Table
	void drawHistogramTable(int bins);
	
	//draws the selected row and bins
	//zoomed rows cannot be selected again --> when selected they only disappear
	//map contains the selected actor with its selected bins
	//selectedBinNumber represents number of bins of all other rows, which were not selected
	//selectedBinNumber represents number of bins that should be drawn in the zoomed in row
	void drawLinearZoom(Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber, QList<bin::BinType*>* zoomedRowData);

	//redraw the selected bin(s)/row(s) with a specified amount of bins
	//selectedBinNumber contains the number of bins that should be drawn for each selected bin in the row(s)
	void redrawZoomedRow(int selectedBinNumber);
	void drawPointRepresentation();
	void removePointRepresentation();

	int getBins();
	void setBins(int bins);
	int getBinsZoomed();
	void setBinsZoomed(int bins);

	int getMinBins();
	int getMaxBins();

	std::vector<int>* getIndexOfPickedRows();
	std::vector<int>* getAmountObjectsEveryDataset();

	vtkSmartPointer<vtkActor> getHighlightingRowActor();

	//return the actors representing the original rows
	std::vector<vtkSmartPointer<vtkActor>>* getOriginalRowActors();

	//returns the renderer of the visualization
	vtkSmartPointer<vtkRenderer> getRenderer();
	//re-render the widget/visualization
	void renderWidget();
	
	//get the selected dataset with its MDS values
	// and the selected dataset with its object IDs
	std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> getSelectedData(Pick::PickedMap* map);
	//highlight the selected cells with an outline
	void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId);
	void highlightSelectedRow(vtkSmartPointer<vtkActor> pickedActor);

	//dehighlight the selected cells with an outline 
	//(necessary that the renderer only contains the datarows for further calculations)
	void removeHighlightedCells();
	//dehighlihgte the selcted row
	bool removeHighlightedRow();

	void reorderHistogramTable(vtkSmartPointer<vtkActor> movingActor);
	//calculate the old position (drawing order) of the picked/moved plane
	void calculateOldDrawingPositionOfMovingActor(vtkSmartPointer<vtkActor> movingActor);

	//remove the bar chart visulaiztion showing the number of objects for each dataset
	void removeBarCharShowingAmountOfObjects();
	//get the boolean indicating that the bar chart visulaiztion showing the number of objects for each dataset is active
	bool getBarChartAmountObjectsActive();

	void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType);
	void removeSelectionOfCorrelationMap();

   private:
	//calculate the histogram datastructure
	void calculateHistogramTable();

	//create the color lookuptable
	void makeLUTFromCTF();
	void makeLUTDarker();
	//color the planes according to the colors and the amount of elements each bin stores
	void colorRow(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins);
	void colorRowForZoom(vtkUnsignedCharArray* colors, int currBin, bin::BinType* data, int amountOfBins);
	void colorBinsOfRow(vtkUnsignedCharArray* colors, bin::BinType* data, int amountOfBins);

	//create the histogramTable visualization
	void initializeHistogramTable();
	//define the maximum number of elements in a bin for visualization of the table
	void calculateBinLength();
	//create the legend
	void initializeLegend();
	//add the name of the dataset left beside its correpsonding row
	void addDatasetName(int currDataset, double* position);
	//create correct label format
	std::string initializeLegendLabels(std::string input);
	//create the interactionstyle
	void initializeInteraction();
	//initialize the order of the datasets from the last to the first
	void initializeOrderOfIndices();

	//round the value to a certain decimal
	double round_up(double value, int decimal_places);
	//sorts the input vector according to the given orderStyle ascending(0) or descending(1)
	std::vector<int>* sortWithMemory(std::vector<int> input, int orderStyle);
	std::vector<int>* sortWithMemory(std::vector<double> input, int orderStyle);
	std::vector<int>* reorderAccordingTo(std::vector<int>* newPositions);
	double calculateChiSquaredMetric(bin::BinType* observedFrequency, bin::BinType* expectedFrequency);

	//draws the bar chart for showing the number of objects for each dataset
	void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset);
	//creates the bar actors for showing the number of objects for each dataset
	void createBar(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects, int maxAmountObjects);
	//creates the text actors for showing the number of objects for each dataset
	void createAmountOfObjectsText(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects);

	//draw each row from bottom to top --> the higher the column number, the further on the top it is drawn
	//currDataInd: contains the index to the current datastructure
	//currentColumn: contains the index at which location it is drawn
	//amountOfBins: contains the number how many bins are drawn
	//offset contains: the offset by how much the plane will be drawn above the previous plane
	vtkSmartPointer<vtkPlaneSource> drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset);

	//draw the zoomed row beneath its parent row
	//currDataInd: contains the index to the current datastructure
	//currentColumn: contains the index at which location it is drawn
	//amountOfBins: contains the number how many bins are drawn per selected cell in the original plane
	//currentData is: the data of the selected cell in the original plane
	//offset contains: the offset by how much the zoomed plane will be drawn above the previous plane
	std::vector<vtkSmartPointer<vtkPlaneSource>>* drawZoomedRow(int currDataInd, int currentColumn, int amountOfBins, bin::BinType* currentData, double offsetHeight, std::vector<vtkIdType>* cellIdsOriginalPlane);
	
	vtkSmartPointer<vtkPlaneSource> drawZoomedPlanes(int bins, double startX, double startY, double endX, double endY, int currBinIndex, bin::BinType* currentData);
	
	//draw the line from each selected cell of the original row plane to the zoomed row plane and border the bins in the zoomed row accrodingly
	void drawLineBetweenRowAndZoomedRow(std::vector<vtkSmartPointer<vtkPlaneSource>>* zoomedRowPlanes, vtkSmartPointer<vtkPlaneSource> originalRowPlane, std::vector<vtkIdType>* cellIdsOriginalPlane);
	//draw a line from start- to endPoint with a certain color and width
	vtkSmartPointer<vtkActor> drawLine(double* startPoint, double* endPoint, double lineColor[3], double lineWidth);
	//draw a polyline according to specified points with a certain color and width
	vtkSmartPointer<vtkActor> drawPolyLine(vtkSmartPointer<vtkPoints> points, double lineColor[3], double lineWidth);
	vtkSmartPointer<vtkActor> drawPoints(vtkSmartPointer<vtkPoints> points, double color[3], double radius, double lineColor[3], double lineWidth);

	vtkSmartPointer<vtkPoints> calculatePointPosition(std::vector<double> dataPoints, double newMinX, double newMaxX, double y, std::vector<double> currMinMax);

	//calculate the height and width each row can have to fit into the screen
	void calculateRowWidthAndHeight(double width, double heigth, double numberOfDatasets);

	//calculate the new position (drawing order) of the picked/moved plane
	//returns the drawing position in which it is laying
	int calculateCurrentPosition(vtkSmartPointer<vtkActor> movingActor);
	void reorderIndices(int newDrawingPos, int oldDrawingPos);
	
	//initialize in which drawing area the a row with specified y coordinates belongs to
	//stores for each y-area = [minY,maxY] to which drawing position it belongs to
	void determineRowAreas();

	void drawStippledTexture(double* origin, double* point1, double* point2, double* color);

	iACompVisMain* m_main;
	iACsvDataStorage* m_dataStorage;
	std::vector<vtkSmartPointer<vtkActor>>* m_datasetNameActors;

	QList<csvFileData>* m_inputData;
	iAMultidimensionalScaling* m_mds;
	iACompHistogramTableData* m_histData;

	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkLookupTable> m_lut;

	
	//amount of colors
	int m_tableSize;


	//stores the actors needed for the point representation
	std::vector<vtkSmartPointer<vtkActor>>* m_pointRepresentationActors;
	//stores the actors that contain the original rows
	std::vector<vtkSmartPointer<vtkActor>>* m_originalPlaneActors;
	//stores the actors that contain the zoomed rows
	std::vector<vtkSmartPointer<vtkActor>>* m_zoomedPlaneActors;

	std::map<vtkSmartPointer<vtkActor>, std::vector< vtkSmartPointer<vtkActor> >* >* originalPlaneZoomedPlanePair;
	
	//stores the actors added to display the border of the selected cells
	//have to be removed before any calculation for zooming can take place!
	std::vector<vtkSmartPointer<vtkActor>>* m_highlighingActors;

	//stores for each picked actor/row the id of the cells that were picked
	//for each id stored in m_indexOfPickedRow, here the ids of the picked cells are stored
	std::map<int, std::vector<vtkIdType>*>* m_pickedCellsforPickedRow;

	//stores the order of the row which was picked
	std::vector<int>* m_indexOfPickedRow;

	//store bin data of selected rows that will be zoomed
	//each entry in the list represents a row, where any cell(or several) were selected.
	//the first entry is the most upper row that was selected, the ordering is then descending.
	//each entry has as many bins as cells were selected for this row
	QList<bin::BinType*>* m_zoomedRowData;

	//stores the order of the indices of each dataset
	//each dataset has an index between 0 and amountOfDatasets-1
	//initially the last dataset is stored first, descendingly
	//the position in the vector determines the drawingPosition --> 0 is bottom, m_amountDatasets-1 is top
	std::vector<int>* m_orderOfIndicesDatasets;

	//stores for each row which dataset is currently drawn inside
	std::map<vtkSmartPointer<vtkActor>, int>* m_rowDataIndexPair;

	//stores the ORIGINAL order of the indices of each dataset
	std::vector<int>* m_originalOrderOfIndicesDatasets;
	
	std::map<int, std::vector<double>>* m_drawingPositionForRegions;
	

	//stores the order of the indices of each dataset during the manual repositioning of an actor
	std::vector<int>* m_newOrderOfIndicesDatasets;

	//stores the actor showing the highlight for one row during the manual repositioning
	vtkSmartPointer<vtkActor> m_highlightRowActor;

	//defines whether the table should be drawn darker
	//is set true during drawing the bar chart to show the number of objects for each dataset
	bool m_useDarkerLut;
	//stores the darker color lookup table
	vtkSmartPointer<vtkLookupTable> m_lutDarker;
	//stores the bar actors drawn to show the number of objects for each dataset
	std::vector<vtkSmartPointer<vtkActor>>* m_barActors;
	//stores the text actors drawn to show the number of objects for each dataset
	std::vector<vtkSmartPointer<vtkTextActor>>* m_barTextActors;
	std::vector<vtkSmartPointer<vtkActor>>* m_stippledActors;

	bool m_initialRendering;

	int m_oldDrawingPosition;
	int m_newDrawingPosition;

	//number of elements per color
	double m_BinRangeLength;

	//amount of datasets
	int m_amountDatasets;
	//amount of bins
	int m_bins;
	//amount of bins that are drawn in the selected rows
	int m_binsZoomed;
	//number of planes
	double m_colSize;
	//number of subdivisions
	double m_rowSize;
	//stores the screen ratio
	double screenRatio;

	double m_windowWidth;
	double m_windowHeight;

	//each dataset is one plane row
	const int m_ColForData = 1;
	//minimal amount of bins
	const int minBins = 10;
	//maximal amount of bins
	const int maxBins = 80;
};
