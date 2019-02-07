/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "../include/Plot3DVtk.h"
#include "../include/common.h"
//VTK
#include <vtkObject.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkIntArray.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkVoxel.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkDepthSortPolyData.h>
#include <vtkPointPicker.h>
#include <vtkQuad.h>
#include <vtkAxes.h>
#include <vtkProperty2D.h>
#include <vtkAxisActor2D.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkPointData.h>
#include <vtkConeSource.h>
#include <vtkVersion.h>

Plot3DVtk::Plot3DVtk():
					m_correctTransparency(0),
					lastPickSuccessful(0)
{
	m_points = 0;
	m_cellScalars = 0;
	m_depthSort = 0;
	m_renderer = vtkRenderer::New();
	m_grid = vtkPolyData::New();

	m_cellNumbers = vtkIntArray::New();
	m_cellNumbers->SetNumberOfComponents(1);
	m_cellNumbers->SetNumberOfTuples(0);
	m_cellNumbers->SetName("Numbers");

	m_mapper = vtkPolyDataMapper::New();
	m_actor = vtkActor::New();
	m_actor->SetMapper(m_mapper);
	m_actor->GetProperty()->LightingOff();
	m_actor->GetProperty()->SetAmbient(1);
	m_wireMapper = vtkPolyDataMapper::New();
	m_wireMapper->SetResolveCoincidentTopologyToPolygonOffset();
	m_wireMapper->ScalarVisibilityOff();
	m_wireActor = vtkActor::New();
	m_wireActor->GetProperty()->SetRepresentationToWireframe();
	m_wireActor->SetMapper(m_wireMapper);
	m_pickedMapper = vtkPolyDataMapper::New();
	m_pickedMapper->SetResolveCoincidentTopologyToPolygonOffset();
	//m_pickedMapper->ScalarVisibilityOff();
	m_pickedActor = vtkActor::New();
	//m_pickedActor->GetProperty()->SetRepresentationToWireframe();
	m_pickedActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
	//m_pickedActor->GetProperty()->LightingOff();
	//m_pickedActor->GetProperty()->SetAmbient(1);
	//m_pickedActor->GetProperty()->SetAmbientColor(1.0, 1.0, 0.0);
	m_lookupTable = vtkLookupTable::New();

	m_scalarBarActor = vtkScalarBarActor::New();
	/*m_scalarBarActor->GetTitleTextProperty()->SetFontFamilyToArial();
	m_scalarBarActor->GetTitleTextProperty()->BoldOff();
	m_scalarBarActor->GetTitleTextProperty()->ItalicOff();
	m_scalarBarActor->GetTitleTextProperty()->ShadowOff();
	m_scalarBarActor->SetLabelFormat("%3.3f");
	m_scalarBarActor->GetLabelTextProperty()->BoldOff();
	m_scalarBarActor->GetLabelTextProperty()->ItalicOff();
	m_scalarBarActor->GetLabelTextProperty()->ShadowOff();*/
	m_scalarBarActor->SetHeight(0.17);
	m_scalarBarActor->SetWidth(0.3);
	m_scalarBarActor->SetPosition(0.3,0.01);
	//m_scalarBarActor->VisibilityOff();
	m_scalarBarWidget=vtkScalarBarWidget::New();
	m_scalarBarWidget->SetScalarBarActor(m_scalarBarActor);
	m_scalarBarActor->SetTitle("a");
	m_scalarBarActor->SetOrientationToHorizontal();
	m_picker = vtkPointPicker::New();

	m_cubeAxes = vtkCubeAxesActor2D::New();
	//m_cubeAxes->SetLabelFormat("%3.3f");
	//m_cubeAxes->SetXLabel("X");
	//m_cubeAxes->SetYLabel("Y");
	//m_cubeAxes->SetZLabel("Z");
	m_cubeAxes->SetFlyModeToOuterEdges();
	//m_cubeAxes->SetFontFactor(1.5);
	//m_cubeAxes->GetAxisTitleTextProperty()->ShadowOff();
	//m_cubeAxes->GetAxisLabelTextProperty()->ShadowOff();
	//m_cubeAxes->GetAxisLabelTextProperty()->BoldOff();
	m_cubeAxes->ScalingOff();
	m_cubeAxes->SetCamera(m_renderer->GetActiveCamera());
	//m_cubeAxes->GetXAxisActor2D()->AdjustLabelsOn();
	//m_cubeAxes->GetYAxisActor2D()->AdjustLabelsOn();
	//m_cubeAxes->GetZAxisActor2D()->AdjustLabelsOn();
	//m_cubeAxes->UseRangesOn();
	//m_cubeAxes->SetRanges(0, 1, 0, 1, 0, 1);
	//m_cubeAxes->GetAxisLabelTextProperty()->SetColor(1.,1.,1.);
	//m_cubeAxes->GetAxisTitleTextProperty()->SetColor(1.,1.,1.);
	m_cubeAxes->GetProperty()->SetColor(1.,1.,1.);

	m_renderer->AddActor(m_scalarBarActor);
	m_renderer->AddActor(m_actor);
	m_renderer->AddActor(m_wireActor);
	m_renderer->AddActor(m_pickedActor);
	m_renderer->AddActor(m_cubeAxes);
	m_renderer->SetBackground2(0.5, 0.66666666666666666666666666666667, 1);
}

Plot3DVtk::~Plot3DVtk()
{
	if (m_points!=0) m_points->Delete();
	if (m_grid!=0) m_grid->Delete();
	if (m_cellScalars!=0) m_cellScalars->Delete();
	if (m_cellNumbers!=0) m_cellNumbers->Delete();
	if (m_mapper!=0) m_mapper->Delete();
	if (m_wireMapper!=0) m_wireMapper->Delete();
	if (m_actor!=0) m_actor->Delete();
	if (m_wireActor!=0) m_wireActor->Delete();
	if (m_lookupTable!=0) m_lookupTable->Delete();
	if (m_scalarBarActor!=0) m_scalarBarActor->Delete();
	if (m_scalarBarWidget!=0) m_scalarBarWidget->Delete();
	if (m_depthSort!=0) m_depthSort->Delete();
	if (m_renderer!=0) m_renderer->Delete();
	if (m_picker!=0) m_picker->Delete();
	if (m_cubeAxes!=0) m_cubeAxes->Delete();
	if (m_pickedMapper!=0) m_pickedMapper->Delete();
	if (m_pickedActor!=0) m_pickedActor->Delete();
}

void Plot3DVtk::SetSolidColor(double r, double g, double b)
{
	m_actor->GetProperty()->SetColor(r, g, b);
}

void Plot3DVtk::SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
	MinX=xmin;
	MaxX=xmax;
	MinY=ymin;
	MaxY=ymax;
	MinZ=zmin;
	MaxZ=zmax;
}

void Plot3DVtk::Update()
{
	m_mapper->ScalarVisibilityOn();
	m_mapper->SetScalarModeToUsePointData();
	if(!m_correctTransparency)
	{
		m_mapper->SetInputData(m_grid);
	}
	m_mapper->SetLookupTable(m_lookupTable);
	m_mapper->Update();

	m_wireMapper->SetInputData(m_grid);
	m_wireMapper->Update();
	//m_pickedMapper->SetInput(m_pickedGrid);
	if(m_pickedMapper->GetInput() != 0)//otherwise some vtk warnings do appear
		m_pickedMapper->Update();
	m_scalarBarActor->SetLookupTable(m_lookupTable);
}

void Plot3DVtk::SetGridCellNumbers( long *data )
{
	unsigned int i=0;
	for( unsigned int x = 0; x < m_sizeX; x+=1)
	for( unsigned int y = 0; y < 1; y+=1)
	for( unsigned int z = 0; z < m_sizeZ; z+=1)
	{
		m_cellNumbers->InsertNextTuple1(data[i]);
		i++;
	}
	m_grid->GetCellData()->AddArray(m_cellNumbers);
}

void Plot3DVtk::SetAutoScalarRange()
{
	m_Smin = 0;
	m_Smax = m_lookupTable->GetNumberOfTableValues();;

	m_lookupTable->SetTableRange(m_Smin, m_Smax);
	m_mapper->SetLookupTable(m_lookupTable);
	m_mapper->SetScalarRange(m_Smin, m_Smax);
	m_mapper->ScalarVisibilityOn();
}

void Plot3DVtk::SetUserScalarRange( double Smin, double Smax )
{
	/*double eps = Smin<0 ? -Smin*0.1 : Smin*0.1;////TODO: bad style
	Smin-=eps; Smax+=eps;*/
	m_Smin = Smin;
	m_Smax = Smax;
	
	m_lookupTable->SetTableRange(Smin, Smax);
	m_mapper->SetLookupTable(m_lookupTable);
	m_mapper->SetScalarRange(Smin, Smax);
	m_mapper->ScalarVisibilityOn();
}

void Plot3DVtk::ShowWireGrid( int show, float r, float g, float b )
{
	m_showgrid = show;
	m_wireActor->GetProperty()->SetColor(r, g, b);
	m_wireActor->SetVisibility(m_showgrid);
}

void Plot3DVtk::SetOpacity( double opacity )
{
	if (opacity==m_actor->GetProperty()->GetOpacity())
		return;
	m_actor->GetProperty()->SetOpacity(opacity);
	m_wireActor->GetProperty()->SetOpacity(opacity);
}

void Plot3DVtk::RenderWthCorrectTransparency( int correctTransparency )
{
	/*if(m_correctTransparency==correctTransparency)
		return;
	m_correctTransparency = correctTransparency;
	if(correctTransparency!=0)
	{
		if (m_depthSort!=0) m_depthSort->Delete();
		m_depthSort = vtkDepthSortPolyData::New();
		m_depthSort->SetCamera(m_renderer->GetActiveCamera());
		m_depthSort->SetDepthSortModeToParametricCenter();
		m_depthSort->SetInput(modelPD);
		m_mapper->SetInput(m_depthSort->GetOutput());
	}
	else
	{
		if (m_depthSort!=0) m_depthSort->Delete();
		m_mapper->SetInput(m_grid);
	}
	Update();*/
}

void  Plot3DVtk::Pick( double xpos, double ypos)
{
	//empty
	m_picker->Pick(xpos, ypos, 0, m_renderer);
	if(m_picker->GetMapper()==0)
		return;
	if (m_picker->GetMapper()->IsA("vtkPolyDataMapper"))
	{
		if (((vtkPolyDataMapper *)m_picker->GetMapper()) == m_mapper)
		{
			m_picker->GetPickPosition(pickData.pos);
			pickData.pntnum = m_picker->GetPointId();
			//div by number of cube facets
			pickData.xInd = (pickData.pntnum)%m_sizeX;
			pickData.zInd = (pickData.pntnum)/m_sizeX;
			HighlightPickedPoint();
			lastPickSuccessful = 1;
			return;
		}
	}
	lastPickSuccessful = 0;
}

void Plot3DVtk::SetPalette( int count, double *colors )
{//çàäàòü ïàëèòðó
	if (m_lookupTable!=0) 
	{
		m_lookupTable->Delete();
		m_lookupTable=0;
	}
	if (m_lookupTable==0) m_lookupTable = vtkLookupTable::New();
	m_lookupTable->SetNumberOfTableValues(count);//ñîçäàåì ïàëèòðó íà óêàçàííîå ÷èñëî öâåòîâ
	for( int i = 0; i < count; i++)//êàæäîìó ýëåìåíòó ïàëèòðû çàäàòü ñâîé öâåò
	{
		m_lookupTable->SetTableValue( i, colors[3*i], colors[3*i+1], colors[3*i+2] );
	}
	m_lookupTable->Build();
	//set auto scalar range
	m_Smin = 0;
	m_Smax = count;

	m_lookupTable->SetTableRange(m_Smin, m_Smax);
	m_mapper->SetLookupTable(m_lookupTable);
	m_mapper->SetScalarRange(m_Smin, m_Smax);
	m_mapper->ScalarVisibilityOn();
}

void Plot3DVtk::SetPalette( int count, unsigned int r1, unsigned int g1, unsigned int b1, unsigned int r2, unsigned int g2, unsigned int b2 )
{
	//m_lookupTable->SetRampToLinear();
	if (m_lookupTable!=0) 
	{
		m_lookupTable->Delete();
		m_lookupTable=0;
	}
	if (m_lookupTable==0) m_lookupTable = vtkLookupTable::New();
	m_lookupTable->SetNumberOfTableValues(count);//     
	double dr = (double)r2-(double)r1;
	double dg = (double)g2-(double)g1;
	double db = (double)b2-(double)b1;
	for( int i = 0; i < count; i++)//
	{
		m_lookupTable->SetTableValue(i,
			((double)r1+dr*i/count)/255, 
			((double)g1+dg*i/count)/255,
			((double)b1+db*i/count)/255);
	}	
	m_lookupTable->Build();
}

void Plot3DVtk::SetAxesParams( int showaxes, int showlabels, double color[3], long fontfactor )
{
	m_cubeAxes->SetVisibility(showaxes);
	m_cubeAxes->GetXAxisActor2D()->SetLabelVisibility(showlabels);
	m_cubeAxes->GetYAxisActor2D()->SetLabelVisibility(showlabels);
	m_cubeAxes->GetZAxisActor2D()->SetLabelVisibility(showlabels);
	m_cubeAxes->GetProperty()->SetColor(color[0], color[1], color[2]);
	m_cubeAxes->SetFontFactor(0.3+0.2*fontfactor);
	m_wireActor->GetProperty()->SetColor(color[0], color[1], color[2]);
}

void Plot3DVtk::HighlightPickedPoint()
{
	vtkConeSource * src = vtkConeSource::New();
	src->SetRadius(0.025);
	src->SetHeight(0.1);
	src->SetResolution(12);
	src->SetDirection(0, -1, 0);
	src->SetCenter(pickData.pos[0], pickData.pos[1]+0.5*src->GetHeight(), pickData.pos[2]);
	m_pickedMapper->SetInputConnection(src->GetOutputPort());
	m_pickedActor->SetMapper(m_pickedMapper);
	src->Delete();
	Update();
}

void Plot3DVtk::loadFromData( double * plotData, double * scalars, int cntX, int cntZ, float scale)
{
	//êàæäàÿ ïëîñêîñòü çàäàåòñÿ 12 ÷èñëàìè èç ìàññèâà òî÷åê â ëåäóþùåì ïîðÿäêå
	//x1y1z1 x2y2z2 x3y3z3 x4y4z4 è òä
	unsigned int maxDim = max_macro(cntX,cntZ);
	float clenX = 2.0f/(float)(cntX-1);
	float clenZ = 2.0f/(float)(cntZ-1);
	m_cubeAxes->SetRanges(0, maxDim, 0, maxDim, 0, maxDim);
	vtkQuad *GridQuad=vtkQuad::New();
	vtkCellArray *gridCells=vtkCellArray::New();
	m_sizeX = cntX; m_sizeZ = cntZ;
	if (m_points!=0) m_points->Delete();
	m_points = vtkPoints::New();
	float offs = -1.0;
	if (m_cellScalars!=0) 
		m_cellScalars->Delete();
	m_cellScalars = vtkFloatArray::New();
	m_cellScalars->SetNumberOfComponents(1);
	m_cellScalars->SetNumberOfTuples(0);
	for( int z = 0; z < (int)m_sizeZ; ++z)
		for( int x = 0; x < (int)m_sizeX; ++x)
		{
			m_points->InsertNextPoint((x+0)*clenX+offs, plotData[(x+0) + (z+0)*cntX]*scale+offs, (z+0)*clenZ+offs);
			m_cellScalars->InsertNextTuple1(scalars[(x+0) + (z+0)*cntX]);
		}
	//
	for( int z = 0; z < (int)m_sizeZ-1; ++z)
		for( int x = 0; x < (int)m_sizeX-1; ++x)
		{
			GridQuad->GetPointIds()->SetId(0, (x+0) + (z+0)*cntX);
			GridQuad->GetPointIds()->SetId(1, (x+1) + (z+0)*cntX);
			GridQuad->GetPointIds()->SetId(2, (x+1) + (z+1)*cntX);
			GridQuad->GetPointIds()->SetId(3, (x+0) + (z+1)*cntX);
			gridCells->InsertNextCell(GridQuad);
		}
	/*for( int z = 0; z < (int)m_sizeZ-1; ++z)
	for( int x = 0; x < (int)m_sizeX-1; ++x)
	{
		vtkIdType k = m_points->GetNumberOfPoints();
		m_points->InsertNextPoint((x+0)*clenX+offs, plotData[(x+0) + (z+0)*cntX]*scale+offs, (z+0)*clenZ+offs);
		m_cellScalars->InsertNextTuple1(scalars[(x+0) + (z+0)*cntX]);
		m_points->InsertNextPoint((x+1)*clenX+offs, plotData[(x+1) + (z+0)*cntX]*scale+offs, (z+0)*clenZ+offs);
		m_cellScalars->InsertNextTuple1(scalars[(x+1) + (z+0)*cntX]);
		m_points->InsertNextPoint((x+1)*clenX+offs, plotData[(x+1) + (z+1)*cntX]*scale+offs, (z+1)*clenZ+offs);
		m_cellScalars->InsertNextTuple1(scalars[(x+1) + (z+1)*cntX]);
		m_points->InsertNextPoint((x+0)*clenX+offs, plotData[(x+0) + (z+1)*cntX]*scale+offs, (z+1)*clenZ+offs);
		m_cellScalars->InsertNextTuple1(scalars[(x+0) + (z+1)*cntX]);

		GridQuad->GetPointIds()->SetId(0, k+0);
		GridQuad->GetPointIds()->SetId(1, k+1);
		GridQuad->GetPointIds()->SetId(2, k+2);
		GridQuad->GetPointIds()->SetId(3, k+3);
		gridCells->InsertNextCell(GridQuad);
	}*/
	m_grid->SetPoints(m_points);
	m_grid->SetPolys(gridCells);
	m_grid->GetPointData()->SetScalars(m_cellScalars);
	gridCells->Delete();
	GridQuad->Delete();
	Update();
}

int Plot3DVtk::GetNumberOfLookupTableValues()
{
	return m_lookupTable->GetNumberOfTableValues();
}

void Plot3DVtk::setPicked( int indX, int indZ )
{
	pickData.xInd = indX;
	pickData.zInd = indZ;
	m_grid->GetPoints()->GetPoint(indX + indZ*m_sizeZ, pickData.pos);
	HighlightPickedPoint();
}
