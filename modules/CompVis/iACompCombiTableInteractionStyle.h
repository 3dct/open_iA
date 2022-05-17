#pragma once

//CompVis
#include "iACompTableInteractorStyle.h"

//vtk
#include "vtkActor.h"
#include "vtkSmartPointer.h"

class iACompCombiTable;

//vtk
class vtkRenderer;
class vtkPropPicker;

class iACompCombiTableInteractionStyle : public iACompTableInteractorStyle
{
public:
	static iACompCombiTableInteractionStyle* New();
	vtkTypeMacro(iACompCombiTableInteractionStyle, iACompTableInteractorStyle);

	//init table visualization
	void setVisualization(iACompCombiTable* visualization);
	virtual void OnMouseWheelForward() override;
	virtual void OnMouseWheelBackward() override;

	virtual void updateCharts() override;
	virtual void updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes) override;

protected:
	iACompCombiTableInteractionStyle();

	virtual iACompTable* getVisualization() override;
	virtual std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData) override;

	virtual void resetHistogramTable() override;

private:
	iACompCombiTable* m_visualization;
};