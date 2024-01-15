// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompTable.h"
#include "iACompUniformTableInteractorStyle.h"

//CompVis
class iACompUniformBinningData;
class iACompHistogramVis;
class iACompHistogramTableData;

//vtk
class vtkActor;
class vtkPlaneSource;
class vtkUnsignedCharArray;
class vtkTextActor;

class iACompUniformTable : public iACompTable
{

public:

	iACompUniformTable(iACompHistogramVis* vis, iACompUniformBinningData* uniformBinningData);

	//set the visualization is active (it will be drawn)
	virtual void setActive() override;
	//set the visualization inactive (it will no longer be drawn)
	virtual void setInactive() override;
	//initialize the camera. The camera set by vtk in this view will be given to all other tables
	virtual void initializeCamera() override;

	/******************************************  Ordering/Ranking  **********************************/
	//draw Histogram table with rows ordered ascending to its amount of objects
	virtual void drawHistogramTableInAscendingOrder() override;
	//draw Histogram table with rows ordered descending to its amount of objects
	virtual void drawHistogramTableInDescendingOrder() override;
	//draw Histogram table with rows ordered according to loading the datasets
	virtual void drawHistogramTableInOriginalOrder() override;

	/******************************************  Rendering  **********************************/
	//draw initial Histogram Table
	void drawHistogramTable(int bins);
	//draw Histogram table according to the similiarity values calculated for a picked row AND CELL
	void drawHistogramTableAccordingToCellSimilarity(Pick::PickedMap* m_picked);
	//draw Histogram table according to the similiarity values calculated for a picked row
	void drawHistogramTableAccordingToSimilarity(vtkSmartPointer<vtkActor> referenceData);
	//draw Histogram table after manual repositioning is finsihed
	void drawReorderedHistogramTable();

	//draws the selected row and bins
	//zoomed rows cannot be selected again --> when selected they only disappear
	//map contains the selected actor with its selected bins
	//selectedBinNumber represents number of bins of all other rows, which were not selected
	//selectedBinNumber represents number of bins that should be drawn in the zoomed in row
	void drawNonLinearZoom(
		Pick::PickedMap* map, int notSelectedBinNumber, int selectedBinNumber, QList<bin::BinType*>* zoomedRowData);

	//redraw the selected bin(s)/row(s) with a specified amount of bins
	//selectedBinNumber contains the number of bins that should be drawn for each selected bin in the row(s)
	void zoomInZoomedRow(int selectedBinNumber);

	void drawPointRepresentation();

	/******************************************  Interaction  **********************************************/
	
	//dehighlihgte the selcted row
	bool removeHighlightedRow();
	//highlight the selected cells with an outline
	virtual void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId) override;
	void highlightSelectedRow(vtkSmartPointer<vtkActor> pickedActor);

	void removePointRepresentation();

	//remove the bar chart visulaiztion showing the number of objects for each dataset
	void removeBarCharShowingAmountOfObjects();

	void reorderHistogramTable(vtkSmartPointer<vtkActor> movingActor);

	//calculate the old position (drawing order) of the picked/moved plane
	void calculateOldDrawingPositionOfMovingActor(vtkSmartPointer<vtkActor> movingActor);

	//get the selected dataset with its MDS values
	// and the selected dataset with its object IDs
	virtual std::tuple<QList<bin::BinType*>*, QList<std::vector<csvDataType::ArrayType*>*>*> getSelectedData(
		Pick::PickedMap* map) override;

	/****************************************** Recompute Binning **********************************************/
	void recomputeBinning();

	/******************************************  Getter & Setter  **********************************************/
	//return the actors representing the original rows
	virtual std::vector<vtkSmartPointer<vtkActor>>* getOriginalRowActors() override;

	vtkSmartPointer<vtkActor> getHighlightingRowActor();
	int getBins();
	void setBins(int bins);
	int getBinsZoomed();
	void setBinsZoomed(int bins);
	int getMinBins();
	int getMaxBins();

	vtkSmartPointer<iACompUniformTableInteractorStyle> getInteractorStyle();

	/******************************************  Update THIS  **********************************************/
	virtual void showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType) override;
	virtual void removeSelectionOfCorrelationMap() override;
	
protected:

	virtual void initializeTable() override;
	virtual void initializeInteraction() override;

	//define the range of the bins for the visualization
	//for uniform binning, the range of the objects is divded by the number of bins
	virtual void calculateBinRange() override;

private:

	vtkSmartPointer<vtkPolyData> initializeRow(int numberOfBins);

	/******************************************  Rendering  *******************************************/
	//draw each row from bottom to top --> the higher the column number, the further on the top it is drawn
	//currDataInd: contains the index to the current datastructure
	//currentColumn: contains the index at which location it is drawn
	//amountOfBins: contains the number how many bins are drawn
	//offset contains: the offset by how much the plane will be drawn above the previous plane
	vtkSmartPointer<vtkPolyData> drawRow(int currDataInd, int currentColumn, int amountOfBins, double offset);

	//draw the zoomed row beneath its parent row
	//currDataInd: contains the index to the current datastructure
	//currentColumn: contains the index at which location it is drawn
	//amountOfBins: contains the number how many bins are drawn per selected cell in the original plane
	//currentData is: the data of the selected cell in the original plane
	//offset contains: the offset by how much the zoomed plane will be drawn above the previous plane
	std::vector<vtkSmartPointer<vtkPolyData>>* drawZoomedRow(int currentColumn, int amountOfBins,
		bin::BinType* currentData, double offsetHeight, std::vector<vtkIdType>* cellIdsOriginalPlane);
	
	//draw the individual zoomed row for a cell that was selected in its original row
	vtkSmartPointer<vtkPolyData> drawZoomedPlane(
		int bins, double startX, double startY, double endX, double endY, int currBinIndex, bin::BinType* currentData);
	
	//draw the line from each selected cell of the original row plane to the zoomed row plane and border the bins in the zoomed row accrodingly
	void drawLineBetweenRowAndZoomedRow(std::vector<vtkSmartPointer<vtkPolyData>>* zoomedRowPlanes,
		vtkSmartPointer<vtkPolyData> originalRowPlane, std::vector<vtkIdType>* cellIdsOriginalPlane);
	
	//draw a line from start- to endPoint with a certain color and width
	vtkSmartPointer<vtkActor> drawLine(double* startPoint, double* endPoint, double lineColor[3], double lineWidth);
	void drawStippledTexture(double* origin, double* point1, double* point2, double* color);
	
	//color the planes according to the colors and the amount of elements each bin stores
	void colorRow(vtkUnsignedCharArray* colors, int currDataset, int numberOfBins);
	void colorBinOfRow(vtkUnsignedCharArray* colors, int position, double numberOfObjectsInBin);
	void drawZoomForZoomedRow(
		vtkSmartPointer<vtkActor> zoomedRowActor, int currBin, bin::BinType* data, int amountOfBins);

	//draw a polyline according to specified points with a certain color and width
	vtkSmartPointer<vtkActor> drawPolyLine(vtkSmartPointer<vtkPoints> points, double lineColor[3], double lineWidth);
	
	vtkSmartPointer<vtkActor> drawPoints(
		vtkSmartPointer<vtkPoints> points, double color[3], double radius, double lineColor[3], double lineWidth);

	vtkSmartPointer<vtkPoints> calculatePointPosition(
		std::vector<double> dataPoints, double newMinX, double newMaxX, double y);

	/******************************************  Ordering/Ranking  **********************************/
	//draws the bar chart for showing the number of objects for each dataset
	virtual void drawBarChartShowingAmountOfObjects(std::vector<int> amountObjectsEveryDataset) override;

	double calculateChiSquaredMetric(bin::BinType* observedFrequency, bin::BinType* expectedFrequency);

	//calculate the new position (drawing order) of the picked/moved plane
	//returns the drawing position in which it is laying
	int calculateCurrentPosition(vtkSmartPointer<vtkActor> movingActor);
	void reorderIndices(int newDrawingPos, int oldDrawingPos);

	//datastructure containing the binned data points
	iACompUniformBinningData* m_uniformBinningData;

	//stores the actors that contain the original rows
	std::vector<vtkSmartPointer<vtkActor>>* m_originalPlaneActors;
	//stores the actors that contain the zoomed rows
	std::vector<vtkSmartPointer<vtkActor>>* m_zoomedPlaneActors;

	//number of elements per color
	//double m_BinRangeLength;

	std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>* originalPlaneZoomedPlanePair;

	//amount of bins
	int m_bins;
	//amount of bins that are drawn in the selected rows
	int m_binsZoomed;
	//minimal amount of bins
	int minBins;
	//maximal amount of bins
	const int maxBins = 80;
	//each dataset is one plane row
	const int m_ColForData = 1;

	/******************************************  Interaction  **********************************************/

	//stores the actor showing the highlight for one row during the manual repositioning
	vtkSmartPointer<vtkActor> m_highlightRowActor;
	//stores the actors needed for the point representation
	std::vector<vtkSmartPointer<vtkActor>>* m_pointRepresentationActors;

	std::vector<vtkSmartPointer<vtkActor>>* m_stippledActors;

	int m_oldDrawingPosition;
	int m_newDrawingPosition;

	vtkSmartPointer<iACompUniformTableInteractorStyle> m_interactionStyle;
};
