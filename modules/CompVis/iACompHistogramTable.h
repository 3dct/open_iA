#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
#include "iACompHistogramTableInteractorStyle.h"
#include "iACompHistogramTableData.h"

//iA
#include "charts/iAHistogramData.h"

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

#include <vtkSmartPointer.h>

class MainWindow;
class iACsvDataStorage;
class QVTKOpenGLNativeWidget;
class vtkLookupTable;
class vtkDataObject;
class vtkActor;
class vtkUnsignedCharArray;
class vtkPlaneSource;
class vtkRenderer;
class vtkPoints;

class iACompVisMain;


//testing

class iACompHistogramTableData;

class iACompHistogramTable : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT
   public:
	iACompHistogramTable(MainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* m_dataStorage, iACompVisMain* main);
	void showEvent(QShowEvent* event);

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

	const int getMinBins();
	const int getMaxBins();

	//return the actors representing the original rows
	std::vector<vtkSmartPointer<vtkActor>>* getOriginalRowActors();

	//returns the renderer of the visualization
	vtkSmartPointer<vtkRenderer> getRenderer();
	//re-render the widget/visualization
	void renderWidget();

	QList<bin::BinType*>* getSelectedData(Pick::PickedMap* map);
	//highlight the selected cells with an outline
	void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId);
	//dehighlight the selected cells with an outline 
	//(necessary that the renderer only contains the datarows for further calculations)
	void iACompHistogramTable::removeHighlightedCells();

   private:
	//calculate the histogram datastructure
	void calculateHistogramTable();

	//create the color lookuptable
	void makeLUTFromCTF();
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
	std::string iACompHistogramTable::initializeLegendLabels(std::string input);
	//create the interactionstyle
	void initializeInteraction();

	//round the value to a certain decimal
	double round_up(double value, int decimal_places);

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

	vtkSmartPointer<vtkPoints> calculatePointPosition(std::vector<double> dataPoints, double newMinX, double newMaxX, double y);

	//calculate the height and width each row can have to fit into the screen
	void calculateRowWidthAndHeight(double width, double heigth, double numberOfDatasets);


	iACompVisMain* m_main;
	iACsvDataStorage* m_dataStorage;
	std::vector<vtkSmartPointer<vtkActor>>* m_datasetNameActors;

	QList<csvFileData>* m_inputData;
	iAMultidimensionalScaling* m_mds;
	iACompHistogramTableData* m_histData;

	QVTKOpenGLNativeWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkLookupTable> m_lut;

	//amount of bins
	int m_bins;
	//amount of datasets
	int m_amountDatasets;
	//number of elements per color
	double m_BinRangeLength;
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
	//amount of colors
	int m_tableSize;

	//amount of bins that are drawn in the selected rows
	int m_binsZoomed;

	//stores the actors needed for the point representation
	std::vector<vtkSmartPointer<vtkActor>>* m_pointRepresentationActors;
	//stores the actors that contain the original rows
	std::vector<vtkSmartPointer<vtkActor>>* m_originalPlaneActors;
	//stores the actors that contain the zoomed rows
	std::vector<vtkSmartPointer<vtkActor>>* m_zoomedPlaneActors;

	std::map<vtkSmartPointer<vtkActor>, std::vector< vtkSmartPointer<vtkActor> >* >* originalPlaneZoomedPlanePair;
	std::map<vtkSmartPointer<vtkActor>, int>* originalPlaneIndexPair;

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

};