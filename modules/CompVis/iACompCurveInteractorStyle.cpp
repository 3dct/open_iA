#include "iACompCurveInteractorStyle.h"
#include <vtkObjectFactory.h> //for macro!

#include "iACompCurve.h"

vtkStandardNewMacro(iACompCurveInteractorStyle);

iACompCurveInteractorStyle::iACompCurveInteractorStyle() : 
	iACompTableInteractorStyle(),
	m_visualization(nullptr)
{
}

void iACompCurveInteractorStyle::setCurveVisualization(iACompCurve* visualization)
{
	m_visualization = visualization;
}

void iACompCurveInteractorStyle::OnLeftButtonDown()
{
}
void iACompCurveInteractorStyle::OnLeftButtonUp()
{
}

void iACompCurveInteractorStyle::OnMouseMove()
{
}

void iACompCurveInteractorStyle::OnMiddleButtonDown()
{
}
void iACompCurveInteractorStyle::OnRightButtonDown()
{
}
void iACompCurveInteractorStyle::OnMouseWheelForward()
{
	iACompTableInteractorStyle::OnMouseWheelForward();
}
void iACompCurveInteractorStyle::OnMouseWheelBackward()
{
	iACompTableInteractorStyle::OnMouseWheelBackward();
}
void iACompCurveInteractorStyle::OnKeyPress()
{
}
void iACompCurveInteractorStyle::OnKeyRelease()
{
}

void iACompCurveInteractorStyle::Pan()
{
}

void iACompCurveInteractorStyle::updateCharts()
{
}
void iACompCurveInteractorStyle::updateOtherCharts(
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes)
{
}

iACompTable* iACompCurveInteractorStyle::getVisualization()
{
	return m_visualization;
}

std::map<int, std::vector<double>>* iACompCurveInteractorStyle::calculatePickedObjects(
	QList<bin::BinType*>* zoomedRowData)
{
	return nullptr;
}

void iACompCurveInteractorStyle::resetHistogramTable()
{
}