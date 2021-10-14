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
		virtual void drawRow(int currDataInd, int currentColumn, double offset) override;

private:

	/*** Rendering ***/

	/**
	 * @brief draws a single bin of an dataset
	 * @param drawingPoints - contains for all bins the lower and upper boundary [x_min, x_max]
	 * @param numberOfObjectsInsideBin - contains the number of objects which are located inside each bin
	 * @param drawingDimensions - contains the dimensions of the drawing area [x_min, x_max, y_min, y_max]
	*/
	void drawBins(std::vector<std::vector<double>>* drawingPoints, std::vector<double>* numberOfObjectsInBins, double drawingDimensions[4]);

	/*** Interaction ***/
	vtkSmartPointer<iACompCombiTableInteractionStyle> m_interactionStyle;
};