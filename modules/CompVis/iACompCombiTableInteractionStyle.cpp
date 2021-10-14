#include "iACompCombiTableInteractionStyle.h"

#include <vtkObjectFactory.h> //for macro!

#include "iACompCombiTable.h"

vtkStandardNewMacro(iACompCombiTableInteractionStyle);

iACompCombiTableInteractionStyle::iACompCombiTableInteractionStyle() : m_visualization(nullptr)
{
}

void iACompCombiTableInteractionStyle::setVisualization(iACompCombiTable* visualization)
{
	m_visualization = visualization;
}

void iACompCombiTableInteractionStyle::OnLeftButtonDown()
{
}
void iACompCombiTableInteractionStyle::OnLeftButtonUp()
{
}

void iACompCombiTableInteractionStyle::OnMouseMove()
{
}

void iACompCombiTableInteractionStyle::OnMiddleButtonDown()
{
}
void iACompCombiTableInteractionStyle::OnRightButtonDown()
{
}
void iACompCombiTableInteractionStyle::OnMouseWheelForward()
{
	iACompTableInteractorStyle::OnMouseWheelForward();
}
void iACompCombiTableInteractionStyle::OnMouseWheelBackward()
{
	iACompTableInteractorStyle::OnMouseWheelBackward();
}
void iACompCombiTableInteractionStyle::OnKeyPress()
{
}
void iACompCombiTableInteractionStyle::OnKeyRelease()
{
}

void iACompCombiTableInteractionStyle::Pan()
{
}

void iACompCombiTableInteractionStyle::updateCharts()
{
}
void iACompCombiTableInteractionStyle::updateOtherCharts(
	QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes)
{
}

iACompTable* iACompCombiTableInteractionStyle::getVisualization()
{
	return m_visualization;
}

std::map<int, std::vector<double>>* iACompCombiTableInteractionStyle::calculatePickedObjects(
	QList<bin::BinType*>* zoomedRowData)
{
	return nullptr;
}

void iACompCombiTableInteractionStyle::resetHistogramTable()
{
}