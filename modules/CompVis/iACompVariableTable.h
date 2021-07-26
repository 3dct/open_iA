#pragma once
#include "iACompTable.h"
#include "iACompBayesianBlocksData.h"
#include "iACompNaturalBreaksData.h"
#include "iACompVariableTableInteractorStyle.h"

//CompVis
class iACompHistogramVis;
//class iACompVariableTableInteractionStyle;

//vtk
class vtkUnsignedCharArray;
class vtkDoubleArray;


//because of vtk this method has to be placed outside the class
void buildGlyphRepresentation(void* arg);

class iACompVariableTable :public iACompTable
{
public:

    iACompVariableTable(iACompHistogramVis* vis, iACompBayesianBlocksData* bayesianBlocksData, iACompNaturalBreaksData* naturalBreaksData);

	//set the visualization is active (it will be drawn)
	virtual void setActive();
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive();
	//initialize the camera. The camera set by vtk in iACompUniformTable and will be given to this table.
	virtual void initializeCamera();

	void setActiveBinning(iACompVisOptions::binningType binningType);

	/***  Getter & Setter ***/
	vtkSmartPointer<iACompVariableTableInteractorStyle> getInteractorStyle();

	/***  Ordering/Ranking  ***/
	//draw Histogram table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder(int bins);
	//draw Histogram table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder(int bins);
	//draw Histogram table with rows ordered according to loading the datasets
	virtual void drawHistogramTableInOriginalOrder(int bins);
	//draws the bar chart for showing the number of objects for each dataset
	virtual void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset);

	/*** Rendering ***/
	//draw initial Histogram Table
	void drawHistogramTable();

	/*** Update THIS ***/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType);
	virtual void removeSelectionOfCorrelationMap();

protected:

	/***  Initialization  ***/
	virtual void initializeTable();
	virtual void initializeInteraction();

	//create the color lookuptable
	virtual void makeLUTFromCTF();
	virtual void makeLUTDarker();

	//define the range of the color map bins for the visualization
	void calculateBinRange();
	//define the range of the color map bins for the visualization
	//range of the bins of the color map is calculated by the uniform binning
	double calculateUniformBinRange();

private:

	//draw each row from bottom to top --> the higher the column number, the further on the top it is drawn
	//currDataInd: contains the index to the current datastructure
	//currentColumn: contains the index at which location it is drawn
	//offset contains: the offset by how much the plane will be drawn above the previous plane
	void drawRow(int currDataInd, int currentColumn, double offset);

	void constructBins(bin::BinType* currRowData, 
		vtkSmartPointer<vtkDoubleArray> originArray,
		vtkSmartPointer<vtkDoubleArray> point1Array,
		vtkSmartPointer<vtkDoubleArray> point2Array, 
		vtkSmartPointer<vtkUnsignedCharArray> colorArray,
		int currentColumn, 
		double offset);

	void reinitalizeState();


	//used for drawing computations
	//It either stores the bayesianBlocksData or the JenksNaturalBreaksData, depending on the interaction of the user
	iACompHistogramTableData* m_activeData;

    //datastructure containing the data points binned with Bayesian Blocks Method
	iACompBayesianBlocksData* m_bbData;
	//datastructure containing the data points binned with Natural Breaks Method
	iACompNaturalBreaksData* m_nbData;

	/*** Interaction ***/
	vtkSmartPointer<iACompVariableTableInteractorStyle> m_interactionStyle;

	std::vector<vtkSmartPointer<vtkPlaneSource>>* m_originalPlanes;
};

