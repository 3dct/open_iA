#pragma once

//CompVis
#include "iACompTableInteractorStyle.h"


//vtk
#include "vtkSmartPointer.h"
#include "vtkActor.h"

class iACompCurve;

//vtk
class vtkRenderer;
class vtkPropPicker;

class iACompCurveInteractorStyle : public iACompTableInteractorStyle
{
public:
	static iACompCurveInteractorStyle* New();
	vtkTypeMacro(iACompCurveInteractorStyle, iACompTableInteractorStyle);

	//init table visualization
	void setCurveVisualization(iACompCurve* visualization);
	virtual void OnMouseWheelForward() override;
	virtual void OnMouseWheelBackward() override;

	virtual void updateCharts() override;
	virtual void updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes) override;

protected:
	iACompCurveInteractorStyle();

	virtual iACompTable* getVisualization() override;
	virtual std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData) override;

	virtual void resetHistogramTable() override;

private:
	iACompCurve* m_visualization;
};
