// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompTableInteractorStyle.h"

//Debug
#include "iALog.h"
#include "iACompHistogramVis.h"

//VTK
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkSmartPointer.h>
#include <vtkRenderWindowInteractor.h>


namespace Pick
{
	void copyPickedMap(PickedMap* input, PickedMap* result)
	{
		for (PickedMap::iterator it = input->begin(); it != input->end(); it++)
		{
			vtkSmartPointer<vtkActor> currAc = it->first;
			std::vector<vtkIdType>* currVec = it->second;

			result->insert({currAc, currVec});
		}
	};

	void debugPickedMap(PickedMap* input)
	{
		LOG(lvlDebug, "#######################################################");
		LOG(lvlDebug, "");
		LOG(lvlDebug, "size = " + QString::number(input->size()));

		std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>::iterator it;
		int count = 0;
		for (it = input->begin(); it != input->end(); it++)
		{
			vtkSmartPointer<vtkActor> currAc = it->first;
			std::vector<vtkIdType>* currVec = it->second;

			LOG(lvlDebug,
				"Actor " + QString::number(count) + " has " + QString::number(currVec->size()) + " picked cells");
		}
		LOG(lvlDebug, "");
		LOG(lvlDebug, "#######################################################");
	};
}

iACompTableInteractorStyle::iACompTableInteractorStyle() : 
	vtkInteractorStyleTrackballCamera(),
	m_main(nullptr), 
	m_zoomLevel(1), 
	m_zoomOn(true), 
	m_picked(new Pick::PickedMap()),
	m_pickedOld(new Pick::PickedMap()),
	m_zoomedRowData(nullptr),
	m_DButtonPressed(false)
{

}

void iACompTableInteractorStyle::setIACompHistogramVis(iACompHistogramVis* main)
{
	m_main = main;
}

void iACompTableInteractorStyle::OnEnter()
{
	//LOG(lvlInfo, "Please click on the background to activate the keyboard.");
}

void iACompTableInteractorStyle::OnLeftButtonDown()
{
}
void iACompTableInteractorStyle::OnLeftButtonUp()
{
}

void iACompTableInteractorStyle::OnMouseMove()
{
	vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void iACompTableInteractorStyle::OnMiddleButtonDown()
{
	vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
}

void iACompTableInteractorStyle::OnMiddleButtonUp()
{
	vtkInteractorStyleTrackballCamera::OnMiddleButtonUp();
}

void iACompTableInteractorStyle::OnRightButtonDown()
{
}

void iACompTableInteractorStyle::OnMouseWheelForward()
{
	onKeyDPressedMouseWheelForward();
}

void iACompTableInteractorStyle::changeDistributionVisualizationForward()
{
	if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::UniformTable)
	{
		m_main->drawCombiTable();

		m_main->deactivateOrderingButton();
	}
	else if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::VariableTable)
	{
		m_main->drawCombiTable();

		m_main->deactivateOrderingButton();
	}
	else if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::CombTable)
	{
		m_main->drawCurveTable();

		m_main->deactivateOrderingButton();
	}
	else if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::CurveVisualization)
	{
	}
	else
	{
	}
}

void iACompTableInteractorStyle::onKeyDPressedMouseWheelForward()
{
	if (m_DButtonPressed == true)
	{
		changeDistributionVisualizationForward();
	}
}

void iACompTableInteractorStyle::onKeyDPressedMouseWheelBackward()
{
	if (m_DButtonPressed == true)
	{
		changeDistributionVisualizationBackward();
	}
}

void iACompTableInteractorStyle::OnMouseWheelBackward()
{
	onKeyDPressedMouseWheelBackward();
}

void iACompTableInteractorStyle::changeDistributionVisualizationBackward()
{
	if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::UniformTable)
	{
		return;
	}
	else if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::VariableTable)
	{
		return;
	}
	else if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::CombTable)
	{
		if (m_main->getActiveBinning() == iACompVisOptions::binningType::Uniform ||
			m_main->getActiveBinning() == iACompVisOptions::binningType::Undefined)
		{
			m_main->drawUniformTable();

			m_main->activateOrderingButton();
		}
		else if (m_main->getActiveBinning() == iACompVisOptions::binningType::JenksNaturalBreaks)
		{
			m_main->drawNaturalBreaksTable();

			m_main->activateOrderingButton();
		}
		else if (m_main->getActiveBinning() == iACompVisOptions::binningType::BayesianBlocks)
		{
			m_main->drawBayesianBlocksTable();

			m_main->activateOrderingButton();
		}
	}
	else if (m_main->getActiveVisualization() == iACompVisOptions::activeVisualization::CurveVisualization)
	{
		m_main->drawCombiTable();

		m_main->deactivateOrderingButton();
	}
	else
	{
	}
}

void iACompTableInteractorStyle::OnKeyPress()
{
	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	if (interactor == nullptr)
	{
		return;
	}

	char* keyInChar = interactor->GetKeySym();
	if (keyInChar == nullptr)
	{
		return;
	}

	std::string key(keyInChar);

	//when d is pressed --> the visualization is switched to adaptive tile maps
	if (key == "d")
	{
		m_DButtonPressed = true;
	}
}
void iACompTableInteractorStyle::OnKeyRelease()
{
	vtkRenderWindowInteractor* interactor = this->GetInteractor();

	if (interactor == nullptr)
	{
		return;
	}

	char* keyInChar = interactor->GetKeySym();

	if (keyInChar == nullptr)
	{
		return;
	}

	std::string key(keyInChar);

	if (key == "d")
	{
		m_DButtonPressed = false;
	}
}

void iACompTableInteractorStyle::Pan()
{
	vtkInteractorStyleTrackballCamera::Pan();
}

bool iACompTableInteractorStyle::generalZoomIn()
{
	//camera zoom in
	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	if (interactor != nullptr)
	{
		if (interactor->GetControlKey())
		{
			m_main->getCamera()->Zoom(m_zoomLevel + 0.05);
			m_main->renderWidget();
			return true;
		}
		return false;
	}
	return false;
}

bool iACompTableInteractorStyle::generalZoomOut()
{
	//camera zoom in
	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	if (interactor != nullptr)
	{
		if (interactor->GetControlKey())
		{
			m_main->getCamera()->Zoom(m_zoomLevel - 0.05);
			m_main->renderWidget();
			return true;
		}
		return false;
	}
	return false;
}

void iACompTableInteractorStyle::storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id)
{
	if (m_picked->find(pickedA) != m_picked->end())
	{
		//when this actor has been picked already
		std::vector<vtkIdType>* v = m_picked->find(pickedA)->second;
		if (std::find(v->begin(), v->end(), id) == v->end())
		{  //when cellId is not already in the vector, add it
			v->push_back(id);
		}
	}
	else
	{
		//when this actor has NOT been picked until now
		std::vector<vtkIdType>* pickedCellsList = new std::vector<vtkIdType>();
		pickedCellsList->push_back(id);

		m_picked->insert({pickedA, pickedCellsList});
	}
}

void iACompTableInteractorStyle::resetOtherCharts()
{
	m_main->resetOtherCharts();
}

csvDataType::ArrayType* iACompTableInteractorStyle::formatPickedObjects(
	QList<std::vector<csvDataType::ArrayType*>*>* zoomedRowData)
{
	csvDataType::ArrayType* result = new csvDataType::ArrayType();

	/*LOG(lvlDebug,"++++++++++++++++++++++++");
	//DEBUG
	for (int i = 0; i < zoomedRowData->size(); i++)
	{ //datasets
		for (int k = 0; k < zoomedRowData->at(i)->size(); k++)
		{ //bins

			csvDataType::ArrayType* data = zoomedRowData->at(i)->at(k);

			for (int j = 0; j < data->size(); j++)
			{
				LOG(lvlDebug,"fiberLabelId = " + QString::number(data->at(j).at(0)) + " --> at Bin: " + QString::number(k));
			}

		}
	}
	LOG(lvlDebug,"++++++++++++++++++++++++");*/

	int amountDatasets = zoomedRowData->size();

	if (amountDatasets == 0 || (zoomedRowData->at(0)->size() == 0) || (zoomedRowData->at(0)->at(0)->size() == 0))
	{  //when selecting empty cell
		return result;
	}

	int amountAttributes = static_cast<int>(zoomedRowData->at(0)->at(0)->at(0).size());

	for (int attrInd = 0; attrInd < amountAttributes; attrInd++)
	{  //for all attributes
		std::vector<double> attr = std::vector<double>();

		for (int datasetInd = 0; datasetInd < amountDatasets; datasetInd++)
		{  //for the datasets that were picked
			int amountBins = static_cast<int>(zoomedRowData->at(datasetInd)->size());

			for (int binId = 0; binId < amountBins; binId++)
			{  //for the bins that were picked

				int amountVals = static_cast<int>(zoomedRowData->at(datasetInd)->at(binId)->size());

				for (int objInd = 0; objInd < amountVals; objInd++)
				{
					csvDataType::ArrayType* vals = zoomedRowData->at(datasetInd)->at(binId);
					attr.push_back(vals->at(objInd).at(attrInd));
				}
			}
		}

		result->push_back(attr);
	}

	//DEBUG
	/*for (int i = 0; i < result->size(); i++)
	{
		std::vector<double> attr = result->at(i);
		LOG(lvlDebug,"Attr " + QString::number(i) + " has " + QString::number(attr.size()) + " fibers");
	}*/

	return result;
}

std::map<int, std::vector<double>>* iACompTableInteractorStyle::calculateStatisticsForDatasets(
	QList<bin::BinType*>* zoomedRowData, std::vector<int>* indexOfPickedRows, std::vector<int>* amountObjectsEveryDataset,
	std::map<int, std::vector<double>>* result)
{
	for (int i = 0; i < ((int)zoomedRowData->size()); i++)
	{
		std::vector<double> container = std::vector<double>(2, 0);
		double totalNumber = 0;
		double pickedNumber = 0;

		//get total number of object of the picked dataset
		totalNumber = amountObjectsEveryDataset->at(indexOfPickedRows->at(i));

		//get number of picked objects
		bin::BinType* bins = zoomedRowData->at(i);
		for (int binInd = 0; binInd < ((int)bins->size()); binInd++)
		{  //sum over all bins to get amount of picked objects
			pickedNumber += bins->at(binInd).size();
		}

		container.at(0) = totalNumber;
		container.at(1) = pickedNumber;

		result->insert({indexOfPickedRows->at(i), container});
	}

	return result;
}
