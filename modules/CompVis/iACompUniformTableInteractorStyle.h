#pragma once

//CompVis
#include "iACompTableInteractorStyle.h"
#include "iACompHistogramTableData.h"

//vtk
#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkIdTypeArray.h"

//C++
#include <map>
#include <vector>

//CompVis
class iACompUniformTable;

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

class iACompUniformTableInteractorStyle : public iACompTableInteractorStyle
{
   public:
	static iACompUniformTableInteractorStyle* New();
	   vtkTypeMacro(iACompUniformTableInteractorStyle, iACompTableInteractorStyle);

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
	void setUniformTableVisualization(iACompUniformTable* visualization);

	Pick::PickedMap* getPickedObjects();

	//remove unnecessary highlights and bar char visualization
	void reinitializeState();

   protected:
	iACompUniformTableInteractorStyle();

   private:	
	
	std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData);

	//reformats picked object such that the other charts can work with it
	csvDataType::ArrayType* formatPickedObjects(QList<std::vector<csvDataType::ArrayType*>*>* zoomedRowData);

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

	void resetUniformTable();

	void manualTableRelocatingStart(vtkSmartPointer<vtkActor> movingActor);
	void manualTableRelocatingStop();

	//set the picklist for the propPicker to only pick original row actors
	void setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors);

	void storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id);

	//reset the bar chart visualization showing the number of objects for each dataset
	bool resetBarChartAmountObjects();

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

	iACompUniformTable* m_visualization;

	QList<bin::BinType*>* m_zoomedRowData;

	vtkSmartPointer<vtkPropPicker> m_actorPicker;
	vtkSmartPointer<vtkActor> m_currentlyPickedActor;

	bool m_panActive;
};
