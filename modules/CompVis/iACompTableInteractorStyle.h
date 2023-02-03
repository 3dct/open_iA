#pragma once
//vtk
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkIdTypeArray.h"
#include "iACsvDataStorage.h"
#include "iACompHistogramTableData.h"

//Debug
#include "iALog.h"

//C++
#include <map>
#include <vector>

//Qt
#include <QString>

//CompVis
class iACompHistogramVis;
class iACompTable;

namespace Pick
{
	using PickedMap = std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>;

	void copyPickedMap(PickedMap* input, PickedMap* result);
	void debugPickedMap(PickedMap* input);
}

class iACompTableInteractorStyle: public vtkInteractorStyleTrackballCamera
{
public:
	//static iACompTableInteractorStyle* New();
	//vtkTypeMacro(iACompTableInteractorStyle, vtkInteractorStyleTrackballCamera);

	//initialization
	void setIACompHistogramVis(iACompHistogramVis* main);

	virtual void OnLeftButtonDown() override;
	virtual void OnLeftButtonUp() override;

	virtual void OnMouseMove() override;

	virtual void OnMiddleButtonDown() override;
	virtual void OnMiddleButtonUp() override;
	virtual void OnRightButtonDown() override;
	virtual void OnMouseWheelForward() override;
	virtual void OnMouseWheelBackward() override;
	virtual void OnKeyPress() override;
	virtual void OnKeyRelease() override;
	virtual void OnEnter() override;

	virtual void Pan() override;

	virtual void updateCharts() = 0;
	virtual void updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes) = 0;
	virtual void resetOtherCharts();

protected:
	iACompTableInteractorStyle();

	virtual iACompTable* getVisualization() = 0;

	virtual void resetHistogramTable()  = 0;

	/*** ***/
	void changeDistributionVisualizationForward();
	void changeDistributionVisualizationBackward();
	void onKeyDPressedMouseWheelForward();
	void onKeyDPressedMouseWheelBackward();

	/*** Interaction Camera Zoom ***/
	//general zooming in executed by the camera
	bool generalZoomIn();
	//general zooming out executed by the camera
	bool generalZoomOut();

	/*** Interaction Picking ***/
	virtual void storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id);
	
	virtual std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData) = 0;
	
	std::map<int, std::vector<double>>* calculateStatisticsForDatasets(
		QList<bin::BinType*>* zoomedRowData, std::vector<int>* indexOfPickedRows,
		std::vector<int>* amountObjectsEveryDataset, std::map<int, std::vector<double>>* result);
	
	csvDataType::ArrayType* formatPickedObjects(QList<std::vector<csvDataType::ArrayType*>*>* zoomedRowData);

	iACompHistogramVis* m_main;

	//is always 1 -> only needed for changing the zoom for the camera
	double m_zoomLevel;
	//if ture --> zooming is active, otherwise ordering according to selected bins is active
	bool m_zoomOn;

	//stores for each actor a vector for each picked cell according to their vtkIdType
	Pick::PickedMap* m_picked;
	//store for reinitialization after minimization etc. of application
	Pick::PickedMap* m_pickedOld;

	QList<bin::BinType*>* m_zoomedRowData;

	bool m_DButtonPressed;
};
