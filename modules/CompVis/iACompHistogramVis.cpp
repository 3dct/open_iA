// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompHistogramVis.h"

//CompVis
#include "iACompUniformTable.h"
#include "iACompVariableTable.h"
#include "iACompHistogramTable.h"
#include "iACompCurve.h"
#include "iACompCombiTable.h"
#include "iACompUniformBinningData.h"
#include "iACompNaturalBreaksData.h"
#include "iACompBayesianBlocksData.h"

//iA
#include "iAMainWindow.h"
#include "iAQVTKWidget.h"

//Qt
#include <QBoxLayout>

//vtk
#include <vtkCamera.h>
#include <vtkInteractorObserver.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>

iACompHistogramVis::iACompHistogramVis(
	iACompHistogramTable* table, iAMainWindow* parent, int amountDatasets, bool MDSComputedFlag) :
	QDockWidget(parent),
	mainCamera(vtkSmartPointer<vtkCamera>::New()),
	m_amountDatasets(amountDatasets),
	m_AreaOpacity(0.25),  //0.15
	m_lineWidth(3),
	m_initialRendering(true),
	m_windowWidth(-1),
	m_windowHeight(-1),
	xAxis(false),
	m_drawingPositionForRegions(new std::map<int, std::vector<double>>()),
	m_main(table),
	m_activeVis(iACompVisOptions::activeVisualization::Undefined),
	m_activeBinning(iACompVisOptions::binningType::Undefined)
{
	//1. initialize variables
	m_orderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);
	m_originalOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);
	m_newOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	//2.initialize visualizations
	initializeVisualization();

	//3. initialize GUI
	setupUi(this);

	QVBoxLayout* layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new iAQVTKWidget(this);
	layout->addWidget(m_qvtkWidget);

	//4.initialize x-axis when univariate datasets are drawn
	if (!MDSComputedFlag)
	{
		xAxis = true;
	}
}
void iACompHistogramVis::reinitialize()
{
	initializeVisualization();
}

void iACompHistogramVis::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);

	if (m_initialRendering)
	{
		showInitally();
	}
	else
	{
		showAFresh();
	}
}

void iACompHistogramVis::showInitally()
{
	m_windowWidth = m_qvtkWidget->width() * 0.85;  /////////////////////m_uniformTable->getRenderingView()
	m_windowHeight = m_qvtkWidget->height();

	calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);
	m_initialRendering = false;

	//initialize drawing areas of rows for manual repositioning
	determineRowAreas();

	m_activeVis = iACompVisOptions::activeVisualization::UniformTable;
	m_activeBinning = iACompVisOptions::binningType::Uniform;
	showUniformTable();
}

void iACompHistogramVis::showAFresh()
{
	//initialize drawing areas of rows for manual repositioning
	determineRowAreas();

	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		showUniformTable();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::VariableTable)
	{
		showVariableTable();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{
		showCombiTable();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{
		showCurve();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::WhiteCurveVisualization)
	{
		showWhiteCurve();
	}
	else
	{
		showUniformTable();
	}
}

void iACompHistogramVis::calculateRowWidthAndHeight(double width, double heigth, double numberOfDatasets)
{
	if (heigth > width)
	{
		m_screenRatio = width/heigth;
		m_colSize = 1;
		m_rowSize = numberOfDatasets/m_screenRatio;
	}
	else
	{
		m_screenRatio = heigth / width;
		m_colSize = (m_screenRatio / numberOfDatasets);
		m_rowSize = 1;
	}
}

void iACompHistogramVis::determineRowAreas()
{
	//drawing position 0 = bottom until m_amountDatasets-1 = top
	for(int drawingPosition = 0; drawingPosition < m_amountDatasets; drawingPosition++)
	{
		double ymin = m_colSize * drawingPosition;
		double ymax = ymin + m_colSize;

		std::vector<double> yInterval = std::vector<double>(2, 0);
		yInterval.at(0) = ymin;
		yInterval.at(1) = ymax;

		m_drawingPositionForRegions->insert({drawingPosition, yInterval});
	}
}

void iACompHistogramVis::initializeVisualization()
{
	//initialize row of dataset index
	initializeOrderOfIndices();

	m_uniformTable = new iACompUniformTable(this, m_main->getUniformBinningData());

	m_variableTable = new iACompVariableTable(this, m_main->getBayesianBlocksData(), m_main->getNaturalBreaksData());

	m_curveTable = new iACompCurve(this, m_main->getKernelDensityEstimationData(), m_lineWidth,m_AreaOpacity);

	m_combiTable = new iACompCombiTable(this, m_main->getKernelDensityEstimationData(), m_lineWidth, m_AreaOpacity);

	//add additional visualizations
}

void iACompHistogramVis::initializeOrderOfIndices()
{
	int dataIndex = m_amountDatasets - 1;
	for (int position = 0; position < m_amountDatasets; position++)
	{
		m_orderOfIndicesDatasets->at(position) = dataIndex;
		m_originalOrderOfIndicesDatasets->at(position) = dataIndex;

		dataIndex--;
	}
}


/*************** Update Table Visualization ****************************/
void iACompHistogramVis::drawUniformTable()
{
	m_activeVis = iACompVisOptions::activeVisualization::UniformTable;
	m_activeBinning = iACompVisOptions::binningType::Uniform;
	showAFresh();
}

void iACompHistogramVis::drawBayesianBlocksTable()
{
	m_activeVis = iACompVisOptions::activeVisualization::VariableTable;
	m_activeBinning = iACompVisOptions::binningType::BayesianBlocks;
	showAFresh();
}

void iACompHistogramVis::drawNaturalBreaksTable()
{
	m_activeVis = iACompVisOptions::activeVisualization::VariableTable;
	m_activeBinning = iACompVisOptions::binningType::JenksNaturalBreaks;
	showAFresh();
}

void iACompHistogramVis::drawWhiteCurveTable()
{
	m_activeVis = iACompVisOptions::activeVisualization::WhiteCurveVisualization;
	showAFresh();
}

void iACompHistogramVis::drawCurveTable()
{
	m_activeVis = iACompVisOptions::activeVisualization::CurveVisualization;
	showAFresh();
}

void iACompHistogramVis::drawCombiTable()
{
	m_activeVis = iACompVisOptions::activeVisualization::CombTable;
	showAFresh();
}

/*************** Rendering ****************************/
void iACompHistogramVis::showUniformTable()
{
	//remove the other visualization
	removeAllRendererFromWidget();
	//m_variableTable->setInactive();

	//activate uniform table visualization
	m_uniformTable->setInteractorStyleToWidget(m_uniformTable->getInteractorStyle());
	m_uniformTable->getInteractorStyle()->setIACompHistogramVis(this);
	m_uniformTable->addRendererToWidget();
	m_uniformTable->setActive();
}

void iACompHistogramVis::showVariableTable()
{
	//remove the other visualization
	removeAllRendererFromWidget();

	m_variableTable->setActiveBinning(m_activeBinning);
	m_variableTable->setInteractorStyleToWidget(m_variableTable->getInteractorStyle());
	m_variableTable->getInteractorStyle()->setIACompHistogramVis(this);
	m_variableTable->addRendererToWidget();
	m_variableTable->setActive();
}

void iACompHistogramVis::showCombiTable()
{
	//remove the other visualization
	removeAllRendererFromWidget();
	//m_variableTable->setInactive();

	//activate combination table visualization
	m_combiTable->setInteractorStyleToWidget(m_combiTable->getInteractorStyle());
	m_combiTable->getInteractorStyle()->setIACompHistogramVis(this);
	m_combiTable->setUBBinData(m_main->getUniformBinningData()->getBinPolyData());
	m_combiTable->setNBBinData(m_main->getNaturalBreaksData()->getBinPolyData());
	m_combiTable->setBBBinData(m_main->getBayesianBlocksData()->getBinPolyData());
	m_combiTable->addRendererToWidget();
	m_combiTable->setActive();
}

void iACompHistogramVis::showCurve()
{
	//remove the other visualization
	removeAllRendererFromWidget();
	//m_variableTable->setInactive();

	//activate curve visualization
	m_curveTable->setInteractorStyleToWidget(m_curveTable->getInteractorStyle());
	m_curveTable->getInteractorStyle()->setIACompHistogramVis(this);
	m_curveTable->setUBBinData(m_main->getUniformBinningData()->getBinPolyData());
	m_curveTable->setNBBinData(m_main->getNaturalBreaksData()->getBinPolyData());
	m_curveTable->setBBBinData(m_main->getBayesianBlocksData()->getBinPolyData());
	m_curveTable->addRendererToWidget();
	m_curveTable->setActive();
}

void iACompHistogramVis::showWhiteCurve()
{
	m_curveTable->setDrawWhite(true);
	showCurve();

	m_curveTable->setDrawWhite(false);
}

void iACompHistogramVis::addRendererToWidget(vtkSmartPointer<vtkRenderer> renderer)
{
	m_qvtkWidget->renderWindow()->AddRenderer(renderer);
}

void iACompHistogramVis::renderWidget()
{
	//m_qvtkWidget->renderWindow()->GetInteractor()->Render();
	m_qvtkWidget->renderWindow()->Render(); ////////////////////////////////
}

void iACompHistogramVis::setInteractorStyleToWidget(vtkSmartPointer<vtkInteractorObserver> style)
{
	m_qvtkWidget->interactor()->SetInteractorStyle(style);
}

void iACompHistogramVis::removeAllRendererFromWidget()
{
	vtkRendererCollection* rendererList = m_qvtkWidget->renderWindow()->GetRenderers();

	vtkCollectionSimpleIterator sit;
	rendererList->InitTraversal(sit);
	for (int i = 0; i < rendererList->GetNumberOfItems(); i++)
	{
		vtkRenderer* currRend = rendererList->GetNextRenderer(sit);
		m_qvtkWidget->renderWindow()->RemoveRenderer(currRend);
	}
}

/****************************************** Getter & Setter **********************************************/
iACompVisOptions::binningType iACompHistogramVis::getActiveBinning()
{
	return m_activeBinning;
}

iACompVisOptions::activeVisualization iACompHistogramVis::getActiveVisualization()
{
	return m_activeVis;
}

int iACompHistogramVis::getAmountDatasets()
{
	return m_amountDatasets;
}

double iACompHistogramVis::getWindowHeight()
{
	return m_windowHeight;
}

double iACompHistogramVis::getWindowWidth()
{
	return m_windowWidth;
}

double iACompHistogramVis::getRowSize()
{
	return m_rowSize;
}

double iACompHistogramVis::getColSize()
{
	return m_colSize;
}

iACsvDataStorage* iACompHistogramVis::getDataStorage()
{
	return m_main->getDataStorage();
}

std::vector<int>* iACompHistogramVis::getOrderOfIndicesDatasets()
{
	return m_orderOfIndicesDatasets;
}

void iACompHistogramVis::setOrderOfIndicesDatasets(std::vector<int>* newOrderOfIndicesDatasets)
{
	m_orderOfIndicesDatasets = newOrderOfIndicesDatasets;
}

std::vector<int>* iACompHistogramVis::getNewOrderOfIndicesDatasets()
{
	return m_newOrderOfIndicesDatasets;
}

std::vector<int>* iACompHistogramVis::getOriginalOrderOfIndicesDatasets()
{
	return m_originalOrderOfIndicesDatasets;
}

std::map<int, std::vector<double>>* iACompHistogramVis::getDrawingPositionForRegions()
{
	return m_drawingPositionForRegions;
}

std::vector<int>* iACompHistogramVis::getAmountObjectsEveryDataset()
{
	return m_main->getAmountObjectsEveryDataset();
}

vtkSmartPointer<vtkCamera> iACompHistogramVis::getCamera()
{
	return mainCamera;
}

void iACompHistogramVis::setCamera(vtkSmartPointer<vtkCamera> newCamera)
{
	mainCamera = newCamera;
}

bool iACompHistogramVis::getXAxis(){
	return xAxis;
}

/****************************************** Recalculate Data Binning **********************************************/

iACompHistogramTableData* iACompHistogramVis::recalculateBinning(iACompVisOptions::binningType binningType, int numberOfBins)
{
	return m_main->recalculateBinning(binningType, numberOfBins);
}

iACompHistogramTableData* iACompHistogramVis::calculateSpecificBins(
	iACompVisOptions::binningType binningType, bin::BinType* dataBins, int currBin, int amountOfBins)
{
	return m_main->calculateSpecificBins(binningType, dataBins, currBin, amountOfBins);
}

void iACompHistogramVis::recomputeKernelDensityCurveUB()
{
	iACompKernelDensityEstimationData* kdeData = m_main->recomputeKernelDensityCurveUB();

	m_curveTable->setKDEData(kdeData);
	m_combiTable->setKDEData(kdeData);
}

/******************************************  Update  OTHERS **********************************************/
void iACompHistogramVis::resetOtherCharts()
{
	m_main->resetOtherCharts();
}

void iACompHistogramVis::updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	m_main->updateOtherCharts(selectedData, pickStatistic);
}

/******************************************  Update  THIS **********************************************/
void iACompHistogramVis::showSelectionOfCorrelationMap(std::map<int, double>* dataIndxSelectedType)
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->showSelectionOfCorrelationMap(dataIndxSelectedType);
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::VariableTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{  //TODO
	}
}

void iACompHistogramVis::removeSelectionOfCorrelationMap()
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->removeSelectionOfCorrelationMap();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::VariableTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{  //TODO
	}
}

/******************************************  Ordering/Ranking  **********************************/
void iACompHistogramVis::drawDatasetsInAscendingOrder()
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->drawHistogramTableInAscendingOrder();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::VariableTable)
	{
		m_variableTable->drawHistogramTableInAscendingOrder();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{  //TODO
	}
}

void iACompHistogramVis::drawDatasetsInDescendingOrder()
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->drawHistogramTableInDescendingOrder();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::VariableTable)
	{
		m_variableTable->drawHistogramTableInDescendingOrder();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{  //TODO
	}
}

void iACompHistogramVis::drawDatasetsInOriginalOrder()
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->drawHistogramTableInOriginalOrder();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::VariableTable)
	{
		m_variableTable->drawHistogramTableInOriginalOrder();
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{  //TODO
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{  //TODO
	}
}

std::vector<int>* iACompHistogramVis::sortWithMemory(std::vector<int> input, int orderStyle)
{
	std::vector<int>* newOrder = new std::vector<int>(input.size(), 0);
	int n(0);
	std::generate(std::begin(*newOrder), std::end(*newOrder), [&] { return n++; });

	if (orderStyle == 0)
	{  // ascending ordering
		auto comparator = [input](int i1, int i2) { return input.at(i1) < input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::less<int>());
	}
	else
	{  // descending ordering
		auto comparator = [input](int i1, int i2) { return input.at(i1) > input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::greater<int>());
	}

	return newOrder;
}

std::vector<int>* iACompHistogramVis::sortWithMemory(std::vector<double> input, int orderStyle)
{
	std::vector<int>* newOrder = new std::vector<int>(input.size(), 0.0);
	int n(0);
	std::generate(std::begin(*newOrder), std::end(*newOrder), [&] { return n++; });

	if (orderStyle == 0)
	{  //ascending ordering
		auto comparator = [input](double i1, double i2) { return input.at(i1) < input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::less<double>());
	}
	else
	{
		// descending ordering
		auto comparator = [input](double i1, double i2) { return input.at(i1) > input.at(i2); };
		std::sort(std::begin(*newOrder), std::end(*newOrder), comparator);

		std::sort(std::begin(input), std::end(input), std::greater<double>());
	}

	return newOrder;
}

std::vector<int>* iACompHistogramVis::reorderAccordingTo(std::vector<int>* newPositions)
{
	std::vector<int>* result = new std::vector<int>(newPositions->size(), 0);

	for (int i = 0; i < static_cast<int>(newPositions->size()); i++)
	{
		result->at((newPositions->size() - 1) - i) = newPositions->at(i);
	}

	return result;
}

void iACompHistogramVis::deactivateOrderingButton()
{
	m_main->deactivateOrderingButton();
}

void iACompHistogramVis::activateOrderingButton()
{
	m_main->activateOrderingButton();
}
