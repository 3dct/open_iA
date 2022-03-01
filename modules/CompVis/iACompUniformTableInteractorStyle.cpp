#include "iACompUniformTableInteractorStyle.h"
#include <vtkObjectFactory.h> //for macro!

#include "iACompUniformTable.h"
#include "iACompHistogramVis.h"

//VTK
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

#include <tuple>


vtkStandardNewMacro(iACompUniformTableInteractorStyle);

iACompUniformTableInteractorStyle::iACompUniformTableInteractorStyle() :
	iACompTableInteractorStyle(),
	m_visualization(nullptr),
	//m_zoomedRowData(nullptr),
	m_actorPicker(vtkSmartPointer<vtkPropPicker>::New()),
	m_currentlyPickedActor(vtkSmartPointer<vtkActor>::New()),
	m_panActive(false),
	m_controlBinsInZoomedRows(false),
	m_pointRepresentationOn(false),
	m_panCamera(false)
{
	m_actorPicker->SetPickFromList(true);
}

void iACompUniformTableInteractorStyle::OnKeyPress()
{
	iACompTableInteractorStyle::OnKeyPress();
}

void iACompUniformTableInteractorStyle::OnKeyRelease()
{
	iACompTableInteractorStyle::OnKeyRelease();

	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	std::string key = interactor->GetKeySym();

	//when shift is released, the computation of the zoom is performed
	if (key == "Shift_L")
	{
		if(m_zoomOn)
		{
			if (m_picked->size() >= 1)
			{
				m_visualization->removeHighlightedCells();

				//forward update to all other charts & histogram table
				updateCharts();

				Pick::copyPickedMap(m_picked, m_pickedOld);

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
				m_visualization->drawHistogramTableAccordingToCellSimilarity(m_picked);

				Pick::copyPickedMap(m_picked, m_pickedOld);
				//reset selection variables
				m_picked->clear();
			}
		}
	}
}

void iACompUniformTableInteractorStyle::OnLeftButtonDown()
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
		LOG(lvlDebug,"UniformTableInteractorStyle: currentRenderer is null!!");
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
			
		}else
		{ //manual reordering only working when NO non-linear zoom is active
			m_currentlyPickedActor = pickedA;
			manualTableRelocatingStart(m_currentlyPickedActor);

			//make actor nearest to user
			m_visualization->getRenderer()->RemoveActor(m_currentlyPickedActor);
			m_visualization->getRenderer()->AddActor(m_currentlyPickedActor);

			m_visualization->highlightSelectedRow(m_currentlyPickedActor);
		}
	}
}

void iACompUniformTableInteractorStyle::OnLeftButtonUp()
{
	manualTableRelocatingStop();
}

void iACompUniformTableInteractorStyle::OnMouseMove()
{
	if (m_panCamera)
	{  //pan the camera so the whole table is better visible
		vtkInteractorStyleTrackballCamera::OnMouseMove();
		return;
	}
	
	//move row of histogram table to new position
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

void iACompUniformTableInteractorStyle::manualTableRelocatingStart(vtkSmartPointer<vtkActor> movingActor)
{
	m_visualization->calculateOldDrawingPositionOfMovingActor(movingActor);
	this->StartPan();
}

void iACompUniformTableInteractorStyle::manualTableRelocatingStop()
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

void iACompUniformTableInteractorStyle::Pan()
{
	if (m_panCamera)
	{ //pan the camera so the whole table is better visible
		vtkInteractorStyleTrackballCamera::Pan();
		return;
	}
	
	
	//move row of histogram table to new position
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

void iACompUniformTableInteractorStyle::resetHistogramTable()
{
	//reset visualization when clicked anywhere
	m_visualization->setBinsZoomed(m_visualization->getMinBins());
	m_visualization->removePointRepresentation();
	m_visualization->removeHighlightedCells();
	m_visualization->removeHighlightedRow();
	removeBarChart();

	m_controlBinsInZoomedRows = false;
	m_pointRepresentationOn = false;
	
	m_visualization->clearRenderer();
	m_visualization->drawHistogramTable(m_visualization->getBins());

	//m_visualization->getRenderer()->ResetCamera();
	m_visualization->renderWidget();

	resetOtherCharts();
}

void iACompUniformTableInteractorStyle::OnMiddleButtonDown()
{
	reinitializeState();

	m_panCamera = true;

	// Forward events
	iACompTableInteractorStyle::OnMiddleButtonDown();
}

void iACompUniformTableInteractorStyle::OnMiddleButtonUp()
{
	m_panCamera = false;

	vtkInteractorStyleTrackballCamera::OnMiddleButtonUp();
}

void iACompUniformTableInteractorStyle::OnRightButtonDown()
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
		m_visualization->drawHistogramTableAccordingToSimilarity(pickedA);
	}
}

void iACompUniformTableInteractorStyle::OnMouseWheelForward()
{
	reinitializeState();

	iACompTableInteractorStyle::OnMouseWheelForward();

	//camera zoom
	bool zoomed = generalZoomIn();
	
	if (m_DButtonPressed == false && !zoomed)
	{
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
}

void iACompUniformTableInteractorStyle::OnMouseWheelBackward()
{
	reinitializeState();

	iACompTableInteractorStyle::OnMouseWheelBackward();

	//camera zoom
	bool zoomed = generalZoomOut();

	if (m_DButtonPressed == false && !zoomed)
	{
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
}

void iACompUniformTableInteractorStyle::linearZoomInHistogram()
{
	//linear zooming in on histogram over all bins
	int bins = m_visualization->getBins();
	if (bins >= m_visualization->getMinBins() && bins < m_visualization->getMaxBins())
	{
		bins = bins * 2;

		m_visualization->setBins(bins);
		m_visualization->recomputeBinning();
		m_visualization->drawHistogramTable(bins);
	}
}
void iACompUniformTableInteractorStyle::linearZoomOutHistogram()
{
	//linear zooming out on histogram over all bins
	int bins = m_visualization->getBins();
	if (bins > m_visualization->getMinBins() && bins <= m_visualization->getMaxBins())
	{
		bins = bins / 2;

		m_visualization->setBins(bins);
		m_visualization->recomputeBinning();
		m_visualization->drawHistogramTable(bins);
	}
}

void iACompUniformTableInteractorStyle::nonLinearZoomIn()
{
	int bins = m_visualization->getBinsZoomed();
	if (bins >= m_visualization->getMinBins() && bins < m_visualization->getMaxBins())
	{
		bins = bins * 2;
		m_visualization->setBinsZoomed(bins);
		m_visualization->zoomInZoomedRow(bins);
	}
	else if (bins == m_visualization->getMaxBins())
	{  //draw point representation
		m_visualization->drawPointRepresentation();
		m_pointRepresentationOn = true;
	}
}
void iACompUniformTableInteractorStyle::nonLinearZoomOut()
{
	//linear zooming out in histogram
	int bins = m_visualization->getBinsZoomed();

	if (m_pointRepresentationOn)
	{
		m_pointRepresentationOn = false;
		m_visualization->removePointRepresentation();

		m_visualization->setBinsZoomed(bins);
		m_visualization->zoomInZoomedRow(bins);
	}
	else if (bins > m_visualization->getMinBins() && bins <= m_visualization->getMaxBins())
	{
		bins = bins / 2;

		m_visualization->setBinsZoomed(bins);
		m_visualization->zoomInZoomedRow(bins);
	}
}

void iACompUniformTableInteractorStyle::setUniformTableVisualization(iACompUniformTable* visualization)
{
	m_visualization = visualization;
}

void iACompUniformTableInteractorStyle::updateCharts()
{
	QList<bin::BinType*>* zoomedRowDataMDS;
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes;

	//calculate the fiberIds per selected cells & the mds values per selected cells
	std::tie(zoomedRowDataMDS, selectedObjectAttributes) = m_visualization->getSelectedData(m_picked);
	m_zoomedRowData = zoomedRowDataMDS;

	//change histogram table
	m_visualization->drawNonLinearZoom(
		m_picked, m_visualization->getBins(), m_visualization->getBinsZoomed(), m_zoomedRowData);

	updateOtherCharts(selectedObjectAttributes);
}

void iACompUniformTableInteractorStyle::updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes)
{
	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	csvDataType::ArrayType* selectedData = formatPickedObjects(selectedObjectAttributes);

	std::map<int, std::vector<double>>* pickStatistic = calculatePickedObjects(m_zoomedRowData);

	m_main->updateOtherCharts(selectedData, pickStatistic);
}

std::map<int, std::vector<double>>* iACompUniformTableInteractorStyle::calculatePickedObjects(QList<bin::BinType*>* zoomedRowData)
{
	std::map<int, std::vector<double>>* statisticForDatasets = new std::map<int, std::vector<double>>();

	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	//get number of all object in this dataset
	std::vector<int>* amountObjectsEveryDataset = m_visualization->getHistogramVis()->getAmountObjectsEveryDataset();

	calculateStatisticsForDatasets(zoomedRowData, indexOfPickedRows, amountObjectsEveryDataset, statisticForDatasets);

	return statisticForDatasets;
}

void iACompUniformTableInteractorStyle::setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors)
{
	m_actorPicker->InitializePickList();

	for (int i = 0; i < ((int)originalRowActors->size()); i++)
	{
		m_actorPicker->AddPickList(originalRowActors->at(i));
	}
}

bool iACompUniformTableInteractorStyle::removeBarChart()
{
	if(m_visualization->getBarChartAmountObjectsActive())
	{
		m_visualization->removeBarCharShowingAmountOfObjects();
		m_visualization->drawHistogramTable(m_visualization->getBins());
		return true;
	}

	return false;
}

void iACompUniformTableInteractorStyle::reinitializeState()
{
	bool resetVis = removeBarChart();
	bool resetHigh = m_visualization->removeHighlightedRow();

	if(resetVis || resetHigh)
	{
		m_visualization->renderWidget();
	}
}

Pick::PickedMap* iACompUniformTableInteractorStyle::getPickedObjects()
{
	return m_pickedOld;
}

iACompTable* iACompUniformTableInteractorStyle::getVisualization()
{
	return m_visualization;
}
