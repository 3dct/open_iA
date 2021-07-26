#pragma once

#include "iACompVisOptions.h"

//vtk
#include "vtkSmartPointer.h"

//C++
#include <map>
 
//CompVis
class iACompTableInteractorStyle;

//vtk
class vtkColorTransferFunction;
class vtkLookupTable;
class iACompHistogramVis;
class vtkRenderer;
class vtkPlaneSource;
class vtkTextActor;
class vtkActor;


class iACompTable
{
public:
	iACompTable(iACompHistogramVis* vis);

	vtkSmartPointer<vtkRenderer> getRenderer();
	iACompHistogramVis* getHistogramVis();

	void addRendererToWidget();
	void addLegendRendererToWidget();
	void setInteractorStyleToWidget(vtkSmartPointer<iACompTableInteractorStyle> interactorStyle);
	void renderWidget();

	//set the visualization is active (it will be drawn)
	virtual void setActive() = 0;
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive() = 0;

	/*** Ordering/Ranking ***/
	//draw Histogram table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder(int bins) = 0;
	//draw Histogram table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder(int bins) = 0;
	//draw Histogram table with rows ordered according to loading the datasets
	virtual void drawHistogramTableInOriginalOrder(int bins) = 0;

	//draws the bar chart for showing the number of objects for each dataset
	virtual void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset) = 0;
	//creates the bar actors for showing the number of objects for each dataset
	void createBar(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects, int maxAmountObjects);
	//creates the text actors for showing the number of objects for each dataset
	void createAmountOfObjectsText(vtkSmartPointer<vtkPlaneSource> currPlane, int currAmountObjects);
	//get the boolean indicating that the bar chart visulaiztion showing the number of objects for each dataset is active
	bool getBarChartAmountObjectsActive();
	//remove bars from the visualization
	void removeBarCharShowingAmountOfObjects();

	/*** Update THIS****/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType) = 0;
	virtual void removeSelectionOfCorrelationMap() = 0;

protected:

	virtual void initializeTable() = 0;
	virtual void initializeInteraction() = 0;

	//create the color lookuptable
	virtual void makeLUTFromCTF() = 0;
	virtual void makeLUTDarker() = 0;

	//define the range of the color map bins for the visualization
	virtual void calculateBinRange() = 0;

	//initialize the camera. The camera set by vtk in iACompUniformTable and will be given to all other tables.
	virtual void initializeCamera() = 0;

	/*** Legend Initialization ***/
	//create correct label format
	std::string initializeLegendLabels(std::string input);
	//create the legend
	void initializeLegend();

	/*** Renderer Initialization ***/
	//setup rendering environment
	void initializeRenderer();

	//round the value to a certain decimal
	double round_up(double value, int decimal_places);

	//add the name of the dataset left beside its correpsonding row
	void addDatasetName(int currDataset, double* position);

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

	/*** Ordering ***/
	//stores the bar actors drawn to show the number of objects for each dataset
	std::vector<vtkSmartPointer<vtkActor>>* m_barActors;
	//stores the text actors drawn to show the number of objects for each dataset
	std::vector<vtkSmartPointer<vtkTextActor>>* m_barTextActors;

	
};
