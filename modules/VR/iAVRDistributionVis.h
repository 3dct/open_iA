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
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include "vtkTable.h"
#include <vtkAxisActor.h>
#include <vtkGlyph3D.h>

#include "iACsvIO.h"
#include "iAVR3DText.h"
#include "iAVRMetrics.h"

#include <unordered_map>
#include <QColor>

//! Creates a 3D Distribution Visualization of the volume in the VR Environment
class iAVRDistributionVis
{
public:
	iAVRDistributionVis(vtkRenderer* ren, iAVRMetrics* fiberMetric, vtkTable* objectTable, iACsvIO io);
	void createVisualization(double *pos, int level, std::vector<vtkIdType>* regions, std::vector<int>* featureList);
	void show();
	void showAxisMarksInView(double* viewDir);
	void hide();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkTable> m_objectTable;
	vtkSmartPointer<vtkActor> m_sphereActor;
	vtkSmartPointer<vtkActor> m_activeHistogramActor;
	vtkSmartPointer<vtkActor> m_inactiveHistogramActor;
	vtkSmartPointer<vtkActor> m_axisActor;
	//Stores for an [axis] the glyphs (cubes) for the histogram
	std::vector<vtkSmartPointer<vtkGlyph3D>>* m_histogramGlyphs;
	//Stores for an [axis] and a [direction] (x,y) the different 3D TextLabels of the axis pair
	std::vector<std::vector<std::vector<iAVR3DText>>>* m_axisLabelActor;
	//Stores for an [axis] its title
	std::vector<iAVR3DText>* m_axisTitleActor;
	iACsvIO m_io;
	iAVRMetrics* m_fiberMetric;
	//Stores the [axis] polydata with the 3 points which create a X axis and the y axis
	std::vector<vtkSmartPointer<vtkPolyData>>* m_axesPoly;
	//Stores for every [axis] and [direction] (x,y) the polydata with 2 points for each mark on an axis
	std::vector<std::vector<vtkSmartPointer<vtkPolyData>>>* m_axesMarksPoly;
	//Stores for each Axis its view direction
	std::unordered_map<vtkIdType, iAVec3d>* m_AxesViewDir;
	bool m_visible;
	int m_numberOfXBins;
	int m_numberOfYBins;
	double m_offsetFromCenter;
	double m_radius;
	double m_axisLength;
	double axisAngle;
	int axisInView;
	double binY;
	HistogramParameters* m_histogramParameter;

	void calculateAxisPositionInCircle(int axis, int numberOfAxes, double *centerPos, double radius, double* pointOnCircle);
	void drawAxes();
	void drawHistogram();
	void calculateHistogram(int axis);
	void calculateAxis(double *pos1, double *pos2);
	void calculateCenterOffsetPos(double* pos1, double* pos2, double* newPos);
	double calculateAxisLength(double* pos1, double radius);
	void createAxisMarks(int axis);
	void createAxisLabels(int axis);
	void calculateAxesViewDir(int axis);
	void calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor);
	void calculateFittingCubeSize(double* cubeSize);

	iAVec3d applyShiftToVector(double point1[3], double point2[3], double shift[3]);
};