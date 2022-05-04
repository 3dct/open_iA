#include "iACompVariableTableInteractorStyle.h"
#include <vtkObjectFactory.h> //for macro!

//CompVis
#include "iACompVariableTable.h"
#include "iACompHistogramVis.h"

//vtk
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>

#include <vtkActor.h>
#include <vtkProperty.h>

vtkStandardNewMacro(iACompVariableTableInteractorStyle);

iACompVariableTableInteractorStyle::iACompVariableTableInteractorStyle() : 
	iACompTableInteractorStyle(), 
	m_visualization(nullptr), 
	m_picker(vtkSmartPointer<vtkCellPicker>::New())
{
}

void iACompVariableTableInteractorStyle::setVariableTableVisualization(iACompVariableTable* visualization)
{
	m_visualization = visualization;
}

void iACompVariableTableInteractorStyle::OnLeftButtonDown()
{
	//set pick list
	setPickList(m_visualization->getOriginalRowActors());

	// Get the location of the click (in window coordinates)
	int* pos = this->GetInteractor()->GetEventPosition();
	this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();
	if (currentRenderer == nullptr)
	{
		LOG(lvlDebug, "VariableTableInteractorStyle: currentRenderer is null!!");
		return;
	}

	int cellPicked = m_picker->Pick(pos[0], pos[1], 0, this->CurrentRenderer);

	if (cellPicked == 0)
	{
		resetHistogramTable();
		resetOtherCharts();
		m_picked->clear();
		return;
	}
	
	if (cellPicked != NULL && this->GetInteractor()->GetShiftKey() && m_picker->GetActor()!= NULL)
	{
		if (m_picked->empty())
		{ //when it is the first time of a new picking time --> remove the highlight from the last one
			m_visualization->removeHighlightedCells();
		}

		vtkIdType id = m_picker->GetCellId();
		storePickedActorAndCell(m_picker->GetActor(), id);
		m_visualization->highlightSelectedCell(m_picker->GetActor(), id);
	}

	m_visualization->renderWidget();
}

void iACompVariableTableInteractorStyle::OnMouseWheelForward()
{
	iACompTableInteractorStyle::OnMouseWheelForward();
	
	//camera zoom
	bool zoomed = generalZoomIn();
}

void iACompVariableTableInteractorStyle::OnMouseWheelBackward()
{
	iACompTableInteractorStyle::OnMouseWheelBackward();
	
	//camera zoom
	bool zoomed = generalZoomOut();
}

void iACompVariableTableInteractorStyle::OnKeyPress()
{
	iACompTableInteractorStyle::OnKeyPress();
}
void iACompVariableTableInteractorStyle::OnKeyRelease()
{
	iACompTableInteractorStyle::OnKeyRelease();

	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	std::string key = interactor->GetKeySym();

	//when shift is released, the computation of the zoom is performed
	if (key == "Shift_L")
	{
		if (m_picked->size() >= 1)
		{
			
			//forward update to all other charts & histogram table
			updateCharts();

			Pick::copyPickedMap(m_picked, m_pickedOld);

			//reset selection variables
			m_picked->clear();

			m_visualization->renderWidget();
			
		}
	}
}

void iACompVariableTableInteractorStyle::resetHistogramTable()
{
	m_visualization->removeHighlightedCells();
	removeBarChart();

	m_visualization->drawHistogramTable();
	m_visualization->renderWidget();
}

bool iACompVariableTableInteractorStyle::removeBarChart()
{
	if (m_visualization->getBarChartAmountObjectsActive())
	{
		m_visualization->removeBarCharShowingAmountOfObjects();
		return true;
	}

	return false;
}

void iACompVariableTableInteractorStyle::setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors)
{
	m_picker->InitializePickList();

	for (int i = 0; i < originalRowActors->size(); i++)
	{
		m_picker->AddPickList(originalRowActors->at(i));
	}
}

void iACompVariableTableInteractorStyle::updateCharts()
{
	QList<bin::BinType*>* zoomedRowDataMDS;
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes;

	std::tie(zoomedRowDataMDS, selectedObjectAttributes) = m_visualization->getSelectedData(m_picked);
	m_zoomedRowData = zoomedRowDataMDS;

	updateOtherCharts(selectedObjectAttributes);
}

void iACompVariableTableInteractorStyle::updateOtherCharts(
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes)
{
	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	csvDataType::ArrayType* selectedData = formatPickedObjects(selectedObjectAttributes);
	
	std::map<int, std::vector<double>>* pickStatistic = calculatePickedObjects(m_zoomedRowData);
	
	m_main->updateOtherCharts(selectedData, pickStatistic);
}

std::map<int, std::vector<double>>* iACompVariableTableInteractorStyle::calculatePickedObjects(
	QList<bin::BinType*>* zoomedRowData)
{
	std::map<int, std::vector<double>>* statisticForDatasets = new std::map<int, std::vector<double>>();

	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	//get number of all object in this dataset
	std::vector<int>* amountObjectsEveryDataset = m_visualization->getHistogramVis()->getAmountObjectsEveryDataset();

	calculateStatisticsForDatasets(zoomedRowData, indexOfPickedRows, amountObjectsEveryDataset, statisticForDatasets);

	return statisticForDatasets;
}

iACompTable* iACompVariableTableInteractorStyle::getVisualization()
{
	return m_visualization;
}