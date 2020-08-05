/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVRDistributionVis.h"

#include <iAConsole.h>
#include <iAvec3.h>

#include <vtkRegularPolygonSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkMapper.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkTextActor3D.h>
#include <vtkMath.h>
#include <vtkStringArray.h>
#include <vtkPropCollection.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkCellData.h>
#include <vtkLine.h>
#include <vtkAppendPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkCubeSource.h>

iAVRDistributionVis::iAVRDistributionVis(vtkRenderer* ren, iAVRMetrics* fiberMetric, vtkTable* objectTable, iACsvIO io) :m_renderer(ren), m_fiberMetric(fiberMetric), m_sphereActor(vtkSmartPointer<vtkActor>::New()), m_objectTable(objectTable), m_io(io)
{
	m_visible = false;
	m_axesPoly = new std::vector<vtkSmartPointer<vtkPolyData>>();
	m_axesMarksPoly = new std::vector<std::vector<vtkSmartPointer<vtkPolyData>>>();
	m_axisActor = vtkSmartPointer<vtkActor>::New();
	m_axisLabelActor = new std::vector<std::vector<std::vector<iAVR3DText>>>();
	m_AxesViewDir = new std::unordered_map<vtkIdType, iAVec3d>();
	m_activeHistogramActor = vtkSmartPointer<vtkActor>::New();

	m_radius = 280;
	m_numberOfXBins = 6;
	m_numberOfYBins = 6;
	m_offsetFromCenter = 65;
	axisInView = -1;
}

void iAVRDistributionVis::createVisualization(double *pos, int level, std::vector<vtkIdType>* regions)
{
	// Create a circle
	vtkSmartPointer<vtkRegularPolygonSource> polygonSource = vtkSmartPointer<vtkRegularPolygonSource>::New();
	polygonSource->GeneratePolygonOff(); // Uncomment this line to generate only the outline of the circle
	polygonSource->SetNumberOfSides(35);
	polygonSource->SetRadius(m_radius);
	polygonSource->SetCenter(pos);
	polygonSource->SetNormal(0, 1, 0);

	vtkSmartPointer<vtkPolyDataMapper> mapper =	vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(polygonSource->GetOutputPort());;

	m_sphereActor = vtkSmartPointer<vtkActor>::New();
	m_sphereActor->SetMapper(mapper);
	m_sphereActor->GetProperty()->SetOpacity(0.5);

	m_axesPoly->clear();
	m_axesMarksPoly->clear();
	m_axisLabelActor->clear();
	m_AxesViewDir->clear();

	//start from new centerPos
	double* newCenterPos = new double();
	
	for (int features = 0; features < 5; features++)
	{
		double posOnCircle[3];
		calculateAxisPositionInCircle(features, 5, pos, m_radius, posOnCircle);
		calculateCenterOffsetPos(pos, posOnCircle, newCenterPos);
		calculateAxis(newCenterPos, posOnCircle);
		createAxisMarks(features);
		createAxisLabels(features);
		calculateAxesViewDir(features);
	}
	drawAxes();
	showHistogram(0);

	m_fiberMetric->getHistogram(level, nullptr, regions->at(0), regions->at(1));
}

void iAVRDistributionVis::show()
{
	if (m_visible)
	{
		return;
	}

	m_renderer->AddActor(m_sphereActor);
	m_renderer->AddActor(m_axisActor);
	m_renderer->AddActor(m_activeHistogramActor);

	//for (int i = 0; i < m_axisLabelActor->size(); i++)
	//{
	//	for (int j = 0; j < m_axisLabelActor->at(i).size(); j++)
	//	{
	//		for (int k = 0; k < m_axisLabelActor->at(i).at(j).size(); k++)
	//		{
	//			m_axisLabelActor->at(i).at(j).at(k).show();
	//		}
	//	}
	//}

	m_visible = true;
}

//! Show the axis marks depinding on the view direction
//! Hide the last shown axis marks
void iAVRDistributionVis::showAxisMarksInView(double* viewDir)
{
	double minDistance = std::numeric_limits<double>::infinity();
	int newAxisInView = -1;

	for (int i = 0; i < m_axisLabelActor->size(); i++)
	{
		auto focalP = iAVec3d(viewDir[0], viewDir[1], viewDir[2]);
		auto ray = (m_AxesViewDir->at(i) - (focalP));
		auto tempDistance = ray.length();
		
		//auto axisV = iAVec3d(abs(m_AxesViewDir->at(i)[0]), abs(m_AxesViewDir->at(i)[1]), abs(m_AxesViewDir->at(i)[2]));
		//auto absV = iAVec3d(abs(viewDir[0]), abs(viewDir[1]), abs(viewDir[2]));
		//auto tempDistance = abs((axisV - absV).length());
		if (tempDistance < minDistance)
		{
			minDistance = tempDistance;
			newAxisInView = i;
		}
	}
	if(newAxisInView != -1 && axisInView != newAxisInView)
	{
		if(axisInView != -1)
		{
			//Delete old
			for (int j = 0; j < m_axisLabelActor->at(axisInView).size(); j++)
			{
				for (int k = 0; k < m_axisLabelActor->at(axisInView).at(j).size(); k++)
				{
					m_axisLabelActor->at(axisInView).at(j).at(k).hide();
				}
			}
		}
		//Create new
		for (int j = 0; j < m_axisLabelActor->at(newAxisInView).size(); j++)
		{
			for (int k = 0; k < m_axisLabelActor->at(newAxisInView).at(j).size(); k++)
			{
				m_axisLabelActor->at(newAxisInView).at(j).at(k).show();
			}
		}
	}
	axisInView = newAxisInView;
}

void iAVRDistributionVis::hide()
{
	if (!m_visible)
	{
		return;
	}

	m_renderer->RemoveActor(m_sphereActor);
	m_renderer->RemoveActor(m_axisActor);
	m_renderer->RemoveActor(m_activeHistogramActor);

	for (int i = 0; i < m_axisLabelActor->size(); i++)
	{
		for (int j = 0; j < m_axisLabelActor->at(i).size(); j++)
		{
			for (int k = 0; k < m_axisLabelActor->at(i).at(j).size(); k++)
			{
				m_axisLabelActor->at(i).at(j).at(k).hide();
			}
		}
	}

	m_visible = false;
}

void iAVRDistributionVis::showHistogram(int axis)
{
	vtkSmartPointer<vtkPoints> barPoints = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkPolyData> bars = vtkSmartPointer<vtkPolyData>::New();

	//vtkSmartPointer<vtkDoubleArray> glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	//glyphScales->SetName("scales");
	//glyphScales->SetNumberOfComponents(3);

	vtkSmartPointer<vtkUnsignedCharArray> glyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	glyphColor->SetName("colors");
	glyphColor->SetNumberOfComponents(4);

	double cubeSize[3] = {12,12,12}; //Should depend on size between two marks

	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	cubeSource->SetXLength(cubeSize[0]);
	cubeSource->SetYLength(cubeSize[1]);
	cubeSource->SetZLength(cubeSize[2]);

	for (vtkIdType point = 0; point < m_axesMarksPoly->at(axis).at(0)->GetNumberOfPoints(); point += 2)
	{
		// Move first cube a little bit to the left and second to the right
		auto markPos = m_axesMarksPoly->at(axis).at(0)->GetPoint(point);
		double markPosShiftedLeft[3] = { markPos[0] - cubeSize[0] / 2, markPos[1], markPos[2] };
		double markPosShiftedRight[3] = { markPos[0] + cubeSize[0] / 2, markPos[1], markPos[2] };
		
		calculateBarsWithCubes(markPosShiftedLeft, cubeSize, point, barPoints, glyphColor, QColor(85,217,73,255));
		calculateBarsWithCubes(markPosShiftedRight, cubeSize, point +1, barPoints, glyphColor, QColor(217, 73, 157, 255));
	}

	bars->SetPoints(barPoints);
	bars->GetPointData()->AddArray(glyphColor);
	//bars->GetPointData()->SetScalars(glyphScales);

	m_activeGlyphHistogram = vtkSmartPointer<vtkGlyph3D>::New();
	m_activeGlyphHistogram->GeneratePointIdsOn();
	m_activeGlyphHistogram->SetSourceConnection(cubeSource->GetOutputPort());
	m_activeGlyphHistogram->SetInputData(bars);

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(m_activeGlyphHistogram->GetOutputPort());

	m_activeHistogramActor->SetMapper(glyphMapper);
	m_activeHistogramActor->GetMapper()->ScalarVisibilityOn();
	m_activeHistogramActor->GetMapper()->SetScalarModeToUsePointFieldData();
	m_activeHistogramActor->GetMapper()->SelectColorArray("colors");
	m_activeHistogramActor->Modified();

}

//!Calculates the point on the circle for a specific Axis.
//! The circle with the given radius and centerPos gets equally divided and the position is return in pointOnCircle
void iAVRDistributionVis::calculateAxisPositionInCircle(int axis, int numberOfAxes, double *centerPos, double radius, double *pointOnCircle)
{
	double angleSteps = (2.0 * vtkMath::Pi()) / double(numberOfAxes); // 360° = 2*PI
	double currentAngle = axis * angleSteps;

	pointOnCircle[0] = centerPos[0] + (radius * cos(currentAngle));
	pointOnCircle[1] = centerPos[1];
	pointOnCircle[2] = centerPos[2] + (radius * sin(currentAngle));
}

void iAVRDistributionVis::drawAxes()
{
	vtkSmartPointer<vtkAppendPolyData> appendFilter =	vtkSmartPointer<vtkAppendPolyData>::New();

	for (int axisPoly = 0; axisPoly < m_axesPoly->size(); axisPoly++)
	{
		appendFilter->AddInputData(m_axesPoly->at(axisPoly));

		appendFilter->AddInputData(m_axesMarksPoly->at(axisPoly).at(0)); //xMarks
		appendFilter->AddInputData(m_axesMarksPoly->at(axisPoly).at(1)); //yMarks
	}

	appendFilter->Update();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> axesMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	axesMapper->SetInputConnection(appendFilter->GetOutputPort());

	m_axisActor->SetMapper(axesMapper);
	m_axisActor->SetPickable(false);
	m_axisActor->GetProperty()->SetLineWidth(3);
	m_axisActor->GetProperty()->SetColor(0.4, 0.4, 0.4);
}

//! Calculates the 3 Points and their two lines which create a X and Y Axis and saves thm in a vector
//! pos1 defines the start of the line, pos2 the end. The Y Axis starts at po2 and extends in the y direction in the size of the radius
//! Stores the 3 points in an Vector and initializes a additional vector for the later added marks.
void iAVRDistributionVis::calculateAxis(double* pos1, double* pos2)
{
	m_axesPoly->push_back(vtkSmartPointer<vtkPolyData>::New());
	m_axesMarksPoly->push_back(std::vector<vtkSmartPointer<vtkPolyData>>());
	vtkSmartPointer<vtkPoints> axesPoints = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> axesLines = vtkSmartPointer<vtkCellArray>::New();

	axesPoints->InsertNextPoint(pos1);
	axesPoints->InsertNextPoint(pos2);
	axesPoints->InsertNextPoint(pos2[0], pos2[1] + m_radius, pos2[2]); // y Axis

	auto l = vtkSmartPointer<vtkLine>::New();
	//x Axis
	l->GetPointIds()->SetId(0, 0);
	l->GetPointIds()->SetId(1, 1);
	axesLines->InsertNextCell(l);
	//y Axis
	l->GetPointIds()->SetId(0, 1);
	l->GetPointIds()->SetId(1, 2);
	axesLines->InsertNextCell(l);

	m_axesPoly->back()->SetPoints(axesPoints);
	m_axesPoly->back()->SetLines(axesLines);
}

//! Calculates and stores the new center position of an axes by adding an offset
void iAVRDistributionVis::calculateCenterOffsetPos(double* pos1, double* pos2, double* newPos)
{
	double direction[3];
	direction[0] = pos2[0] - pos1[0];
	direction[1] = pos2[1] - pos1[1];
	direction[2] = pos2[2] - pos1[2];

	iAVec3d normDir = iAVec3d(direction);
	normDir.normalize();
	normDir = normDir * m_offsetFromCenter;

	newPos[0] = pos1[0] + normDir[0];
	newPos[1] = pos1[1] + normDir[1];
	newPos[2] = pos1[2] + normDir[2];
}

//! Creates as many Marks on the axis as defined by m_numberOfXBins and m_numberOfYBins
//! The Marks have 2% of the length of the axis
void iAVRDistributionVis::createAxisMarks(int axis)
{
	iAVec3d pos1 = iAVec3d(m_axesPoly->at(axis)->GetPoint(0));
	iAVec3d pos2 = iAVec3d(m_axesPoly->at(axis)->GetPoint(1));
	iAVec3d pos3 = iAVec3d(m_axesPoly->at(axis)->GetPoint(2));

	//Divide Range by bins
	iAVec3d normDirX = pos2 - pos1;
	double rangeX = normDirX.length();
	normDirX.normalize();
	double stepX = rangeX / m_numberOfXBins;

	iAVec3d normDirY = pos3 - pos2;
	double rangeY = normDirY.length();
	normDirY.normalize();
	double stepY = rangeY / m_numberOfYBins;

	vtkSmartPointer<vtkPoints> markPoints = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> markLines = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkPolyData> markPoly = vtkSmartPointer<vtkPolyData>::New();
	double markLengthX = rangeX * 0.02;
	vtkIdType pointID = 0;

	for (int x = 0; x < m_numberOfXBins; x++)
	{
		iAVec3d move = normDirX * (stepX * x);
		iAVec3d mark = pos1 + move;

		markPoints->InsertNextPoint(mark[0], mark[1] + markLengthX, mark[2]); // upperPoint
		markPoints->InsertNextPoint(mark[0], mark[1] - markLengthX, mark[2]); // lowerPoint

		auto l = vtkSmartPointer<vtkLine>::New();
		//x Axis
		l->GetPointIds()->SetId(0, pointID);
		l->GetPointIds()->SetId(1, pointID+1);
		markLines->InsertNextCell(l);
		pointID += 2;
	}
	//Store X
	markPoly->SetPoints(markPoints);
	markPoly->SetLines(markLines);
	m_axesMarksPoly->at(axis).push_back(markPoly);

	markPoints = vtkSmartPointer<vtkPoints>::New();
	markLines = vtkSmartPointer<vtkCellArray>::New();
	markPoly = vtkSmartPointer<vtkPolyData>::New();
	double markLengthY = rangeY * 0.02;
	pointID = 0;

	for (int y = 0; y < m_numberOfYBins; y++)
	{
		iAVec3d move = normDirY * (stepY *y);
		iAVec3d mark = pos2 + move;

		markPoints->InsertNextPoint(mark[0] - markLengthY, mark[1], mark[2]); // leftPoint
		markPoints->InsertNextPoint(mark[0] + markLengthY, mark[1], mark[2]); // rightPoint

		auto l = vtkSmartPointer<vtkLine>::New();
		//y Axis
		l->GetPointIds()->SetId(0, pointID);
		l->GetPointIds()->SetId(1, pointID+1);
		markLines->InsertNextCell(l);
		pointID += 2;
	}

	//Store Y
	markPoly->SetPoints(markPoints);
	markPoly->SetLines(markLines);
	m_axesMarksPoly->at(axis).push_back(markPoly);
}

void iAVRDistributionVis::createAxisLabels(int axis)
{
	//m_axisLabelActor->clear();
	m_axisLabelActor->push_back(std::vector<std::vector<iAVR3DText>>());

	double offsetXAxis = 9.0;
	double offsetYAxis = 8.0;

	//X Dir
	m_axisLabelActor->at(axis).push_back(std::vector<iAVR3DText>());
	for (vtkIdType point = 1; point < m_axesMarksPoly->at(axis).at(0)->GetNumberOfPoints(); point+=2)
	{
		double* pos = m_axesMarksPoly->at(axis).at(0)->GetPoint(point);
		pos[1] -= offsetXAxis; //Add offset to y
		auto tempText = iAVR3DText(m_renderer);
		tempText.createSmall3DLabel(QString("Px%1").arg(point));
		tempText.setLabelPos(pos);
		
		m_axisLabelActor->at(axis).at(0).push_back(tempText);
	}


	//X Dir
	m_axisLabelActor->at(axis).push_back(std::vector<iAVR3DText>());
	for (vtkIdType point = 1; point < m_axesMarksPoly->at(axis).at(1)->GetNumberOfPoints(); point += 2)
	{
		double* pos = m_axesMarksPoly->at(axis).at(1)->GetPoint(point);
		pos[0] += offsetYAxis; //Add offset to x
		auto tempText = iAVR3DText(m_renderer);
		tempText.createSmall3DLabel(QString("Py%1").arg(point));
		tempText.setLabelPos(pos);

		m_axisLabelActor->at(axis).at(1).push_back(tempText);
	}

}

//! Calculates with the 3 points of the two axes, the normal of the plane which they create
//! The normal is normalized!
void iAVRDistributionVis::calculateAxesViewDir(int axis)
{
	//iAVec3d pos1 = iAVec3d(m_axesPoly->at(axis)->GetPoint(0));
	//iAVec3d pos2 = iAVec3d(m_axesPoly->at(axis)->GetPoint(1)); // origin
	//iAVec3d pos3 = iAVec3d(m_axesPoly->at(axis)->GetPoint(2));
	//
	////Calculate Plane Normal (origin - point 1,3)
	//iAVec3d planeVec1 = pos1 - pos2;
	//iAVec3d planeVec2 = pos3 - pos2;

	//iAVec3d normal = crossProduct(planeVec1, planeVec2);
	//normal.normalize();

	//m_AxesViewDir->insert(std::make_pair(axis, normal));

	//Calc middle point of plane
	iAVec3d pos1 = iAVec3d(m_axesPoly->at(axis)->GetPoint(0));
	iAVec3d pos2 = iAVec3d(m_axesPoly->at(axis)->GetPoint(1)); // origin
	iAVec3d pos3 = iAVec3d(m_axesPoly->at(axis)->GetPoint(2));

	iAVec3d planeVec1 = pos2 - pos1;
	iAVec3d planeVec2 = pos2 - pos3;
	iAVec3d middlePoint = iAVec3d(pos1[0] + (planeVec1[0]/2), pos2[1] + (planeVec2[1] / 2), pos1[2] + (planeVec1[2]/2));
	m_AxesViewDir->insert(std::make_pair(axis, middlePoint));
}

void iAVRDistributionVis::calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor)
{
	double* p = markPos; // m_axesMarksPoly->at(axis).at(0)->GetPoint(point);
	barPoints->InsertNextPoint(p[0], p[1] + (cubeSize[1] / 2), p[2]);

	colorArray->InsertNextTuple4(barColor.red(), barColor.green(), barColor.blue(), barColor.alpha());

	double nextCubeHeight = 0;
	for (int y = 0; y < stackSize; y++)
	{
		nextCubeHeight += cubeSize[1];
		barPoints->InsertNextPoint(p[0], p[1] + nextCubeHeight, p[2]);

		colorArray->InsertNextTuple4(barColor.red(), barColor.green(), barColor.blue(), barColor.alpha());
	}
}
