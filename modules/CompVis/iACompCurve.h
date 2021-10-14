#pragma once

#include "iACompTable.h"
#include "iACompCurveInteractorStyle.h"
#include "iACompKernelDensityEstimationData.h"

//vtk
#include "vtkSmartPointer.h"

class iACompHistogramVis;
class iACompBayesianBlocksData;
class iACompNaturalBreaksData;
class vtkUnsignedCharArray;

class vtkPoints;
class vtkPolyData;

class iACompCurve : public iACompTable
{
public:

	iACompCurve(iACompHistogramVis* vis, iACompKernelDensityEstimationData* kdeData, double lineWidth, double opacity);

	//set the visualization is active (it will be drawn)
	virtual void setActive() override;
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive() override;

	/***  Getter & Setter ***/
	vtkSmartPointer<iACompCurveInteractorStyle> getInteractorStyle();
	virtual std::vector<vtkSmartPointer<vtkActor>>* getOriginalRowActors() override;

	/***  Ordering/Ranking  ***/
	//draw Curve table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder() override;
	//draw Curve table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder() override;
	//draw Curve table with rows ordered according to loading the datasets
	virtual void drawHistogramTableInOriginalOrder() override;
	//draws the bar chart for showing the number of objects for each dataset
	virtual void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset) override;
	
	/*** Rendering ***/
	//draw initial Histogram Table
	void drawHistogramTable();

	/*** Update THIS ***/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType) override;
	virtual void removeSelectionOfCorrelationMap() override;

	/*** Interaction Picking***/
	virtual void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId) override;
	virtual std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> getSelectedData(
		Pick::PickedMap* map) override;

protected:
	/***  Initialization  ***/
	virtual void initializeTable() override;
	virtual void initializeInteraction() override;
	void reinitalizeState();

	//create the color lookuptable
	virtual void makeLUTFromCTF() override;
	virtual void makeLUTDarker() override;

	//initialize the camera. The camera set by vtk in iACompUniformTable and will be given to this table.
	virtual void initializeCamera() override;

	//define the range of the color map bins for the visualization
	virtual void calculateBinRange() override;

	//define the range of the color map bins for the visualization
	//range of the bins of the color map is calculated by the uniform binning
	double calculateUniformBinRange();

	QList<kdeData::kdeBins>* getActiveData();
	
	QList<std::vector<double>>* getNumberOfObjectsInsideBin();

	/*** Rendering ***/
	virtual void drawRow(int currDataInd, int currentColumn, double offset);
	//draw white border around each datatset
	vtkSmartPointer<vtkPolyData> drawLine(vtkSmartPointer<vtkPoints> points);
	//draw curve segments
	vtkSmartPointer<vtkPolyData> drawCurve(vtkSmartPointer<vtkPoints> drawingPoints, int numberOfObjectsInsideBin);
	//draw inside of curve segments as polygons
	vtkSmartPointer<vtkPolyData> drawPolygon(vtkSmartPointer<vtkPoints> points, int numberOfObjectsInsideBin);
	//fill the color array according to the given colortable
	void colorCurve(vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkUnsignedCharArray> colorArray,
		int numberOfObjectsInsideBin);
	void colorPolygon(vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkUnsignedCharArray> colorArray,
		int numberOfObjectsInsideBin);

	double* computeColor(double numberOfObjects);
	void computePoints(
		kdeData::kdeBin* currBinData, int currentColumn, double offset, vtkSmartPointer<vtkPoints> points);

	/*** Coloring ***/
	double m_opacity;    //polygons should have a certain opacity between [0,1]
	double m_lineWidth;  //thickness of curve

	/*** Data Structure ***/
	iACompKernelDensityEstimationData* m_kdeData;

	//stores the actors that contain the original rows
	std::vector<vtkSmartPointer<vtkActor>>* m_originalRowActors;

private:
	
	/*** Interaction ***/
	vtkSmartPointer<iACompCurveInteractorStyle> m_interactionStyle;
};