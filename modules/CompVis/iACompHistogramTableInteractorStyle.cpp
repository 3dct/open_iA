#include "iACompHistogramTableInteractorStyle.h"

//Debug
#include "iAConsole.h"

//CompVis
#include "iACompHistogramTable.h"
#include "iACompVisMain.h"

//VTK
#include <vtkObjectFactory.h>
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

//testing
#include <vtkCoordinate.h>


vtkStandardNewMacro(iACompHistogramTableInteractorStyle);

iACompHistogramTableInteractorStyle::iACompHistogramTableInteractorStyle() :
	m_picked(new Pick::PickedMap()),
	m_controlBinsInZoomedRows(false),
	m_pointRepresentationOn(false),
	m_zoomLevel(1)
{
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

		vtkSmartPointer<vtkPropPicker> actorPicker = vtkSmartPointer<vtkPropPicker>::New();
		int is = actorPicker->Pick(pos[0], pos[1], 0, currentRenderer);

		if (is != 0)
		{
			vtkSmartPointer<vtkActor> pickedA = actorPicker->GetActor();

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
		m_visualization->clearZoomedRows();
		m_visualization->removePointRepresentation();
		m_visualization->removeHighlightedCells();

		m_controlBinsInZoomedRows = false;
		m_visualization->drawHistogramTable(m_visualization->getBins());
	}
}

void iACompHistogramTableInteractorStyle::OnMiddleButtonDown()
{
	// Forward events
	vtkInteractorStyleTrackballCamera::OnMiddleButtonDown();
}

void iACompHistogramTableInteractorStyle::OnRightButtonDown()
{
	DEBUG_LOG("Pressed right mouse button.");
	// Forward events
	//vtkInteractorStyleTrackballCamera::OnRightButtonDown();
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
	m_zoomedRowData = m_visualization->getSelectedData(m_picked);
	m_main->updateOtherCharts();
}