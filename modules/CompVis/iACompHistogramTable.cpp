// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompHistogramTable.h"

//Debug
#include "iALog.h"

//CompVis
#include "iACompVisOptions.h"
#include "iACompVisMain.h"
#include "iACompHistogramTableData.h"

#include "iACsvDataStorage.h"
#include "iACompHistogramTableData.h"
#include "iACompHistogramVis.h"
#include "iACompUniformBinningData.h"

//iA
#include <iAMainWindow.h>
#include <iAQVTKWidget.h>

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
	m_dataStorage(dataStorage),
	m_inputData(dataStorage->getData())
{
	std::vector<int>* dataResolution = csvFileData::getAmountObjectsEveryDataset(m_inputData);
	m_amountDatasets = (int)dataResolution->size();

	//initialize datastructure
	initializeBinCalculation(MDSComputedFlag);

	//initialize visualization
	histogramVis = new iACompHistogramVis(this, parent, m_amountDatasets, MDSComputedFlag);
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

void iACompHistogramTable::reinitializeHistogramTable()
{
	//reinitialize datastructure
	initializeBinCalculation(true);

	//reinitalize visualization
	histogramVis->reinitialize();
	drawUniformTable();
}

std::vector<int>* iACompHistogramTable::getAmountObjectsEveryDataset()
{
	return csvFileData::getAmountObjectsEveryDataset(m_dataStorage->getData());
}

iACsvDataStorage* iACompHistogramTable::getDataStorage()
{
	return m_dataStorage;
}

void iACompHistogramTable::setDataStorage(iACsvDataStorage* storage)
{
	m_dataStorage = storage;
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

iACompKernelDensityEstimationData* iACompHistogramTable::recomputeKernelDensityCurveUB()
{
	histogramCalculation->recalculateDensityEstimationUniformBinning();
	return histogramCalculation->getKernelDensityEstimationData();
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

void iACompHistogramTable::deactivateOrderingButton()
{
	m_main->deactivateOrderingButton();
}

void iACompHistogramTable::activateOrderingButton()
{
	m_main->activateOrderingButton();
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

void iACompHistogramTable::drawWhiteCurveTable()
{
	histogramVis->drawWhiteCurveTable();
}
