#include "iACompHistogramTableInteractorStyle.h"
#include <vtkObjectFactory.h> //for macro!

//Debug
#include "iAConsole.h"

//CompVis
#include "iACompHistogramTable.h"
#include "iACompVisMain.h"

//VTK
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

#include <vtkActorCollection.h>
#include <vtkCellPicker.h>
#include <vtkPropPicker.h>
#include <vtkRendererCollection.h>

#include <vtkCellData.h>
#include <vtkProperty.h>

#include <vtkActor.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkLight.h>
#include <vtkMapper.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkUnsignedCharArray.h>

#include <vtkCallbackCommand.h>
#include <vtkTransform.h>

//Qt
#include <QDockWidget>
#include "QVTKOpenGLNativeWidget.h"

#include <tuple>


vtkStandardNewMacro(iACompHistogramTableInteractorStyle);

iACompHistogramTableInteractorStyle::iACompHistogramTableInteractorStyle() :
	m_picked(new Pick::PickedMap()),
	m_controlBinsInZoomedRows(false),
	m_pointRepresentationOn(false),
	m_zoomLevel(1),
	m_actorPicker(vtkSmartPointer<vtkPropPicker>::New()),
	m_zoomOn(true),
	m_currentlyPickedActor(vtkSmartPointer<vtkActor>::New()),
	m_panActive(false)
{
	m_actorPicker->SetPickFromList(true);
}

void iACompHistogramTableInteractorStyle::OnKeyPress(){}

void iACompHistogramTableInteractorStyle::OnKeyRelease()
{
	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	std::string key = interactor->GetKeySym();

	//when shift is released, the compuation of the zoom is performed
	if (key == "Shift_L")
	{
		if(m_zoomOn)
		{
			if (m_picked->size() >= 1)
			{
				m_visualization->removeHighlightedCells();

				//forward update to all other charts & histogram table
				updateCharts();

				//reset selection variables
				m_picked->clear();
				m_controlBinsInZoomedRows = true;
			}
		}
		else
		{
			//only one actor is allowed to be picked --> otherwise the calculation is not working
			if (m_picked->size() == 1)
			{
				m_visualization->drawHistogramTableAccordingToCellSimilarity(m_visualization->getBins(), m_picked);

				//reset selection variables
				m_picked->clear();
			}
		}
	}
}

void iACompHistogramTableInteractorStyle::OnLeftButtonDown()
{
	reinitializeState();

	//set pick list
	setPickList(m_visualization->getOriginalRowActors());

	// Get the location of the click (in window coordinates)
	int* pos = this->GetInteractor()->GetEventPosition();
	this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();
	if (currentRenderer == nullptr)
	{
		DEBUG_LOG("HistogramTableInteractorStyle: currentRenderer is null!!");
		return;
	}

	int is = m_actorPicker->Pick(pos[0], pos[1], 0, this->CurrentRenderer); //this->GetDefaultRenderer()
	if (is == 0) 
	{
		resetHistogramTable();
		m_picked->clear();
		return; 
	}
	
	vtkSmartPointer<vtkActor> pickedA = m_actorPicker->GetActor();

	this->GrabFocus(this->EventCallbackCommand);

	//select rows & bins which should be zoomed
	if (pickedA != NULL && this->GetInteractor()->GetShiftKey())
	{	
		vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
		cellPicker->Pick(pos[0], pos[1], 0, m_visualization->getRenderer());
		cellPicker->SetTolerance(0.0);

		vtkIdType id = cellPicker->GetCellId();
		storePickedActorAndCell(pickedA, id);
				
		//color selected bin
		m_visualization->highlightSelectedCell(pickedA, id);
		m_zoomOn = true;
		
	}
	else if(pickedA != NULL)
	{
		if(m_pointRepresentationOn || m_controlBinsInZoomedRows)
		{//when non-linear zoom is active --> do nothing
			//resetHistogramTable();
		}else
		{ //manual reordering only working when no non-linear zoom is active
			m_currentlyPickedActor = pickedA;
			manualTableRelocatingStart(m_currentlyPickedActor);

			//make actor nearest to user
			m_visualization->getRenderer()->RemoveActor(m_currentlyPickedActor);
			m_visualization->getRenderer()->AddActor(m_currentlyPickedActor);

			m_visualization->highlightSelectedRow(m_currentlyPickedActor);
		}
	}
}

void iACompHistogramTableInteractorStyle::OnLeftButtonUp()
{
	manualTableRelocatingStop();
}

void iACompHistogramTableInteractorStyle::OnMouseMove()
{
	int x = this->Interactor->GetEventPosition()[0];
	int y = this->Interactor->GetEventPosition()[1];

	switch (this->State)
	{
		case VTKIS_PAN:
			this->FindPokedRenderer(x, y);
			this->Pan();
			this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
			break;
	}
}

void iACompHistogramTableInteractorStyle::manualTableRelocatingStart(vtkSmartPointer<vtkActor> movingActor)
{
	m_visualization->calculateOldDrawingPositionOfMovingActor(movingActor);
	this->StartPan();
}

void iACompHistogramTableInteractorStyle::manualTableRelocatingStop()
{
	switch (this->State)
	{
		case VTKIS_PAN:
			this->EndPan();

			if(m_panActive)
			{
				m_visualization->drawReorderedHistogramTable();
				m_panActive = false;
			}
			
			break;
	}

	if (this->Interactor)
	{
		this->ReleaseFocus();
	}
}

void iACompHistogramTableInteractorStyle::Pan()
{
	m_panActive = true;

	//move picked actor
	vtkRenderWindowInteractor* rwi = this->Interactor;

	// Use initial center as the origin from which to pan
	double* obj_center = m_currentlyPickedActor->GetCenter();

	double disp_obj_center[3], new_pick_point[4];
	double old_pick_point[4], motion_vector[3];

	this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], disp_obj_center);

	this->ComputeDisplayToWorld(
		rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], disp_obj_center[2], new_pick_point);

	this->ComputeDisplayToWorld(rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1],
		disp_obj_center[2], old_pick_point);

	motion_vector[0] = new_pick_point[0] - old_pick_point[0];
	motion_vector[1] = new_pick_point[1] - old_pick_point[1];
	motion_vector[2] = new_pick_point[2] - old_pick_point[2];

	if (m_currentlyPickedActor->GetUserMatrix() != nullptr)
	{
		vtkTransform* t = vtkTransform::New();
		t->PostMultiply();
		t->SetMatrix(m_currentlyPickedActor->GetUserMatrix());
		t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
		m_currentlyPickedActor->GetUserMatrix()->DeepCopy(t->GetMatrix());
		t->Delete();
	}
	else
	{
		m_currentlyPickedActor->AddPosition(motion_vector[0], motion_vector[1], motion_vector[2]);
	}

	vtkSmartPointer<vtkActor> highlightActor = m_visualization->getHighlightingRowActor();
	if (highlightActor->GetUserMatrix() != nullptr)
	{
		vtkTransform* t = vtkTransform::New();
		t->PostMultiply();
		t->SetMatrix(m_currentlyPickedActor->GetUserMatrix());
		t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
		highlightActor->GetUserMatrix()->DeepCopy(t->GetMatrix());
		t->Delete();
	}
	else
	{
		highlightActor->AddPosition(motion_vector[0], motion_vector[1], motion_vector[2]);
	}

	if (this->AutoAdjustCameraClippingRange)
	{
		this->CurrentRenderer->ResetCameraClippingRange();
	}

	//reorder HistogramTable visualization accordingly
	m_visualization->reorderHistogramTable(m_currentlyPickedActor);

	m_visualization->renderWidget();
}

void iACompHistogramTableInteractorStyle::storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id)
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

		m_picked->insert({ pickedA, pickedCellsList });
	}
}

void iACompHistogramTableInteractorStyle::resetHistogramTable()
{
	//reset visualization when clicked anywhere
	m_visualization->setBinsZoomed(m_visualization->getMinBins());
	m_visualization->removePointRepresentation();
	m_visualization->removeHighlightedCells();
	m_visualization->removeHighlightedRow();
	resetBarChartAmountObjects();

	m_controlBinsInZoomedRows = false;
	m_pointRepresentationOn = false;
	m_visualization->drawHistogramTable(m_visualization->getBins());

	resetOtherCharts();
}

void iACompHistogramTableInteractorStyle::OnMiddleButtonDown()
{
	reinitializeState();

	// Forward events
	vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
}

void iACompHistogramTableInteractorStyle::OnRightButtonDown()
{
	reinitializeState();

	setPickList(m_visualization->getOriginalRowActors());

	int* pos = this->GetInteractor()->GetEventPosition();
	this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();
	if (currentRenderer == nullptr)
	{
		return;
	}

	if (((pos[0] >= currentRenderer->GetSize()[0]) || (pos[1] >= currentRenderer->GetSize()[1])))
	{
		return;
	}

	//int is = m_actorPicker->Pick(pos[0], pos[1], 0, currentRenderer)
	int is = m_actorPicker->Pick(pos[0], pos[1], 0, m_visualization->getRenderer());

	if (is == 0)
	{
		return;
	}

	vtkSmartPointer<vtkActor> pickedA = m_actorPicker->GetActor();
	if (pickedA == NULL)
	{
		return;
	}

	if (this->GetInteractor()->GetShiftKey())
	{ //order data only according to selected bins when shift is released

		vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
		cellPicker->Pick(pos[0], pos[1], 0, currentRenderer);
		cellPicker->SetTolerance(0.0);

		vtkIdType id = cellPicker->GetCellId();
		storePickedActorAndCell(pickedA, id);

		//color selected bin
		m_visualization->highlightSelectedCell(pickedA, id);
		m_zoomOn = false;
	}
	else
	{	//order datasets according to every bin
		m_visualization->highlightSelectedRow(pickedA);
		m_visualization->drawHistogramTableAccordingToSimilarity(m_visualization->getBins(), pickedA);
	}
}

void iACompHistogramTableInteractorStyle::OnMouseWheelForward()
{
	reinitializeState();

	//camera zoom in
	if (this->GetInteractor()->GetShiftKey() && this->GetInteractor()->GetControlKey())
	{  
		generalZoomIn();
		return;
	}

	//histogram zoom in
	if (m_controlBinsInZoomedRows)
	{
		//non linear zooming in
		nonLinearZoomIn();
	}
	else
	{
		//linear zooming in
		linearZoomInHistogram();
	}
}

void iACompHistogramTableInteractorStyle::OnMouseWheelBackward()
{
	reinitializeState();

	//camera zoom out
	if (this->GetInteractor()->GetShiftKey() && this->GetInteractor()->GetControlKey())
	{  
		generalZoomOut();
		return;
	}

	//histogram zoom out
	if (m_controlBinsInZoomedRows)
	{
		//non linear zooming out
		nonLinearZoomOut();
	}
	else
	{
		//linear zooming out
		linearZoomOutHistogram();
	}
}

void iACompHistogramTableInteractorStyle::linearZoomInHistogram()
{
	//linear zooming in on histogram over all bins
	int bins = m_visualization->getBins();
	if (bins >= m_visualization->getMinBins() && bins < m_visualization->getMaxBins())
	{
		bins = bins * 2;

		m_visualization->setBins(bins);
		m_visualization->drawHistogramTable(bins);
	}
}
void iACompHistogramTableInteractorStyle::linearZoomOutHistogram()
{
	//linear zooming out on histogram over all bins
	int bins = m_visualization->getBins();
	if (bins > m_visualization->getMinBins() && bins <= m_visualization->getMaxBins())
	{
		bins = bins / 2;

		m_visualization->setBins(bins);
		m_visualization->drawHistogramTable(bins);
	}
}

void iACompHistogramTableInteractorStyle::nonLinearZoomIn()
{
	int bins = m_visualization->getBinsZoomed();
	if (bins >= m_visualization->getMinBins() && bins < m_visualization->getMaxBins())
	{
		bins = bins * 2;
		m_visualization->setBinsZoomed(bins);
		m_visualization->redrawZoomedRow(bins);
	}
	else if (bins == m_visualization->getMaxBins())
	{  //draw point representation
		m_visualization->drawPointRepresentation();
		m_pointRepresentationOn = true;
	}
}
void iACompHistogramTableInteractorStyle::nonLinearZoomOut()
{
	//linear zooming out in histogram
	int bins = m_visualization->getBinsZoomed();

	if (m_pointRepresentationOn)
	{
		m_pointRepresentationOn = false;
		m_visualization->removePointRepresentation();

		m_visualization->setBinsZoomed(bins);
		m_visualization->redrawZoomedRow(bins);
	}
	else if (bins > m_visualization->getMinBins() && bins <= m_visualization->getMaxBins())
	{
		bins = bins / 2;

		m_visualization->setBinsZoomed(bins);
		m_visualization->redrawZoomedRow(bins);
	}
}

void iACompHistogramTableInteractorStyle::generalZoomIn()
{
	m_visualization->getRenderer()->GetActiveCamera()->Zoom(m_zoomLevel + 0.05);
	m_visualization->renderWidget();
}
void iACompHistogramTableInteractorStyle::generalZoomOut()
{
	m_visualization->getRenderer()->GetActiveCamera()->Zoom(m_zoomLevel - 0.05);
	m_visualization->renderWidget();
}

void iACompHistogramTableInteractorStyle::setIACompHistogramTable(iACompHistogramTable* visualization)
{
	m_visualization = visualization;
}

void iACompHistogramTableInteractorStyle::setIACompVisMain(iACompVisMain* main)
{
	m_main = main;
}

void iACompHistogramTableInteractorStyle::updateCharts()
{
	QList<bin::BinType*>* zoomedRowDataMDS;
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes;

	//calculate the fiberIds per selected cells & the mds values per selected cells
	std::tie(zoomedRowDataMDS, selectedObjectAttributes) = m_visualization->getSelectedData(m_picked);
	m_zoomedRowData = zoomedRowDataMDS;

	//change histogram table
	m_visualization->drawLinearZoom(m_picked, m_visualization->getBins(), m_visualization->getBinsZoomed(), m_zoomedRowData);

	updateOtherCharts(selectedObjectAttributes);
}

void iACompHistogramTableInteractorStyle::updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes)
{
	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	csvDataType::ArrayType* selectedData = formatPickedObjects(selectedObjectAttributes);

	std::map<int, std::vector<double>>* pickStatistic = calculatePickedObjects(m_zoomedRowData);

	m_main->updateOtherCharts(selectedData, pickStatistic);
}

std::map<int, std::vector<double>>* iACompHistogramTableInteractorStyle::calculatePickedObjects(QList<bin::BinType*>* zoomedRowData)
{
	std::map<int, std::vector<double>>* statisticForDatasets = new std::map<int, std::vector<double>>();

	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	//get number of all object in this dataset
	std::vector<int>* amountObjectsEveryDataset = m_visualization->getAmountObjectsEveryDataset();

	for(int i = 0; i < zoomedRowData->size(); i++)
	{
		std::vector<double> container = std::vector<double>(2, 0);
		double totalNumber = 0;
		double pickedNumber = 0;

		//get total number of object of the picked dataset
		totalNumber = amountObjectsEveryDataset->at(indexOfPickedRows->at(i));

		//get number of picked objects
		bin::BinType* bins = zoomedRowData->at(i);
		for (int binInd = 0; binInd < bins->size(); binInd++)
		{ //sum over all bins to get amount of picked objects
			pickedNumber += bins->at(binInd).size();
		}

		container.at(0) = totalNumber;
		container.at(1) = pickedNumber;

		statisticForDatasets->insert({ indexOfPickedRows->at(i), container});
	}

	return statisticForDatasets;
}

void iACompHistogramTableInteractorStyle::resetOtherCharts()
{	
	m_main->resetOtherCharts();
}

void iACompHistogramTableInteractorStyle::setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors)
{
	m_actorPicker->InitializePickList();

	for (int i = 0; i < originalRowActors->size(); i++)
	{
		m_actorPicker->AddPickList(originalRowActors->at(i));
	}

	//m_actorPicker->SetPickFromList(true);
}

csvDataType::ArrayType* iACompHistogramTableInteractorStyle::formatPickedObjects(QList<std::vector<csvDataType::ArrayType*>*>* zoomedRowData)
{
	csvDataType::ArrayType* result = new csvDataType::ArrayType();

	/*DEBUG_LOG("++++++++++++++++++++++++");
	//DEBUG
	for (int i = 0; i < zoomedRowData->size(); i++)
	{ //datasets
		for (int k = 0; k < zoomedRowData->at(i)->size(); k++)
		{ //bins

			csvDataType::ArrayType* data = zoomedRowData->at(i)->at(k);

			for (int j = 0; j < data->size(); j++)
			{
				DEBUG_LOG("fiberLabelId = " + QString::number(data->at(j).at(0)) + " --> at Bin: " + QString::number(k));
			}

		}
	}
	DEBUG_LOG("++++++++++++++++++++++++");*/

	int amountDatasets = zoomedRowData->size();

	if(amountDatasets == 0 || (zoomedRowData->at(0)->size() == 0) || (zoomedRowData->at(0)->at(0)->size() == 0))
	{ //when selecting empty cell
		return result;
	}

	int amountAttributes = zoomedRowData->at(0)->at(0)->at(0).size();

	for (int attrInd = 0; attrInd < amountAttributes; attrInd++)
	{ //for all attributes
		std::vector<double> attr = std::vector<double>();

		for (int datasetInd = 0; datasetInd < amountDatasets; datasetInd++)
		{ //for the datasets that were picked
			int amountBins = zoomedRowData->at(datasetInd)->size();

			for (int binId = 0; binId < amountBins; binId++)
			{ //for the bins that were picked

				int amountVals = zoomedRowData->at(datasetInd)->at(binId)->size();
				
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
		DEBUG_LOG("Attr " + QString::number(i) + " has " + QString::number(attr.size()) + " fibers");
	}*/

	return result;
}

bool iACompHistogramTableInteractorStyle::resetBarChartAmountObjects()
{
	if(m_visualization->getBarChartAmountObjectsActive())
	{
		m_visualization->removeBarCharShowingAmountOfObjects();
		m_visualization->drawHistogramTable(m_visualization->getBins());
		return true;
	}

	return false;
}

void iACompHistogramTableInteractorStyle::reinitializeState()
{
	bool resetVis = resetBarChartAmountObjects();
	bool resetHigh = m_visualization->removeHighlightedRow();

	if(resetVis || resetHigh)
	{
		m_visualization->renderWidget();
	}
}