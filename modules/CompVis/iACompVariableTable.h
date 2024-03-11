// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompTable.h"
#include "iACompBayesianBlocksData.h"
#include "iACompNaturalBreaksData.h"
#include "iACompVariableTableInteractorStyle.h"

//CompVis
class iACompHistogramVis;

//vtk
class vtkActor;

class iACompVariableTable : public iACompTable
{
public:

    iACompVariableTable(iACompHistogramVis* vis, iACompBayesianBlocksData* bayesianBlocksData, iACompNaturalBreaksData* naturalBreaksData);

	//set the visualization is active (it will be drawn)
	virtual void setActive() override;
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive() override;
	//initialize the camera. The camera set by vtk in iACompUniformTable and will be given to this table.
	virtual void initializeCamera() override;

	void setActiveBinning(iACompVisOptions::binningType binningType);

	/***  Getter & Setter ***/
	vtkSmartPointer<iACompVariableTableInteractorStyle> getInteractorStyle();
	virtual std::vector<vtkSmartPointer<vtkActor>>* getOriginalRowActors() override;

	/***  Ordering/Ranking  ***/
	//draw Histogram table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder() override;
	//draw Histogram table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder() override;
	//draw Histogram table with rows ordered according to loading the datasets
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

	//define the range of the color map bins for the visualization
	void calculateBinRange() override;
	//define the range of the color map bins for the visualization
	//range of the bins of the color map is calculated by the uniform binning
	double calculateUniformBinRange();

private:

	//draw each row from bottom to top --> the higher the column number, the further on the top it is drawn
	//currDataInd: contains the index to the current datastructure
	//currentColumn: contains the index at which location it is drawn
	//offset contains: the offset by how much the plane will be drawn above the previous plane
	void drawRow(int currDataInd, int currentColumn, double offset);

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

	//stores the actors that contain the original rows
	std::vector<vtkSmartPointer<vtkActor>>* m_originalRowActors;




};
