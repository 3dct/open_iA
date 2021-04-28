#include "iACompHistogramVis.h"

//CompVis
#include "iACompUniformTable.h"
#include "iACompHistogramTable.h"
#include "iACompHistogramTable.h"

//iA
#include "iAMainWindow.h"

//Qt
#include <QBoxLayout>

//vtk
#include "QVTKOpenGLNativeWidget.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkInteractorObserver.h"
#include "vtkRendererCollection.h"


iACompHistogramVis::iACompHistogramVis(iACompHistogramTable* table, iAMainWindow* parent, int amountDatasets) :
	m_main(table),
	QDockWidget(parent),
	m_amountDatasets(amountDatasets),
	m_initialRendering(true),
	m_windowWidth(-1),
	m_windowHeight(-1),
	m_drawingPositionForRegions(new std::map<int, std::vector<double>>()),
	m_activeVis(iACompVisOptions::activeVisualization::Undefined)
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

	m_qvtkWidget = new QVTKOpenGLNativeWidget(this);
	layout->addWidget(m_qvtkWidget);

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
	m_windowWidth = m_qvtkWidget->width();
	m_windowHeight = m_qvtkWidget->height();

	calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);
	m_initialRendering = false;

	//initialize drawing areas of rows for manual repositioning
	determineRowAreas();

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
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CombTable)
	{
	}
	else if (m_activeVis == iACompVisOptions::activeVisualization::CurveVisualization)
	{
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
		m_screenRatio = width / heigth;
		m_colSize = 1;
		m_rowSize = (m_screenRatio / numberOfDatasets);
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

	//TODO add additional visualizations


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

/*************** Rendering ****************************/

void iACompHistogramVis::showUniformTable()
{
	//remove the other visualization
	removeAllRendererFromWidget();

	//activate uniform table visualization
	m_uniformTable->setInteractorStyleToWidget();
	m_uniformTable->addRendererToWidget();
	m_uniformTable->setActive();

	m_activeVis = iACompVisOptions::activeVisualization::UniformTable;
}

void iACompHistogramVis::showVariableTable()
{ //TODO implement visualization of variable table
}

void iACompHistogramVis::showCombiTable()
{ //TODO implement visualization of combi table
}

void iACompHistogramVis::showCurve()
{  //TODO implement visualization of curve (kernel density estimation)
}

void iACompHistogramVis::addRendererToWidget(vtkSmartPointer<vtkRenderer> renderer)
{
	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetRenderWindow()->AddRenderer(renderer);
	#else
		m_qvtkWidget->renderWindow()->AddRenderer(renderer);
	#endif
}

void iACompHistogramVis::renderWidget()
{
	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetRenderWindow()->GetInteractor()->Render();
	#else
		m_qvtkWidget->renderWindow()->GetInteractor()->Render();
	#endif
}

void iACompHistogramVis::setInteractorStyleToWidget(vtkSmartPointer<vtkInteractorObserver> style)
{
	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_qvtkWidget->GetInteractor()->SetInteractorStyle(style);
	#else
		m_qvtkWidget->interactor()->SetInteractorStyle(style);
	#endif
}

void iACompHistogramVis::removeAllRendererFromWidget()
{
	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		
	#else
		vtkRendererCollection* rendererList = m_qvtkWidget->renderWindow()->GetRenderers();

		vtkCollectionSimpleIterator sit;
		rendererList->InitTraversal(sit);
		for (int i = 0; i < rendererList->GetNumberOfItems(); i++)
		{
			vtkRenderer* currRend = rendererList->GetNextRenderer(sit);
			m_qvtkWidget->renderWindow()->RemoveRenderer(currRend);
		}
	#endif
}

/****************************************** Getter & Setter **********************************************/
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

/****************************************** Recalculate Data Binning **********************************************/

iACompHistogramTableData* iACompHistogramVis::recalculateBinning(iACompVisOptions::binningType binningType, int numberOfBins)
{
	return m_main->recalculateBinning(binningType, numberOfBins);
}

iACompHistogramTableData* iACompHistogramVis::calculateSpecificBins(
	iACompVisOptions::binningType binningType, bin::BinType* data, int currBin, int amountOfBins)
{
	return m_main->calculateSpecificBins(binningType, data, currBin, amountOfBins);
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
		m_uniformTable->drawHistogramTableInAscendingOrder(m_uniformTable->getBins());
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

void iACompHistogramVis::drawDatasetsInDescendingOrder()
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->drawHistogramTableInDescendingOrder(m_uniformTable->getBins());
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

void iACompHistogramVis::drawDatasetsInOriginalOrder()
{
	if (m_activeVis == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_uniformTable->drawHistogramTableInOriginalOrder(m_uniformTable->getBins());
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
