// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASlicerProfile.h"

#include "iAProfileColors.h"
#include "iATypedCallHelper.h"

#include <QErrorMessage>

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkConeSource.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

namespace
{
	template <typename T>
	void valueAsFloat(void * data, int index, float & out)
	{
		out = static_cast<float>((static_cast<T*>(data))[index]);
	}
}

void iASlicerProfile::setVisibility(bool isVisible)
{
	m_profileLine.actor->SetVisibility(isVisible);
	m_zeroLine.setVisible(isVisible);
	m_plotActor->SetVisibility(isVisible);
	m_plotActorHalo->SetVisibility(isVisible);
}

iASlicerProfile::iASlicerProfile():
	m_plotPolyLine(vtkSmartPointer<vtkPolyLine>::New()),
	m_plotCells(vtkSmartPointer<vtkCellArray>::New()),
	m_plotPolyData(vtkSmartPointer<vtkPolyData>::New()),
	m_plotPoints(vtkSmartPointer<vtkPoints>::New()),
	m_plotActor(vtkSmartPointer<vtkActor>::New()),
	m_plotActorHalo(vtkSmartPointer<vtkActor>::New()),
	m_plotMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_plotScaleFactor(0)
{
	m_profileLine.actor->GetProperty()->SetColor(ProfileLineColor.redF(), ProfileLineColor.greenF(), ProfileLineColor.blueF());
	m_profileLine.actor->GetProperty()->SetLineWidth(3.0);
	m_profileLine.actor->GetProperty()->SetLineStipplePattern(0x00ff);//0xf0f0
	m_profileLine.actor->GetProperty()->SetLineStippleRepeatFactor(1);
	m_profileLine.actor->GetProperty()->SetPointSize(3);

	//plot
	m_plotPolyData->SetPoints(m_plotPoints);
	m_plotPolyData->SetLines(m_plotCells);
	m_plotMapper->SetInputData(m_plotPolyData);
	m_plotActor->SetMapper(m_plotMapper);
	m_plotActor->GetProperty()->SetOpacity(0.999);
	m_plotActor->GetProperty()->SetColor(ProfileStartColor.redF(), ProfileStartColor.greenF(), ProfileStartColor.blueF());
	//m_plotActor->GetProperty()->SetLineWidth(1.5);

	m_plotActorHalo->SetMapper(m_plotMapper);
	m_plotActorHalo->GetProperty()->SetColor(0.0, 0.0, 0.0);
	m_plotActorHalo->GetProperty()->SetOpacity(0.3);
	m_plotActorHalo->GetProperty()->SetLineWidth(4);
}

void iASlicerProfile::addToRenderer(vtkRenderer * ren)
{
	ren->AddActor(m_profileLine.actor);
	m_zeroLine.addToRenderer(ren);
	ren->AddActor(m_plotActorHalo);
	ren->AddActor(m_plotActor);
}

bool iASlicerProfile::updatePosition(double posY, vtkImageData * imgData)
{
	double const * spacing = imgData->GetSpacing();
	double const * origin  = imgData->GetOrigin();
	int const * dimensions = imgData->GetDimensions();
	int profileLength = dimensions[0]-1;
	// add point to the spline linePoints
	double startX = origin[0];
	double endX = origin[0] + profileLength*spacing[0];

	if (posY < origin[1] || posY >= origin[1] + dimensions[1] * spacing[1])
	{
		return false;
	}

	//setup profile horizontal line
	m_profileLine.setPoint(0, startX, posY, ZCoord);
	m_profileLine.setPoint(1, endX, posY, ZCoord);

	//plot
	m_plotPoints->Initialize();
	m_plotPoints->Allocate(profileLength);
	m_plotPolyLine->GetPointIds()->SetNumberOfIds(profileLength);
	void * profileValues = imgData->GetScalarPointer( 0, (posY - origin[1]) / spacing[1] , 0 );
	float curProfVal;
	int scalarType = imgData->GetScalarType();

	float plotValueRange[2] = {std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest() };
	for (int i=0; i<profileLength; i++)//compute range and insert points
	{
		VTK_TYPED_CALL(valueAsFloat, scalarType, profileValues, i, curProfVal);
		if (curProfVal < plotValueRange[0])
		{
			plotValueRange[0] = curProfVal;
		}
		if (curProfVal > plotValueRange[1])
		{
			plotValueRange[1] = curProfVal;
		}

		m_plotPolyLine->GetPointIds()->SetId(i,i);
	}

	if (plotValueRange[1] == plotValueRange[0]) //zero division check
	{
		m_plotScaleFactor = 0;
	}
	else
	{
		m_plotScaleFactor = dimensions[1] * spacing[1] / (plotValueRange[1] - plotValueRange[0]);//normalize
	}

	float offset = 0;
	if (plotValueRange[0] < 0)
	{
		offset = -plotValueRange[0] * m_plotScaleFactor;
	}

	//zero-level pointers
	double zeroLevelPosY = origin[1] + offset;
	m_zeroLine.updatePosition(posY, zeroLevelPosY, startX, endX, spacing);

	for (int i=0; i<profileLength; i++)
	{
		VTK_TYPED_CALL(valueAsFloat, scalarType, profileValues, i, curProfVal);
		m_plotPoints->InsertPoint(
			i,
			origin[0] + spacing[0] * (i + 0.5),
			curProfVal * m_plotScaleFactor + offset,
			ZCoord);
	}
	m_plotCells->Initialize();
	m_plotCells->InsertNextCell(m_plotPolyLine);
	m_plotPolyData->SetPoints(m_plotPoints);
	m_plotPolyData->SetLines(m_plotCells);
	m_plotPolyData->Modified();
	m_plotActorHalo->SetPosition(0, origin[1], ZCoord);
	m_plotActor->SetPosition(0, origin[1], ZCoord);

	return true;
}

void iASlicerProfile::point(vtkIdType id, double pos_out[3])
{
	m_profileLine.point(id, pos_out);
}
