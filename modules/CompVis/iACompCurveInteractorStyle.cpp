// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

void iACompCurveInteractorStyle::OnMouseWheelForward()
{
	iACompTableInteractorStyle::OnMouseWheelForward();

	//camera zoom
	generalZoomIn();
}
void iACompCurveInteractorStyle::OnMouseWheelBackward()
{
	iACompTableInteractorStyle::OnMouseWheelBackward();

	//camera zoom
	generalZoomOut();
}

void iACompCurveInteractorStyle::updateCharts()
{
}
void iACompCurveInteractorStyle::updateOtherCharts(
	QList<std::vector<csvDataType::ArrayType*>*>* )
{
}

iACompTable* iACompCurveInteractorStyle::getVisualization()
{
	return m_visualization;
}

std::map<int, std::vector<double>>* iACompCurveInteractorStyle::calculatePickedObjects(
	QList<bin::BinType*>* )
{
	return nullptr;
}

void iACompCurveInteractorStyle::resetHistogramTable()
{
}
