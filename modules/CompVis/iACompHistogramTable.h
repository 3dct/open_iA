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
class vtkGenericOpenGLRenderWindow;
class vtkPlaneSource;
class vtkRenderer;

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
	void drawLinearZoom(Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber);

	//redraw the selected bin(s)/row(s) with a specified amount of bins
	//selectedBinNumber contains the number of bins that should be drawn for each selected bin in the row(s)
	void redrawZoomedRow(int selectedBinNumber);
	void drawPointRepresentation();
	void removePointRepresentation();

	//clear the list of stored selected rows
	void clearZoomedRows();

	int getBins();
	void setBins(int bins);
	int getBinsZoomed();
	void setBinsZoomed(int bins);

	const int getMinBins();
	const int getMaxBins();

	//returns the renderer of the visualization
	vtkSmartPointer<vtkRenderer> getRenderer();
	//re-render the widget/visualization
	void renderWidget();

	void getSelectedData(Pick::PickedMap* map);

   private:
	//calculate the histogram datastructure
	void calculateHistogramTable();

	//create the color lookuptable
	void makeLUTFromCTF();
	//color the planes according to the colors and the amount of elements each bin stores
	void makeCellData(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins);
	void makeCellDataForZoom(vtkUnsignedCharArray* colors, bin::BinType* data, int amountOfBins);
	void calculateCellData(vtkUnsignedCharArray* colors, bin::BinType* data, int amountOfBins);

	//create the histogramTable visualization
	void initializeHistogramTable();
	//define the maximum number of elements in a bin for visualization of the table
	void calculateCellRange();
	//create the legend
	void initializeLegend(vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow);
	void addDatasetName(int currDataset, double* position);
	//create correct label format
	std::string iACompHistogramTable::initializeLegendLabels(std::string input);
	//create the interactionstyle
	void initializeInteraction();

	bool checkCellRange(double value, double low, double high);
	//round the value to a certain decimal
	double round_up(double value, int decimal_places);

	void drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset);
	void drawRowZoomedRow(int currentColumn, int amountOfBins, bin::BinType* currentData, double offset);

	// Create the outline
	vtkSmartPointer<vtkActor> createOutline(vtkDataObject* object, double color[3]);

	iACompVisMain* m_main;
	iACsvDataStorage* m_dataStorage;
	std::vector<vtkSmartPointer<vtkActor>>* m_datasetNameActors;

	QList<csvFileData>* m_inputData;
	iAMultidimensionalScaling* m_mds;
	iACompHistogramTableData* m_histData;

	QVTKOpenGLNativeWidget* qvtkWidget;
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

	//int m_qvtkWidgetHeight;
	//int m_qvtkWidgetWidth;
	double screenRatio;

	//each dataset is one plane row
	const int m_ColForData = 1;
	//minimal amount of bins
	const int minBins = 10;
	//maximal amount of bins
	const int maxBins = 80;
	//amount of colors
	int m_tableSize;


	std::vector<int>* m_indexOfZoomedRows;
	//store bin data for zoom of selected rows
	QList<bin::BinType*>* zoomedRowData;
	//amount of bins that are drawn in the selected rows
	int m_binsZoomed;
	//stores the actors needed for the point representation
	std::vector<vtkSmartPointer<vtkActor>>* m_pointRepresentationActors;
};