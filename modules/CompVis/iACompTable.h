#pragma once

#include "iACompVisOptions.h"

//vtk
#include "vtkSmartPointer.h"

//C++
#include <map>
 
//CompVis
class iACompUniformTableInteractorStyle;

//vtk
class vtkColorTransferFunction;
class vtkLookupTable;
class iACompHistogramVis;
class vtkRenderer;

class iACompTable
{
public:
	iACompTable(iACompHistogramVis* vis);

	vtkSmartPointer<vtkRenderer> getRenderer();
	iACompHistogramVis* getHistogramVis();

	void addRendererToWidget();
	void addLegendRendererToWidget();
	void setInteractorStyleToWidget();
	void renderWidget();

	//set the visualization is active (it will be drawn)
	virtual void setActive() = 0;
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive() = 0;

	//draw Histogram table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder(int bins) = 0;
	//draw Histogram table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder(int bins) = 0;
	//draw Histogram table with rows ordered according to loading the datasets
	virtual void drawHistogramTableInOriginalOrder(int bins) = 0;

	/*** Update THIS****/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType) = 0;
	virtual void removeSelectionOfCorrelationMap() = 0;

protected:

	virtual void initializeTable() = 0;
	virtual void initializeInteraction() = 0;

	//create the color lookuptable
	virtual void makeLUTFromCTF() = 0;
	virtual void makeLUTDarker() = 0;

	//setup rendering environment
	void initializeRenderer();

	//round the value to a certain decimal
	double round_up(double value, int decimal_places);

	//add the name of the dataset left beside its correpsonding row
	void addDatasetName(int currDataset, double* position);
	

	iACompHistogramVis* m_vis;

	/*************** Coloring ****************************/
	vtkSmartPointer<vtkLookupTable> m_lut;
	//stores the darker color lookup table
	vtkSmartPointer<vtkLookupTable> m_lutDarker;
	//defines whether the table should be drawn darker
	//is set true during drawing the bar chart to show the number of objects for each dataset
	bool m_useDarkerLut;
	//amount of colors
	int m_tableSize;

	/*************** Rendering ****************************/
	//renderer for the color legend at the right side of the widget
	vtkSmartPointer<vtkRenderer> m_rendererColorLegend;
	//renderer for the color legend at the right side of the widget
	vtkSmartPointer<vtkRenderer> m_mainRenderer;

	/*************** Interaction ****************************/
	vtkSmartPointer<iACompUniformTableInteractorStyle> m_interactionStyle;
};
