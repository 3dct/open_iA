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
#include <vtkGlyph3DMapper.h>
#include <vtkAssembly.h>

#include "iACsvIO.h"
#include "iAVR3DText.h"
#include "iAVRHistogramMetric.h"
#include "iAVROctreeMetrics.h"

#include <unordered_map>
#include <QColor>

//! Creates a 3D Distribution Visualization of the volume in the VR Environment
class iAVRHistogramPairVis
{
public:
	iAVRHistogramPairVis(vtkRenderer* ren, iAVRHistogramMetric* histogramMetric, iAVROctreeMetrics* octreeMetric, vtkTable* objectTable, iACsvIO io);
	void createVisualization(double* pos, double visSize, double offset, int level, std::vector<vtkIdType>* regions, std::vector<int>* featureList);
	vtkSmartPointer<vtkAssembly> getVisAssembly();
	void show();
	void hide();
	void determineHistogramInView(double* viewDir);
	void rotateVisualization(double y);
	void flipThroughHistograms(double flipDir);
	double getRadius();
	std::vector<QColor> getBarColors();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkTable> m_objectTable;
	vtkSmartPointer<vtkActor> m_sphereActor;
	vtkSmartPointer<vtkActor> m_activeHistogramActor;
	vtkSmartPointer<vtkActor> m_inactiveHistogramActor;
	vtkSmartPointer<vtkActor> m_activeAxisActor;
	vtkSmartPointer<vtkActor> m_inactiveAxisActor;
	vtkSmartPointer<vtkAssembly> visualizationActor;
	vtkSmartPointer<vtkActor> outlineActor;
	//Stores for an [axis] the bars (points) for the histogram
	std::vector<vtkSmartPointer<vtkPolyData>>* m_histogramBars;
	//Stores for an [axis] and a [direction] (x,y) the different 3D TextLabels of the axis pair
	std::vector<std::vector<std::vector<iAVR3DText>>>* m_axisLabelActor;
	//Stores for an [axis] its title
	std::vector<iAVR3DText>* m_axisTitleActor;
	iACsvIO m_io;
	iAVROctreeMetrics* m_octreeMetric;
	iAVRHistogramMetric* m_histogramMetric;
	//Stores the [axis] polydata with the 3 points which create a X axis and the y axis
	std::vector<vtkSmartPointer<vtkPolyData>>* m_axesPoly;
	//Stores for every [axis] and [direction] (x,y) the polydata with 2 points for each mark on an axis
	std::vector<std::vector<vtkSmartPointer<vtkPolyData>>>* m_axesMarksPoly;
	//Stores for each Axis its view direction
	std::unordered_map<vtkIdType, iAVec3d>* m_AxesViewDir;
	bool m_visible;
	int m_numberOfXBins;
	int m_numberOfYBins;
	double m_centerOfVis[3];
	double m_offsetFromCenter;
	double m_radius;
	double m_axisLength;
	double axisAngle; //in RAD
	int currentlyShownAxis;
	int m_axisInView;
	//[0] Left (0°), [1] Right (180°) Axis
	int m_frontAxes[2];

	//Stores for every [feature] a histogram
	std::vector<iAVRHistogram> m_histogram01;
	std::vector<iAVRHistogram> m_histogram02;
	QColor barColorR1;
	QColor barColorR2;

	void initialize();
	void mergeActors();
	void showHistogramInView();
	void calculateAxisPositionInCircle(int axis, int numberOfAxes, double centerPos[3], double radius, double pointOnCircle[3]);
	void drawAxes(int visibleAxis);
	void drawHistogram(int visibleAxis);
	void createHistogramMapper(vtkSmartPointer<vtkGlyph3DMapper> glyphMapper);
	void calculateHistogram(int axis);
	void calculateAxis(double pos1[3], double pos2[3]);
	void calculateCenterOffsetPos(double pos1[3], double pos2[3], double newPos[3]);
	double calculateAxisLength(double pos1[3], double radius);
	void createAxisMarks(int axis);
	void createAxisLabels(int axis);
	void calculateAxesViewPoint(int axis);
	void calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor);
	double getXZCubeSize();
	int getMaxBinOccurrences(int axis);
	double getMinYCubeSize(int axis);

	iAVec3d applyShiftToVector(double point1[3], double point2[3], double shift[3]);
};
