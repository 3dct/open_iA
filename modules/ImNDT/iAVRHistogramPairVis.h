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
	//! Determine which histogram is currently viewed by comparing the focal point and the direction of the histogram (plane)
	void determineHistogramInView(double* viewDir);
	//! Rotates the whole Visualization (and its labels) around the y axis
	void rotateVisualization(double y);
	//! Flips the histogram ordered in an half circle in the given direction
	void flipThroughHistograms(double flipDir);
	double getRadius() const;
	std::vector<QColor> getBarColors() const;

private:
	vtkRenderer* m_renderer;
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
	//! Stores for an [axis] the bars (points) for the histogram
	std::vector<vtkSmartPointer<vtkPolyData>> m_histogramBars;
	//! Stores for an [axis] and a [direction] (x,y) the different 3D TextLabels of the axis pair
	std::vector<std::vector<std::vector<iAVR3DText>>> m_axisLabelActor;
	//! Stores for an [axis] its title
	std::vector<iAVR3DText> m_axisTitleActor;
	//! Stores the [axis] polydata with the 3 points which create a X axis and the y axis
	std::vector<vtkSmartPointer<vtkPolyData>> m_axesPoly;
	//! Stores for every [axis] and [direction] (x,y) the polydata with 2 points for each mark on an axis
	std::vector<std::vector<vtkSmartPointer<vtkPolyData>>> m_axesMarksPoly;
	//! Stores for each Axis its view direction
	std::unordered_map<vtkIdType, iAVec3d> m_axesViewDir;
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
	//! Adds all actors (active and inactive) into one Assembly with the sphere as root
	void mergeActors();
	//! Show the axis marks depending on the view direction
	//! Hide the last shown axis marks
	void showHistogramInView();
	//! Calculates the point on the half circle for a specific Axis.
	//! The half circle with the given radius and centerPos gets equally divided and the position is return in pointOnCircle
	void calculateAxisPositionInCircle(double axis, double numberOfAxes, double centerPos[3], double radius, double pointOnCircle[3]);
	//! Appends all Axes and its Marks to an active and an inactive Actor which can then be rendered (depends on focal point)
	void drawAxes(int visibleAxis);
	void drawHistogram(int visibleAxis);
	void createHistogramMapper(vtkSmartPointer<vtkGlyph3DMapper> glyphMapper);
	void calculateHistogram(size_t axis);
	//! Calculates the 3 Points and their two lines which create a X and Y Axis and saves thm in a vector
	//! pos1 defines the start of the line, pos2 the end. The Y Axis starts at po2 and extends in the y direction in the size of the radius
	//! Stores the 3 points in an Vector
	void calculateAxis(double pos1[3], double pos2[3]);
	//! Calculates and stores the new center position of an axes by adding an offset
	void calculateCenterOffsetPos(double pos1[3], double pos2[3], double newPos[3]);
	//! Calculates the length between the startPoint (with offset) to the second point on the circle
	double calculateAxisLength(double pos1[3], double radius);
	//! Creates as many Marks on the axis as defined by m_numberOfXBins and m_numberOfYBins
	//! Y Range has m_numberOfYBins without the zero mark
	//! The Marks have 2% of the length of the axis
	void createAxisMarks(size_t axis);
	void createAxisLabels(size_t axis);
	//! Calculates with the 3 points of the two axes, the middle point of the plane which they create
	void calculateAxesViewPoint(size_t axis);
	void calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor);
	//! Calculates the cube (x,z) size based on the axis length and number of x bins
	//! X and Z are the same
	double getXZCubeSize() const;
	int getMaxBinOccurrences(size_t axis);
	//! Calculates the smallest y size by dividing the y axis length by the max occurences of that feature
	double getMinYCubeSize(size_t axis);
	//! Calculates the shifted coordinates based on the vector between two points
	//! The point p2 is shifted in the direction of p1
	iAVec3d applyShiftToVector(double point1[3], double point2[3], double shift[3]);
	//! helper function applying a transform, to avoid code duplication
	void applyTransform(vtkTransform* transform, size_t i);
};
