#pragma once

//CompVis
#include "iACompHistogramTableData.h"

//vtk
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkIdTypeArray.h"

//C++
#include <map>
#include <vector>

//CompVis
class iACompUniformTable;
class iACompHistogramVis;

//vtk
class vtkRenderer;
class vtkPropPicker;

namespace Pick
{
using PickedMap = std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>;

static void copyPickedMap(PickedMap* input, PickedMap* result)
{
	std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>::iterator it;

	for (it = input->begin(); it != input->end(); it++)
	{
		vtkSmartPointer<vtkActor> currAc = it->first;
		std::vector<vtkIdType>* currVec = it->second;

		result->insert({currAc, currVec});
	}
};

static void debugPickedMap(PickedMap* input)
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

		LOG(lvlDebug, "Actor " + QString::number(count) + " has " + QString::number(currVec->size()) + " picked cells");
	}
	LOG(lvlDebug, "");
	LOG(lvlDebug, "#######################################################");
};

}

class iACompUniformTableInteractorStyle : public vtkInteractorStyleTrackballCamera
{
   public:
	static iACompUniformTableInteractorStyle* New();
	vtkTypeMacro(iACompUniformTableInteractorStyle, vtkInteractorStyleTrackballCamera);

	virtual void OnLeftButtonDown();
	virtual void OnLeftButtonUp();

	virtual void OnMouseMove();

	virtual void OnMiddleButtonDown();
	virtual void OnRightButtonDown();
	virtual void OnMouseWheelForward();
	virtual void OnMouseWheelBackward();
	virtual void OnKeyPress();
	virtual void OnKeyRelease();

	virtual void Pan();

	//init iACompHistogramTable
	void setIACompUniformTable(iACompUniformTable* visualization);
	void setIACompHistogramVis(iACompHistogramVis* main);

	Pick::PickedMap* getPickedObjects();

   protected:
	iACompUniformTableInteractorStyle();

   private:	
	
	std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData);

	//reformats picked object such that the other charts can work with it
	csvDataType::ArrayType* formatPickedObjects(QList<std::vector<csvDataType::ArrayType*>*>* zoomedRowData);

	//general zooming in executed by the camera
	void generalZoomIn();
	//general zooming out executed by the camera
	void generalZoomOut();

	//linear zooming in over all bins of the histograms
	void linearZoomInHistogram();
	//linear zooming out over all bins of the histograms
	void linearZoomOutHistogram();

	//non linear zooming in - zooming in on currently selected bin(s)/row(s)
	void nonLinearZoomIn();
	//non linear zooming out - zooming out on currently selected bin(s)/row(s)
	void nonLinearZoomOut();

	void updateCharts();
	void updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes);
	void resetOtherCharts();

	void resetHistogramTable();

	//remove unnecessary highlights and bar char visualization 
	void reinitializeState();

	void manualTableRelocatingStart(vtkSmartPointer<vtkActor> movingActor);
	void manualTableRelocatingStop();

	//set the picklist for the propPicker to only pick original row actors
	void setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors);

	void storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id);

	//reset the bar chart visualization showing the number of objects for each dataset
	bool resetBarChartAmountObjects();

	iACompUniformTable* m_visualization;
	iACompHistogramVis* m_main;

	//stores for each actor a vector for each picked cell according to their vtkIdType
	Pick::PickedMap* m_picked;
	//store for reinitialization after minimization etc. of application
	Pick::PickedMap* m_pickedOld;

	//controls whether the linear or non-linear zoom on the histogram is activated
	//true --> non-linear zoom is activated, otherwise linear zoom
	bool m_controlBinsInZoomedRows;
	//controls if the number of bins is modified or if the point representation should be drawn
	//true --> point representation is drawn
	bool m_pointRepresentationOn;

	//if ture --> zooming is active, otherwise ordering according to selected bins is active
	bool m_zoomOn;

	//is always 1 -> only needed for changing the zoom for the camera
	double m_zoomLevel;

	QList<bin::BinType*>* m_zoomedRowData;

	vtkSmartPointer<vtkPropPicker> m_actorPicker;
	vtkSmartPointer<vtkActor> m_currentlyPickedActor;

	bool m_panActive;
};
