// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include "iAVR3DText.h"

#include <iAVec3.h>

#include <QColor>

#include <unordered_map>

class vtkActor;
class vtkAssembly;
class vtkGlyph3DMapper;
class vtkPoints;
class vtkPolyData;
class vtkRenderer;
class vtkTable;
class vtkUnsignedCharArray;

class iAVRHistogram;
class iAVRHistogramMetric;
class iAVROctreeMetrics;

//! Creates a 3D Distribution Visualization of the volume in the VR Environment
class iAVRHistogramPairVis
{
public:
	iAVRHistogramPairVis(vtkRenderer* ren, iAVRHistogramMetric* histogramMetric, iAVROctreeMetrics* octreeMetric, vtkTable* objectTable);
	void createVisualization(double* pos, double visSize, double offset, int level, std::vector<vtkIdType> const & regions, std::vector<int> const & featureList);
	vtkSmartPointer<vtkAssembly> getVisAssembly();
	void show();
	void hide();
	void determineHistogramInView(double* viewDir);
	void rotateVisualization(double y);
	void flipThroughHistograms(double flipDir);
	double getRadius() const;
	std::vector<QColor> getBarColors() const;

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	iAVRHistogramMetric* m_histogramMetric;
	iAVROctreeMetrics* m_octreeMetric;
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
	void calculateAxisPositionInCircle(double axis, double numberOfAxes, double centerPos[3], double radius, double pointOnCircle[3]);
	void drawAxes(int visibleAxis);
	void drawHistogram(int visibleAxis);
	void createHistogramMapper(vtkSmartPointer<vtkGlyph3DMapper> glyphMapper);
	void calculateHistogram(size_t axis);
	void calculateAxis(double pos1[3], double pos2[3]);
	void calculateCenterOffsetPos(double pos1[3], double pos2[3], double newPos[3]);
	double calculateAxisLength(double pos1[3], double radius);
	void createAxisMarks(size_t axis);
	void createAxisLabels(size_t axis);
	void calculateAxesViewPoint(size_t axis);
	void calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor);
	double getXZCubeSize() const;
	int getMaxBinOccurrences(size_t axis);
	double getMinYCubeSize(size_t axis);

	iAVec3d applyShiftToVector(double point1[3], double point2[3], double shift[3]);
};
