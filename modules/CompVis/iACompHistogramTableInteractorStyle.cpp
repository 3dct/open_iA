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

//testing

#include <vtkCellData.h>
#include <vtkProperty.h>

#include <vtkActor.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkLight.h>
#include <vtkMapper.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkUnsignedCharArray.h>

//Qt
#include <QDockWidget>


#include <tuple>


vtkStandardNewMacro(iACompHistogramTableInteractorStyle);

iACompHistogramTableInteractorStyle::iACompHistogramTableInteractorStyle() :
	m_picked(new Pick::PickedMap()),
	m_controlBinsInZoomedRows(false),
	m_pointRepresentationOn(false),
	m_zoomLevel(1),
	m_actorPicker(vtkSmartPointer<vtkPropPicker>::New())
{
	m_actorPicker->SetPickFromList(true);
}

void iACompHistogramTableInteractorStyle::OnKeyPress()
{
	vtkRenderWindowInteractor* interactor = this->GetInteractor();

	// Get the shift press
	if (interactor->GetShiftKey())
	{
	}
}

void iACompHistogramTableInteractorStyle::OnKeyRelease()
{
	vtkRenderWindowInteractor* interactor = this->GetInteractor();
	std::string key = interactor->GetKeySym();

	//when shift is released, the compuation of the zoom is performed
	if (key == "Shift_L")
	{
		if (m_picked->size() >= 1)
		{
			m_visualization->removeHighlightedCells();

			//forward update to all other charts
			updateOtherCharts();
			
			//change in histogram table
			m_visualization->drawLinearZoom(m_picked, m_visualization->getBins(), m_visualization->getBinsZoomed(), m_zoomedRowData);

			//reset selection variables
			m_picked->clear();
			m_controlBinsInZoomedRows = true;
		}
	}
}

void iACompHistogramTableInteractorStyle::OnLeftButtonDown()
{
	//select rows & bins which should be zoomed
	if (this->GetInteractor()->GetShiftKey())
	{
		//set pick list
		setPickList(m_visualization->getOriginalRowActors());

		// Get the location of the click (in window coordinates)
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

		
		int is = m_actorPicker->Pick(pos[0], pos[1], 0, currentRenderer);

		if (is != 0)
		{
			vtkSmartPointer<vtkActor> pickedA = m_actorPicker->GetActor();

			if (pickedA != NULL)
			{
				vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
				cellPicker->Pick(pos[0], pos[1], 0, currentRenderer);
				cellPicker->SetTolerance(0.0);

				vtkIdType id = cellPicker->GetCellId();

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

				//color selected bin
				m_visualization->highlightSelectedCell(pickedA, id);
			}
		}
	}
	else
	{
		//reset visualization when clicked anywhere
		m_picked->clear();
		m_visualization->setBinsZoomed(m_visualization->getMinBins());
		m_visualization->removePointRepresentation();
		m_visualization->removeHighlightedCells();

		m_controlBinsInZoomedRows = false;
		m_visualization->drawHistogramTable(m_visualization->getBins());

		resetOtherCharts();
	}
}

void iACompHistogramTableInteractorStyle::OnMiddleButtonDown()
{
	// Forward events
	vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
}

void iACompHistogramTableInteractorStyle::OnRightButtonDown()
{
	
	setPickList(m_visualization->getOriginalRowActors());

	int* pos = this->GetInteractor()->GetEventPosition();
	/*this->FindPokedRenderer(pos[0], pos[1]);
	auto currentRenderer = this->GetDefaultRenderer();
	if (currentRenderer == nullptr)
	{
		return;
	}
	
	DEBUG_LOG("Here");

	if (((pos[0] >= currentRenderer->GetSize()[0]) || (pos[1] >= currentRenderer->GetSize()[1])))
	{
		return;
	}
*/

	//int is = m_actorPicker->Pick(pos[0], pos[1], 0, currentRenderer)
	int is = m_actorPicker->Pick(pos[0], pos[1], 0, m_visualization->getRenderer());

	if (is != 0)
	{
		vtkSmartPointer<vtkActor> pickedA = m_actorPicker->GetActor();
		m_visualization->highlightSelectedRow(pickedA);

		m_visualization->drawHistogramTableAccordingToSimilarity(m_visualization->getBins(), pickedA);
	}
}

void iACompHistogramTableInteractorStyle::OnMouseWheelForward()
{
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

void iACompHistogramTableInteractorStyle::updateOtherCharts()
{
	QList<bin::BinType*>* zoomedRowDataMDS;
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes;
	
	//calculate the fiberIds per selected cells & the mds values per selected cells
	std::tie(zoomedRowDataMDS, selectedObjectAttributes) = m_visualization->getSelectedData(m_picked);
	m_zoomedRowData = zoomedRowDataMDS;

	std::vector<int>* indexOfPickedRows = m_visualization->getIndexOfPickedRows();
	csvDataType::ArrayType* selectedData = formatPickedObjects(selectedObjectAttributes);

	std::map<int, std::vector<double>>* pickStatistic = calculatePickedObjects(zoomedRowDataMDS);

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

	//TODO recalculate attribute array with correct values!!!
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
			//DEBUG_LOG("amountBins = " + QString::number(amountBins));

			for (int binId = 0; binId < amountBins; binId++)
			{ //for the bins that were picked

				int amountVals = zoomedRowData->at(datasetInd)->at(binId)->size();
				//DEBUG_LOG("Fibers = " + QString::number(amountVals));

				for (int objInd = 0; objInd < amountVals; objInd++)
				{
					csvDataType::ArrayType* vals = zoomedRowData->at(datasetInd)->at(binId);
					attr.push_back(vals->at(objInd).at(attrInd));

					//DEBUG_LOG("Attribute " + QString::number(attrInd) + " with " + QString::number(vals->at(objInd).at(attrInd)) + " values");
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