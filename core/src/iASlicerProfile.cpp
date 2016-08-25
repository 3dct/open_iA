/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "iASlicerProfile.h"

#include <QErrorMessage>

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkConeSource.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkVersion.h>

inline float GetValueAsFloat(void * data, int scalarType, int index)
{
	switch(scalarType)
	{
	case VTK_CHAR :
		return (float) ( ((char*)data)[index] );
		break;
	case VTK_UNSIGNED_CHAR:
		return (float) ( ((unsigned char*)data)[index] );
		break;
	case VTK_UNSIGNED_SHORT:
		return (float) ( ((unsigned short*)data)[index] );
		break;
	case VTK_FLOAT:
		return (float) ( ((float*)data)[index] );
		break;
	case VTK_DOUBLE:
		return (float) ( ((double*)data)[index] );
		break;
	default:
		QErrorMessage errorMsg;
		errorMsg.showMessage("Datatype of image is not supprted by the slice-profile.");
		errorMsg.exec();
		return 0;
		break;
	}
}

void iASlicerProfile::SetVisibility( bool isVisible )
{
	m_profileLine.actor->SetVisibility(isVisible);
	for (vtkIdType i=0; i<2; i++)
		m_zeroLine.actors[i]->SetVisibility(isVisible);
	m_plotActor->SetVisibility(isVisible);
	m_plotActorHalo->SetVisibility(isVisible);
}

iASlicerProfile::iASlicerProfile() 
	:m_plotScaleFactor(0),
	m_plotPoints( vtkSmartPointer<vtkPoints>::New() ),
	m_plotPolyLine( vtkSmartPointer<vtkPolyLine>::New() ),
	m_plotCells( vtkSmartPointer<vtkCellArray>::New() ),
	m_plotPolyData( vtkSmartPointer<vtkPolyData>::New() ),
	m_plotMapper( vtkSmartPointer<vtkPolyDataMapper>::New() ),
	m_plotActor( vtkSmartPointer<vtkActor>::New() ),
	m_plotActorHalo( vtkSmartPointer<vtkActor>::New() ),
	m_ren(0)

{
	m_profileLine.actor->GetProperty()->SetColor(0.59, 0.73, 0.94);//ffa800//150, 186, 240
	m_profileLine.actor->GetProperty()->SetLineWidth(3.0);
	m_profileLine.actor->GetProperty()->SetLineStipplePattern(0x00ff);//0xf0f0
	m_profileLine.actor->GetProperty()->SetLineStippleRepeatFactor(1);
	m_profileLine.actor->GetProperty()->SetPointSize(3);

	for (vtkIdType i=0; i<2; i++)
	{
		m_zeroLine.actors[i]->GetProperty()->SetAmbientColor(1.0, 1.0, 1.0);
		m_zeroLine.actors[i]->GetProperty()->SetAmbient(1.0);
		//m_zeroLine.actors[i]->GetProperty()->SetLineWidth(1);
	}
	//plot
	m_plotPolyData->SetPoints(m_plotPoints);
	m_plotPolyData->SetLines(m_plotCells);
	m_plotMapper->SetInputData(m_plotPolyData);
	m_plotActor->SetMapper(m_plotMapper);
	m_plotActor->GetProperty()->SetOpacity(0.999);
	m_plotActor->GetProperty()->SetColor(1.0, 0.65, 0.0);//ffa800
	//m_plotActor->GetProperty()->SetLineWidth(1.5);

	m_plotActorHalo->SetMapper(m_plotMapper);
	m_plotActorHalo->GetProperty()->SetColor(0.0, 0.0, 0.0);
	m_plotActorHalo->GetProperty()->SetOpacity(0.3);
	m_plotActorHalo->GetProperty()->SetLineWidth(4);
}

void iASlicerProfile::initialize( vtkRenderer * ren )
{
	m_ren = ren;
	m_ren->AddActor(m_profileLine.actor);
	for (vtkIdType i=0; i<2; i++)
		m_ren->AddActor(m_zeroLine.actors[i]);
	m_ren->AddActor(m_plotActorHalo);
	m_ren->AddActor(m_plotActor);
}

int iASlicerProfile::setup( double posY, vtkImageData * imgData )
{
	double * spacing	= imgData->GetSpacing();
	double * origin		= imgData->GetOrigin();
	int * dimensions	= imgData->GetDimensions();
	int profileLength = dimensions[0]-1;
	// add point to the spline linePoints
	double startX = origin[0]; 
	double endX = origin[0] + profileLength*spacing[0];

	if( posY<origin[1] || posY >= origin[1] + dimensions[1]*spacing[1] )
		return 0;

	//setup profile horizontal line
	m_profileLine.points->SetPoint(	0, startX,	posY, iASlicerProfile::Z_COORD); 
	m_profileLine.points->SetPoint(	1, endX,	posY, iASlicerProfile::Z_COORD);
	m_profileLine.lineSource->SetPoint1(m_profileLine.points->GetPoint(0));
	m_profileLine.lineSource->SetPoint2(m_profileLine.points->GetPoint(1));

	//plot
	m_plotPoints->Initialize();
	m_plotPoints->Allocate(profileLength);
	m_plotPolyLine->GetPointIds()->SetNumberOfIds(profileLength);
	void * profileValues = imgData->GetScalarPointer( 0, (posY - origin[1]) / spacing[1] , 0 );
	float curProfVal;
	int scalarType = imgData->GetScalarType();

	float plotValueRange[2] = {0, 0};
	for (int i=0; i<profileLength; i++)//compute range and insert points
	{
		curProfVal = GetValueAsFloat(profileValues, scalarType, i);
		if(curProfVal < plotValueRange[0])
			plotValueRange[0] = curProfVal;
		if(curProfVal > plotValueRange[1])
			plotValueRange[1] = curProfVal;

		m_plotPolyLine->GetPointIds()->SetId(i,i);
	}

	if(plotValueRange[1] == plotValueRange[0])//zero division check
		m_plotScaleFactor = 0;
	else 
		m_plotScaleFactor = dimensions[1]*spacing[1]/(plotValueRange[1]-plotValueRange[0]);//normalize

	float offset = 0;
	if(plotValueRange[0] < 0)
		offset = -plotValueRange[0]*m_plotScaleFactor; 

	//zero-level pointers
	double zeroLevelPosY = origin[1] + offset;
	m_zeroLine.points->SetPoint(0, startX,	zeroLevelPosY, iASlicerProfile::Z_COORD); 
	m_zeroLine.points->SetPoint(1, endX,	zeroLevelPosY, iASlicerProfile::Z_COORD);
	for(int i=0; i<2; ++i)
		m_zeroLine.pointers[i]->SetCenter(m_zeroLine.points->GetPoint(i));
	m_zeroLine.setPointersScaling( spacing[0] > spacing[1] ? spacing[0] : spacing[1] );

	for (int i=0; i<profileLength; i++)
	{
		curProfVal = GetValueAsFloat(profileValues, scalarType, i);
		m_plotPoints->InsertPoint(
			i, 
			origin[0] + spacing[0] * (i + 0.5), 
			curProfVal * m_plotScaleFactor + offset, 
			iASlicerProfile::Z_COORD);
	}
	m_plotCells->Initialize();
	m_plotCells->InsertNextCell(m_plotPolyLine);
	m_plotPolyData->SetPoints(m_plotPoints);
	m_plotPolyData->SetLines(m_plotCells);
	m_plotPolyData->Modified();
	m_plotActorHalo->SetPosition(0, origin[1], iASlicerProfile::Z_COORD);
	m_plotActor->SetPosition(0, origin[1], iASlicerProfile::Z_COORD);

	return 1;
}

void iASlicerProfile::GetPoint( vtkIdType id, double pos_out[3] )
{
	m_profileLine.points->GetPoint(id, pos_out);
}
