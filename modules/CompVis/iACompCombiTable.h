#pragma once
#include "iACompCurve.h"
#include "iACompCombiTableInteractionStyle.h"
#include "iACompKernelDensityEstimationData.h"

//vtk
#include "vtkSmartPointer.h"

class vtkPolyData;
class vtkPoints;

class iACompCombiTable : public iACompCurve
{
public:

	iACompCombiTable(
		iACompHistogramVis* vis, iACompKernelDensityEstimationData* kdeData, double lineWidth, double opacity);

	/***  Getter & Setter ***/
	vtkSmartPointer<iACompCombiTableInteractionStyle> getInteractorStyle();

	//draws the bar chart for showing the number of objects for each dataset
	virtual void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset) override;

	/*** Update THIS ***/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType) override;
	virtual void removeSelectionOfCorrelationMap() override;

	/*** Interaction Picking***/
	virtual void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId) override;
	virtual std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> getSelectedData(
		Pick::PickedMap* map) override;

protected:

	/*** Rendering ***/
	//draw initial Histogram Table
	virtual void drawHistogramTable() override;
	virtual void drawRow(int currDataInd, int currentColumn, double offset) override;

private:

	/*** Rendering ***/
	/**
	 * @brief draws all bins of all datasets
	 * @param binPolyData - contains the drawing positions and color of the bins
	*/
	void drawBins(QList<vtkSmartPointer<vtkPolyData>>* binPolyData);
	vtkSmartPointer<vtkPolyData> drawCurve(double drawingDimensions[4], kdeData::kdeBins currDataset,
		vtkSmartPointer<vtkPolyData> currBinPolyData, int currDataInd, int currentColumn, double offset);

	/*** Interaction ***/
	vtkSmartPointer<iACompCombiTableInteractionStyle> m_interactionStyle;
};