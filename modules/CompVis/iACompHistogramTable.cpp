#include "iACompHistogramTable.h"

//Debug
#include "iALog.h"

//CompVis
#include "iACompVisOptions.h"
#include "iACompVisMain.h"
#include "iACompHistogramTableData.h"
#include "iACompUniformBinningData.h"

//iA
#include <iAMainWindow.h>
#include <iAVtkWidget.h>
#include <iAVtkVersion.h>

//Qt
#include <QColor>

//vtk
#include <vtkActor.h>

#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkLookupTable.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>

#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataObject.h>
#include <vtkNamedColors.h>
#include <vtkOutlineFilter.h>

#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkUnsignedCharArray.h>

#include <QVTKInteractor.h>
#include <vtkActorCollection.h>
#include <vtkCamera.h>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkLineSource.h>
#include <vtkPolyLine.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSphereSource.h>
#include <vtkGlyph2D.h>

#include <vtkBillboardTextActor3D.h>
#include <vtkCaptionActor2D.h>
#include <vtkFollower.h>
#include <vtkTextActor.h>
#include <vtkTextRepresentation.h>
#include <vtkTextWidget.h>
#include <vtkVectorText.h>

#include <vtkCamera.h>

#include <vtkDataSetMapper.h>
#include <vtkExtractSelection.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGrid.h>

#include <vtkShrinkPolyData.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <functional>
#include <limits>
#include <tuple>
#include <vector>


iACompHistogramTable::iACompHistogramTable(
	iAMainWindow* parent, iACsvDataStorage* dataStorage, iACompVisMain* main, bool MDSComputedFlag) :
	m_main(main),
	m_inputData(dataStorage->getData()),
	m_dataStorage(dataStorage)
{
	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = (int)dataResolution->size();

	//initialize datastructure
	initializeBinCalculation(MDSComputedFlag);

	//initialize visualization
	histogramVis = new iACompHistogramVis(this, parent, m_amountDatasets);
}

void iACompHistogramTable::initializeBinCalculation(bool mdsComputedFlag)
{
	histogramCalculation = new iACompHistogramCalculation(m_dataStorage, mdsComputedFlag);
	histogramCalculation->calculateUniformBinning();
	histogramCalculation->calculateBayesianBlocks();
	histogramCalculation->calculateNaturalBreaks();
	histogramCalculation->calculateDensityEstimation();
	
	//add new binning methods here
}

void iACompHistogramTable::reinitializeHistogramTable(iAMultidimensionalScaling* newMds)
{ //TODO reinitialize HistogramTable for recalculateMDS!
	/*
	m_mds = newMds;
	m_inputData = m_mds->getCSVFileData();

	m_BinRangeLength = 0;
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	m_tableSize = 10;

	m_pointRepresentationActors->clear();
	delete m_pointRepresentationActors;
	m_pointRepresentationActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_datasetNameActors->clear();
	delete m_datasetNameActors;
	m_datasetNameActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_highlighingActors->clear();
	delete m_highlighingActors;
	m_highlighingActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_originalPlaneActors->clear();
	delete m_originalPlaneActors;
	m_originalPlaneActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_zoomedPlaneActors->clear();
	delete m_zoomedPlaneActors;
	m_zoomedPlaneActors = new std::vector<vtkSmartPointer<vtkActor>>();

	originalPlaneZoomedPlanePair->clear();
	delete originalPlaneZoomedPlanePair;
	originalPlaneZoomedPlanePair = new std::map<vtkSmartPointer<vtkActor>, std::vector<vtkSmartPointer<vtkActor>>*>();
	
	m_rowDataIndexPair->clear();
	delete m_rowDataIndexPair;
	m_rowDataIndexPair = new std::map<vtkSmartPointer<vtkActor>, int>();
	
	m_drawingPositionForRegions->clear();
	delete m_drawingPositionForRegions;
	m_drawingPositionForRegions = new std::map<int, std::vector<double>>();
	
	m_highlightRowActor = vtkSmartPointer<vtkActor>::New();
	
	m_useDarkerLut = false;
	
	m_lutDarker = vtkSmartPointer<vtkLookupTable>::New();

	m_barActors->clear();
	delete m_barActors;
	m_barActors = new std::vector<vtkSmartPointer<vtkActor>>();

	m_barTextActors->clear();
	delete m_barTextActors;
	m_barTextActors = new std::vector<vtkSmartPointer<vtkTextActor>>();

	m_stippledActors->clear();
	delete m_stippledActors;
	m_stippledActors = new std::vector<vtkSmartPointer<vtkActor>>();

	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = dataResolution->size();

	m_orderOfIndicesDatasets->clear();
	delete m_orderOfIndicesDatasets;
	m_orderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_originalOrderOfIndicesDatasets->clear();
	delete m_originalOrderOfIndicesDatasets;
	m_originalOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_newOrderOfIndicesDatasets->clear();
	delete m_newOrderOfIndicesDatasets;
	m_newOrderOfIndicesDatasets = new std::vector<int>(m_amountDatasets, 0);

	m_bins = minBins;
	m_binsZoomed = minBins;

	//initialize datastructure
	calculateHistogramTable();

	m_qvtkWidget->renderWindow()->RemoveRenderer(m_renderer);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	double col1[3];
	iACompVisOptions::getDoubleArray(iACompVisOptions::BACKGROUNDCOLOR_GREY, col1);
	m_renderer->SetBackground(col1);
	m_renderer->SetViewport(0, 0, 0.8, 1);
	m_renderer->SetUseFXAA(true);

	m_qvtkWidget->renderWindow()->AddRenderer(m_renderer);

	calculateRowWidthAndHeight(m_windowWidth, m_windowHeight, m_amountDatasets);

	//initialize drawing areas of rows for manual repositioning
	determineRowAreas();

	//initialize visualization
	initializeHistogramTable();
	*/
}

std::vector<int>* iACompHistogramTable::getAmountObjectsEveryDataset()
{
	return csvFileData::getAmountObjectsEveryDataset(m_dataStorage->getData());
}

iACsvDataStorage* iACompHistogramTable::getDataStorage()
{
	return m_dataStorage;
}

iACompHistogramVis* iACompHistogramTable::getHistogramTableVis()
{
	return histogramVis;
}

iACompUniformBinningData* iACompHistogramTable::getUniformBinningData()
{
	return histogramCalculation->getUniformBinningData();
}

iACompBayesianBlocksData* iACompHistogramTable::getBayesianBlocksData()
{
	return histogramCalculation->getBayesianBlocksData();
}

iACompNaturalBreaksData* iACompHistogramTable::getNaturalBreaksData()
{
	return histogramCalculation->getNaturalBreaksData();
}

iACompKernelDensityEstimationData* iACompHistogramTable::getKernelDensityEstimationData()
{
	return histogramCalculation->getKernelDensityEstimationData();
}

void iACompHistogramTable::resetOtherCharts()
{
	m_main->resetOtherCharts();
}

void iACompHistogramTable::updateOtherCharts(
	csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic)
{
	m_main->updateOtherCharts(selectedData, pickStatistic);
}

iACompHistogramTableData* iACompHistogramTable::recalculateBinning(iACompVisOptions::binningType binningType, int numberOfBins)
{
	if (iACompVisOptions::binningType::Uniform == binningType)
	{
		histogramCalculation->calculateUniformBinning(numberOfBins);
		return histogramCalculation->getUniformBinningData();
	}
	else if (iACompVisOptions::binningType::JenksNaturalBreaks == binningType)
	{ // TODO: Implement
	
	}
	else if (iACompVisOptions::binningType::BayesianBlocks == binningType)
	{ // TODO: Implement
	
	}
	
	return nullptr;
}

iACompHistogramTableData* iACompHistogramTable::calculateSpecificBins(
	iACompVisOptions::binningType binningType, bin::BinType* data, int currBin, int amountOfBins)
{
	if (iACompVisOptions::binningType::Uniform == binningType)
	{
		histogramCalculation->calculateUniformBinningSpecificBins(data, currBin, amountOfBins);
		return histogramCalculation->getUniformBinningData();
	}
	else if (iACompVisOptions::binningType::JenksNaturalBreaks == binningType)
	{  // TODO: Implement
	}
	else if (iACompVisOptions::binningType::BayesianBlocks == binningType)
	{  // TODO: Implement
	}

	return nullptr;
}
	
void iACompHistogramTable::drawDatasetsInAscendingOrder()
{
	histogramVis->drawDatasetsInAscendingOrder();
}

void iACompHistogramTable::drawDatasetsInDescendingOrder()
{
	histogramVis->drawDatasetsInDescendingOrder();
}

void iACompHistogramTable::drawDatasetsInOriginalOrder()
{
	histogramVis->drawDatasetsInOriginalOrder();
}

/**************************  Change Table Visualization Methods  ******************************/
void iACompHistogramTable::drawUniformTable()
{
	histogramVis->drawUniformTable();
}

void iACompHistogramTable::drawBayesianBlocksTable()
{
	histogramVis->drawBayesianBlocksTable();
}

void iACompHistogramTable::drawNaturalBreaksTable()
{
	histogramVis->drawNaturalBreaksTable();
}

void iACompHistogramTable::drawCurveTable()
{
	histogramVis->drawCurveTable();
}

