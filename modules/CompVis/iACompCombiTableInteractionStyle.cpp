#include "iACompCombiTableInteractionStyle.h"

#include <vtkObjectFactory.h> //for macro!

#include "iACompCombiTable.h"

vtkStandardNewMacro(iACompCombiTableInteractionStyle);

iACompCombiTableInteractionStyle::iACompCombiTableInteractionStyle() :
	iACompTableInteractorStyle(),
	m_visualization(nullptr)
{
}

void iACompCombiTableInteractionStyle::setVisualization(iACompCombiTable* visualization)
{
	m_visualization = visualization;
}

void iACompCombiTableInteractionStyle::OnMouseWheelForward()
{
	iACompTableInteractorStyle::OnMouseWheelForward();

	//camera zoom
	generalZoomIn();

}
void iACompCombiTableInteractionStyle::OnMouseWheelBackward()
{
	iACompTableInteractorStyle::OnMouseWheelBackward();

	//camera zoom
	generalZoomOut();
}

void iACompCombiTableInteractionStyle::updateCharts()
{
}
void iACompCombiTableInteractionStyle::updateOtherCharts(
	QList<std::vector<csvDataType::ArrayType*>*>* )
{
}

iACompTable* iACompCombiTableInteractionStyle::getVisualization()
{
	return m_visualization;
}

std::map<int, std::vector<double>>* iACompCombiTableInteractionStyle::calculatePickedObjects(
	QList<bin::BinType*>* )
{
	return nullptr;
}

void iACompCombiTableInteractionStyle::resetHistogramTable()
{
}
