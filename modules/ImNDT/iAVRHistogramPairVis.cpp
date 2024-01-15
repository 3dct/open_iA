// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRHistogramPairVis.h"

#include "iAVRHistogramMetric.h"
#include "iAVROctreeMetrics.h"

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkAssembly.h>
#include <vtkCellData.h>
#include <vtkCubeSource.h>
#include <vtkDoubleArray.h>
#include <vtkGlyph3DMapper.h>
#include <vtkLine.h>
#include <vtkMapper.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkProp3DCollection.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTextActor3D.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransformFilter.h>
#include <vtkUnsignedCharArray.h>

#include <iALog.h>
#include <iAVec3.h>

iAVRHistogramPairVis::iAVRHistogramPairVis(vtkRenderer* ren, iAVRHistogramMetric* histogramMetric, iAVROctreeMetrics* octreeMetric, vtkTable* objectTable) :m_renderer(ren), 
m_histogramMetric(histogramMetric), m_octreeMetric(octreeMetric), m_objectTable(objectTable), m_sphereActor(vtkSmartPointer<vtkActor>::New())
{
	initialize();
}

void iAVRHistogramPairVis::initialize()
{
	m_visible = false;
	m_axesPoly = new std::vector<vtkSmartPointer<vtkPolyData>>();
	m_axesMarksPoly = new std::vector<std::vector<vtkSmartPointer<vtkPolyData>>>();
	m_activeAxisActor = vtkSmartPointer<vtkActor>::New();
	m_inactiveAxisActor = vtkSmartPointer<vtkActor>::New();
	m_axisLabelActor = new std::vector<std::vector<std::vector<iAVR3DText>>>();
	m_axisTitleActor = new std::vector<iAVR3DText>();
	m_AxesViewDir = new std::unordered_map<vtkIdType, iAVec3d>();
	m_activeHistogramActor = vtkSmartPointer<vtkActor>::New();
	m_inactiveHistogramActor = vtkSmartPointer<vtkActor>::New();
	m_histogramBars = new std::vector<vtkSmartPointer<vtkPolyData>>();
	visualizationActor = vtkSmartPointer<vtkAssembly>::New();
	outlineActor = vtkSmartPointer<vtkActor>::New();

	m_radius = 400;
	m_numberOfXBins = 6;
	m_numberOfYBins = 15;
	m_offsetFromCenter = 70;
	m_axisInView = -1;
	currentlyShownAxis = -1;
	m_axisLength = -1;

	//https://colorbrewer2.org/#type=diverging&scheme=PiYG&n=4
	//barColorR1 = QColor(85, 217, 73, 255);
	//barColorR2 = QColor(217, 73, 157, 255);
	barColorR1 = QColor(77, 172, 38);
	barColorR2 = QColor(208, 28, 139);
}

void iAVRHistogramPairVis::createVisualization(double* pos, double visSize, double offset, int level, std::vector<vtkIdType>* regions, std::vector<int> const& featureList)
{
	if (regions->size() < 2)
	{
		LOG(lvlDebug, "Too few regions selected!");
		return;
	}
	//Clean Up and start fresh
	initialize();

	//Axes which are at 0° and 180°
	m_frontAxes[0] = 0;
	m_frontAxes[1] = static_cast<int>(featureList.size() - 1);

	//Copy only values from position pointer
	m_centerOfVis[0] = pos[0];
	m_centerOfVis[1] = pos[1];
	m_centerOfVis[2] = pos[2];

	m_radius = visSize + offset;
	m_offsetFromCenter = offset + (m_radius / offset);

	//LOG(lvlDebug,QString("m_radius = %1").arg(m_radius));
	//LOG(lvlDebug,QString("m_offsetFromCenter = %1").arg(m_offsetFromCenter));

	// Create a circle
	vtkSmartPointer<vtkRegularPolygonSource> polygonSource = vtkSmartPointer<vtkRegularPolygonSource>::New();
	//polygonSource->GeneratePolygonOff(); // Uncomment this line to generate only the outline of the circle
	polygonSource->SetNumberOfSides(36);
	polygonSource->SetRadius(m_radius);
	polygonSource->SetCenter(pos);
	polygonSource->SetNormal(0, 1, 0);

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(polygonSource->GetOutputPort());;

	m_sphereActor = vtkSmartPointer<vtkActor>::New();
	m_sphereActor->SetMapper(mapper);
	m_sphereActor->GetProperty()->SetOpacity(0.05);
	m_sphereActor->GetProperty()->SetLineWidth(2.8);

	//Create Bounding Sphere (for interaction)
	vtkSmartPointer<vtkSphereSource> boundingSphere = vtkSmartPointer<vtkSphereSource>::New();
	boundingSphere->SetCenter(m_centerOfVis);
	boundingSphere->SetRadius(m_radius);

	vtkSmartPointer<vtkPolyDataMapper> outlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	outlineMapper->SetInputConnection(boundingSphere->GetOutputPort());
	outlineActor->SetMapper(outlineMapper);
	outlineActor->GetProperty()->SetOpacity(0.0001);

	m_axesPoly->clear();
	m_axesMarksPoly->clear();
	m_axisLabelActor->clear();
	m_AxesViewDir->clear();
	m_histogramBars->clear();

	//Calculate Histogram Values and Bins for every feature
	for(int feature : featureList)
	{
		//LOG(lvlImportant, QString("\n Feature: %1 (%2)").arg(feature).arg(m_octreeMetric->getFeatureName(feature)));

		auto val01 = m_octreeMetric->getRegionValues(level, regions->at(0), feature);
		auto val02 = m_octreeMetric->getRegionValues(level, regions->at(1), feature);
		auto minMax = m_octreeMetric->getMinMaxFromVec(val01, val02);

		m_histogram01.push_back(m_histogramMetric->getHistogram(val01, minMax.at(0), minMax.at(1), m_octreeMetric->getMaxNumberOfFibersInRegion(level)));
		m_histogram02.push_back(m_histogramMetric->getHistogram(val02, minMax.at(0), minMax.at(1), m_octreeMetric->getMaxNumberOfFibersInRegion(level)));
		//Set IDs
		m_histogramMetric->setHistogramFeatureID(&m_histogram01.back(), feature);
		m_histogramMetric->setHistogramFeatureID(&m_histogram02.back(), feature);
	}
	m_numberOfXBins = m_histogram01.at(0).m_histogramParameters.bins;
	m_numberOfYBins = ceil(m_numberOfXBins / 2);

	//Lenght of the axes with offset from center
	m_axisLength = calculateAxisLength(pos, m_radius);

	//start from new centerPos
	double newCenterPos[3]{};

	for (size_t i = 0; i < featureList.size(); i++)
	{
		double posOnCircle[3]{};
		calculateAxisPositionInCircle(i, static_cast<double>(featureList.size()) - 1, pos, m_radius, posOnCircle);
		calculateCenterOffsetPos(pos, posOnCircle, newCenterPos);
		calculateAxis(newCenterPos, posOnCircle);
		createAxisMarks(i);
		createAxisLabels(i);
		calculateAxesViewPoint(i);
		calculateHistogram(i);
	}
}

vtkSmartPointer<vtkAssembly> iAVRHistogramPairVis::getVisAssembly()
{
	return visualizationActor;
}

void iAVRHistogramPairVis::show()
{
	if (m_visible)
	{
		return;
	}

	mergeActors();
	m_renderer->AddActor(visualizationActor);

	m_visible = true;
}

void iAVRHistogramPairVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(visualizationActor);

	for (size_t i = 0; i < m_axisLabelActor->size(); i++)
	{
		m_axisTitleActor->at(i).hide();

		for (size_t j = 0; j < m_axisLabelActor->at(i).size(); j++)
		{
			for (size_t k = 0; k < m_axisLabelActor->at(i).at(j).size(); k++)
			{
				m_axisLabelActor->at(i).at(j).at(k).hide();
			}
		}
	}

	m_axisInView = -1;
	m_visible = false;
}

//! Determine which histogram is currently viewed by comparing the focal point and the direction of the histogram (plane)
void iAVRHistogramPairVis::determineHistogramInView(double* viewDir)
{
	if (m_visible)
	{
		double minDistance = std::numeric_limits<double>::infinity();
		int newAxisInView = -1;

		for (size_t i = 0; i < m_AxesViewDir->size(); i++)
		{
			auto focalP = iAVec3d(viewDir[0], viewDir[1], viewDir[2]);
			auto ray = (m_AxesViewDir->at(i) - (focalP));
			auto tempDistance = ray.length();

			if (tempDistance < minDistance)
			{
				minDistance = tempDistance;
				newAxisInView = static_cast<int>(i);
			}
		}

		if (newAxisInView != -1)
		{
			m_axisInView = newAxisInView;
			showHistogramInView();
		}
	}
}

//! Rotates the whole Visualization (and its labels) around the y axis
void iAVRHistogramPairVis::rotateVisualization(double y)
{
	if (m_visible)
	{
		vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
		transform->PostMultiply(); //this is the key line
		transform->Translate(-m_centerOfVis[0], -m_centerOfVis[1], -m_centerOfVis[2]);
		transform->RotateY(-y);
		transform->Translate(m_centerOfVis[0], m_centerOfVis[1], m_centerOfVis[2]);

		for (size_t i = 0; i < m_axesPoly->size(); i++)
		{
			vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();

			m_AxesViewDir->at(i) = iAVec3d(transform->TransformPoint(m_AxesViewDir->at(i).data()));

			transformFilter->SetInputData(m_axesPoly->at(i));
			transformFilter->SetTransform(transform);
			transformFilter->Update();
			m_axesPoly->at(i) = transformFilter->GetOutput();

			for (size_t d = 0; d < m_axesMarksPoly->at(i).size(); d++)
			{
				transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
				transformFilter->SetInputData(m_axesMarksPoly->at(i).at(d));
				transformFilter->SetTransform(transform);
				transformFilter->Update();
				m_axesMarksPoly->at(i).at(d) = transformFilter->GetOutput();
			}

			transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInputData(m_histogramBars->at(i));
			transformFilter->SetTransform(transform);
			transformFilter->Update();
			m_histogramBars->at(i) = transformFilter->GetOutput();

			auto currentTitleLabel = m_axisTitleActor->at(i).getTextActor();
			currentTitleLabel->SetPosition(transform->TransformPoint(currentTitleLabel->GetPosition()));
			currentTitleLabel->Modified();

			for (size_t k = 0; k < m_axisLabelActor->at(i).size(); k++)
			{
				for (size_t j = 0; j < m_axisLabelActor->at(i).at(k).size(); j++)
				{
					auto currentLabel = m_axisLabelActor->at(i).at(k).at(j).getTextActor();
					currentLabel->SetPosition(transform->TransformPoint(currentLabel->GetPosition()));
					currentLabel->Modified();
				}
			}
		}

		drawAxes(m_axisInView);
		drawHistogram(m_axisInView);
	}
}

//! Flips the histogram ordered in an half circle in the given direction
void iAVRHistogramPairVis::flipThroughHistograms(double flipDir)
{
	int currentFlip = -1;
	//left flip
	if (flipDir > 0) currentFlip = 0;
	else currentFlip = 1;

	vtkSmartPointer<vtkTransform> transformFront = vtkSmartPointer<vtkTransform>::New();
	transformFront->PostMultiply(); //this is the key line
	transformFront->Translate(-m_centerOfVis[0], -m_centerOfVis[1], -m_centerOfVis[2]);
	transformFront->RotateY(180 * flipDir);
	transformFront->Translate(m_centerOfVis[0], m_centerOfVis[1], m_centerOfVis[2]);

	vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
	transform->PostMultiply(); //this is the key line
	transform->Translate(-m_centerOfVis[0], -m_centerOfVis[1], -m_centerOfVis[2]);
	transform->RotateY(vtkMath::DegreesFromRadians(axisAngle) * flipDir);
	transform->Translate(m_centerOfVis[0], m_centerOfVis[1], m_centerOfVis[2]);

	for (size_t i = 0; i < m_axesPoly->size(); i++)
	{
		vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();

		if (static_cast<int>(i) == m_frontAxes[currentFlip]) //The axis with 0 or 180 degree angle
		{
			m_AxesViewDir->at(i) = iAVec3d(transformFront->TransformPoint(m_AxesViewDir->at(i).data()));

			transformFilter->SetInputData(m_axesPoly->at(i));
			transformFilter->SetTransform(transformFront);
			transformFilter->Update();
			m_axesPoly->at(i) = transformFilter->GetOutput();

			for (size_t d = 0; d < m_axesMarksPoly->at(i).size(); d++)
			{
				transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
				transformFilter->SetInputData(m_axesMarksPoly->at(i).at(d));
				transformFilter->SetTransform(transformFront);
				transformFilter->Update();
				m_axesMarksPoly->at(i).at(d) = transformFilter->GetOutput();
			}

			transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInputData(m_histogramBars->at(i));
			transformFilter->SetTransform(transformFront);
			transformFilter->Update();
			m_histogramBars->at(i) = transformFilter->GetOutput();

			auto currentTitleLabel = m_axisTitleActor->at(i).getTextActor();
			currentTitleLabel->SetPosition(transformFront->TransformPoint(currentTitleLabel->GetPosition()));
			currentTitleLabel->Modified();

			for (size_t k = 0; k < m_axisLabelActor->at(i).size(); k++)
			{
				for (size_t j = 0; j < m_axisLabelActor->at(i).at(k).size(); j++)
				{
					auto currentLabel = m_axisLabelActor->at(i).at(k).at(j).getTextActor();
					currentLabel->SetPosition(transformFront->TransformPoint(currentLabel->GetPosition()));
					currentLabel->Modified();
				}
			}
		}
		else
		{
			m_AxesViewDir->at(i) = iAVec3d(transform->TransformPoint(m_AxesViewDir->at(i).data()));

			transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInputData(m_axesPoly->at(i));
			transformFilter->SetTransform(transform);
			transformFilter->Update();
			m_axesPoly->at(i) = transformFilter->GetOutput();

			for (size_t d = 0; d < m_axesMarksPoly->at(i).size(); d++)
			{
				transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
				transformFilter->SetInputData(m_axesMarksPoly->at(i).at(d));
				transformFilter->SetTransform(transform);
				transformFilter->Update();
				m_axesMarksPoly->at(i).at(d) = transformFilter->GetOutput();
			}

			transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
			transformFilter->SetInputData(m_histogramBars->at(i));
			transformFilter->SetTransform(transform);
			transformFilter->Update();
			m_histogramBars->at(i) = transformFilter->GetOutput();

			auto currentTitleLabel = m_axisTitleActor->at(i).getTextActor();
			currentTitleLabel->SetPosition(transform->TransformPoint(currentTitleLabel->GetPosition()));
			currentTitleLabel->Modified();

			for (size_t k = 0; k < m_axisLabelActor->at(i).size(); k++)
			{
				for (size_t j = 0; j < m_axisLabelActor->at(i).at(k).size(); j++)
				{
					auto currentLabel = m_axisLabelActor->at(i).at(k).at(j).getTextActor();
					currentLabel->SetPosition(transform->TransformPoint(currentLabel->GetPosition()));
					currentLabel->Modified();
				}
			}
		}
	}

	m_frontAxes[0] += flipDir;
	m_frontAxes[1] += flipDir;

	for (int pos = 0; pos < 2; pos++)
	{
		if (m_frontAxes[pos] < 0) m_frontAxes[pos] = static_cast<int>(m_axesPoly->size() - 1);
		if (m_frontAxes[pos] > static_cast<int>(m_axesPoly->size()) - 1) m_frontAxes[pos] = 0;
	}

	drawAxes(m_axisInView);
	drawHistogram(m_axisInView);
}

double iAVRHistogramPairVis::getRadius()
{
	return m_radius;
}

std::vector<QColor> iAVRHistogramPairVis::getBarColors()
{
	std::vector<QColor> barCol = std::vector<QColor>();
	barCol.push_back(barColorR1);
	barCol.push_back(barColorR2);

	return barCol;
}

//! Adds all actors (active and inactive) into one Assembly with the sphere as root
void iAVRHistogramPairVis::mergeActors()
{
	visualizationActor->AddPart(m_sphereActor);
	visualizationActor->AddPart(outlineActor);
	visualizationActor->AddPart(m_activeAxisActor);
	visualizationActor->AddPart(m_inactiveAxisActor);
	visualizationActor->AddPart(m_activeHistogramActor);
	visualizationActor->AddPart(m_inactiveHistogramActor);
	visualizationActor->Modified();
}

//! Show the axis marks depending on the view direction
//! Hide the last shown axis marks
void iAVRHistogramPairVis::showHistogramInView()
{
	if (m_axisInView != -1 && m_axisInView != currentlyShownAxis)
	{
		if (currentlyShownAxis != -1)
		{
			//Delete old
			m_axisTitleActor->at(currentlyShownAxis).hide();
			for (size_t j = 0; j < m_axisLabelActor->at(currentlyShownAxis).size(); j++)
			{
				for (size_t k = 0; k < m_axisLabelActor->at(currentlyShownAxis).at(j).size(); k++)
				{
					m_axisLabelActor->at(currentlyShownAxis).at(j).at(k).hide();
				}
			}
		}

		drawAxes(m_axisInView);
		drawHistogram(m_axisInView);

		//Create new
		m_axisTitleActor->at(m_axisInView).show();
		for (size_t j = 0; j < m_axisLabelActor->at(m_axisInView).size(); j++)
		{
			for (size_t k = 0; k < m_axisLabelActor->at(m_axisInView).at(j).size(); k++)
			{
				m_axisLabelActor->at(m_axisInView).at(j).at(k).show();
			}
		}

		visualizationActor->Modified();
	}
	currentlyShownAxis = m_axisInView;
}

//! Calculates the point on the half circle for a specific Axis.
//! The half circle with the given radius and centerPos gets equally divided and the position is return in pointOnCircle
void iAVRHistogramPairVis::calculateAxisPositionInCircle(double axis, double numberOfAxes, double centerPos[3], double radius, double pointOnCircle[3])
{
	//axisAngle = (2.0 * vtkMath::Pi()) / double(numberOfAxes); // 360° = 2*PI
	axisAngle = (vtkMath::Pi()) / numberOfAxes; // 180° = PI
	double currentAngle = axis * axisAngle;

	pointOnCircle[0] = centerPos[0] + (radius * cos(currentAngle));
	pointOnCircle[1] = centerPos[1];
	pointOnCircle[2] = centerPos[2] + (radius * sin(currentAngle));
}

//! Appends all Axes and its Marks to an active and an inactive Actor which can then be rendered (depends on focal point)
void iAVRHistogramPairVis::drawAxes(int visibleAxis)
{
	if (visibleAxis != -1)
	{
		vtkSmartPointer<vtkAppendPolyData> appendActiveFilter = vtkSmartPointer<vtkAppendPolyData>::New();
		vtkSmartPointer<vtkAppendPolyData> appendInactiveFilter = vtkSmartPointer<vtkAppendPolyData>::New();

		for (size_t axisPoly = 0; axisPoly < m_axesPoly->size(); axisPoly++)
		{
			if (static_cast<int>(axisPoly) == visibleAxis)
			{
				appendActiveFilter->AddInputData(m_axesPoly->at(axisPoly));

				appendActiveFilter->AddInputData(m_axesMarksPoly->at(axisPoly).at(0)); //xMarks
				appendActiveFilter->AddInputData(m_axesMarksPoly->at(axisPoly).at(1)); //yMarks
			}
			else
			{
				appendInactiveFilter->AddInputData(m_axesPoly->at(axisPoly));

				appendInactiveFilter->AddInputData(m_axesMarksPoly->at(axisPoly).at(0)); //xMarks
				appendInactiveFilter->AddInputData(m_axesMarksPoly->at(axisPoly).at(1)); //yMarks
			}
		}

		appendActiveFilter->Update();
		appendInactiveFilter->Update();

		// Create a mapper
		vtkSmartPointer<vtkPolyDataMapper> axesActiveMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		axesActiveMapper->SetInputConnection(appendActiveFilter->GetOutputPort());

		m_activeAxisActor->SetMapper(axesActiveMapper);
		m_activeAxisActor->SetPickable(false);
		m_activeAxisActor->GetProperty()->SetLineWidth(3);
		m_activeAxisActor->GetProperty()->SetColor(0.4, 0.4, 0.4);

		vtkSmartPointer<vtkPolyDataMapper> axesInactiveMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		axesInactiveMapper->SetInputConnection(appendInactiveFilter->GetOutputPort());

		m_inactiveAxisActor->SetMapper(axesInactiveMapper);
		m_inactiveAxisActor->SetPickable(false);
		m_inactiveAxisActor->GetProperty()->SetLineWidth(2);
		m_inactiveAxisActor->GetProperty()->SetColor(0.4, 0.4, 0.4);
		m_inactiveAxisActor->GetProperty()->SetOpacity(0.4);
	}
}

void iAVRHistogramPairVis::drawHistogram(int visibleAxis)
{
	if (visibleAxis != -1)
	{
		vtkSmartPointer<vtkAppendPolyData> appendActiveFilter = vtkSmartPointer<vtkAppendPolyData>::New();
		vtkSmartPointer<vtkAppendPolyData> appendInactiveFilter = vtkSmartPointer<vtkAppendPolyData>::New();

		for (int axis = 0; axis < static_cast<int>(m_histogramBars->size()); axis++)
		{
			if (axis == visibleAxis)
			{
				appendActiveFilter->AddInputData(m_histogramBars->at(axis));
			}
			else
			{
				appendInactiveFilter->AddInputData(m_histogramBars->at(axis));
			}
		}

		appendActiveFilter->Update();
		appendInactiveFilter->Update();

		vtkSmartPointer<vtkGlyph3DMapper> activeHistogramGlyphs = vtkSmartPointer<vtkGlyph3DMapper>::New();
		activeHistogramGlyphs->SetInputData(appendActiveFilter->GetOutput());

		vtkSmartPointer<vtkGlyph3DMapper> inactiveHistogramGlyphs = vtkSmartPointer<vtkGlyph3DMapper>::New();
		inactiveHistogramGlyphs->SetInputData(appendInactiveFilter->GetOutput());

		//Create active and inactive GlyphMappers
		createHistogramMapper(activeHistogramGlyphs);
		createHistogramMapper(inactiveHistogramGlyphs);

		m_activeHistogramActor->SetMapper(activeHistogramGlyphs);
		m_activeHistogramActor->GetMapper()->ScalarVisibilityOn();
		m_activeHistogramActor->GetMapper()->SelectColorArray("colors");

		m_inactiveHistogramActor->SetMapper(inactiveHistogramGlyphs);
		m_inactiveHistogramActor->GetMapper()->ScalarVisibilityOn();
		m_inactiveHistogramActor->GetMapper()->SelectColorArray("colors");
		m_inactiveHistogramActor->GetProperty()->SetOpacity(0.4);

		m_activeHistogramActor->Modified();
		m_inactiveHistogramActor->Modified();
	}
}

void iAVRHistogramPairVis::createHistogramMapper(vtkSmartPointer<vtkGlyph3DMapper> glyphMapper)
{
	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	cubeSource->SetXLength(1);
	cubeSource->SetYLength(1);
	cubeSource->SetZLength(1);

	glyphMapper->SetSourceConnection(cubeSource->GetOutputPort());
	glyphMapper->SetScalarModeToUsePointFieldData();
	glyphMapper->SetScaleArray("scale");
	glyphMapper->SetScaleModeToScaleByVectorComponents();
	glyphMapper->SetOrientationArray("rotate");
	glyphMapper->SetOrientationModeToRotation();
}

void iAVRHistogramPairVis::calculateHistogram(size_t axis)
{
	vtkSmartPointer<vtkPoints> barPoints = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkPolyData> bars = vtkSmartPointer<vtkPolyData>::New();

	vtkSmartPointer<vtkDoubleArray> glyphScale = vtkSmartPointer<vtkDoubleArray>::New();
	glyphScale->SetName("scale");
	glyphScale->SetNumberOfComponents(3);

	vtkSmartPointer<vtkDoubleArray> glyphRotation = vtkSmartPointer<vtkDoubleArray>::New();
	glyphRotation->SetName("rotate");
	glyphRotation->SetNumberOfComponents(3);

	vtkSmartPointer<vtkUnsignedCharArray> glyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	glyphColor->SetName("colors");
	glyphColor->SetNumberOfComponents(4);

	double cubeXZSize = getXZCubeSize(); //Should depend on size between two marks
	double minYBarSize = getMinYCubeSize(axis);
	double rotAngle = -(vtkMath::DegreesFromRadians(axisAngle * static_cast<double>(axis)));

	iAVec3d direction = iAVec3d(0, 0, 0);
	direction = iAVec3d(m_axesPoly->at(axis)->GetPoint(1)) - iAVec3d(m_axesPoly->at(axis)->GetPoint(0));
	direction.normalize();
	//iAVec3d cubeHalfSize = iAVec3d(cubeSize[0] / 2.0 ,cubeSize[1] / 2.0 ,cubeSize[2] / 2.0);
	direction = direction * (cubeXZSize / 2.0);

	int binCount = 0;
	for (vtkIdType point = 0; point < m_axesMarksPoly->at(axis).at(0)->GetNumberOfPoints(); point += 2)
	{
		// Move first cube a little bit to the left and second to the right
		auto markPos = m_axesMarksPoly->at(axis).at(0)->GetPoint(point);

		double markPosShiftedLeft[3]{};
		double markPosShiftedRight[3]{};
		vtkMath::Subtract(markPos, (direction).data(), markPosShiftedRight);
		vtkMath::Add(markPos, (direction).data(), markPosShiftedLeft);
		

		//double ySizeR1 = minYBarSize * (double)(m_histogram->histogramRegion1.at(axis).at(binCount));
		//double ySizeR2 = minYBarSize * (double)(m_histogram->histogramRegion2.at(axis).at(binCount));
		double ySizeR1 = minYBarSize * (double)(m_histogram01.at(axis).m_histogramParameters.observations.at(binCount));
		double ySizeR2 = minYBarSize * (double)(m_histogram02.at(axis).m_histogramParameters.observations.at(binCount));

		barPoints->InsertNextPoint(markPosShiftedLeft[0], markPosShiftedLeft[1] + (ySizeR1 / 2.0), markPosShiftedLeft[2]);
		glyphColor->InsertNextTuple4(barColorR1.red(), barColorR1.green(), barColorR1.blue(), barColorR1.alpha());
		glyphScale->InsertNextTuple3(cubeXZSize, ySizeR1, cubeXZSize);
		glyphRotation->InsertNextTuple3(0, rotAngle, 0);

		barPoints->InsertNextPoint(markPosShiftedRight[0], markPosShiftedRight[1] + (ySizeR2 / 2.0), markPosShiftedRight[2]);
		glyphColor->InsertNextTuple4(barColorR2.red(), barColorR2.green(), barColorR2.blue(), barColorR2.alpha());
		glyphScale->InsertNextTuple3(cubeXZSize, ySizeR2, cubeXZSize);
		glyphRotation->InsertNextTuple3(0, rotAngle, 0);

		binCount++;
	}

	bars->SetPoints(barPoints);
	bars->GetPointData()->AddArray(glyphColor);
	bars->GetPointData()->AddArray(glyphScale);
	bars->GetPointData()->AddArray(glyphRotation);

	m_histogramBars->push_back(bars);
}

//! Calculates the 3 Points and their two lines which create a X and Y Axis and saves thm in a vector
//! pos1 defines the start of the line, pos2 the end. The Y Axis starts at po2 and extends in the y direction in the size of the radius
//! Stores the 3 points in an Vector
void iAVRHistogramPairVis::calculateAxis(double pos1[3], double pos2[3])
{
	m_axesPoly->push_back(vtkSmartPointer<vtkPolyData>::New());
	vtkSmartPointer<vtkPoints> axesPoints = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> axesLines = vtkSmartPointer<vtkCellArray>::New();

	axesPoints->InsertNextPoint(pos1);
	axesPoints->InsertNextPoint(pos2);
	auto yAxisLength = m_radius - m_offsetFromCenter;
	axesPoints->InsertNextPoint(pos2[0], pos2[1] + yAxisLength, pos2[2]); // y Axis

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
void iAVRHistogramPairVis::calculateCenterOffsetPos(double pos1[3], double pos2[3], double newPos[3])
{
	double direction[3]{};
	vtkMath::Subtract(pos2, pos1, direction);

	iAVec3d normDir = iAVec3d(direction);
	normDir.normalize();
	normDir = normDir * m_offsetFromCenter;

	newPos[0] = pos1[0] + normDir[0];
	newPos[1] = pos1[1] + normDir[1];
	newPos[2] = pos1[2] + normDir[2];
}

//! Calculates the lenght between the startPoint (with offset) to the second point on the circle
double iAVRHistogramPairVis::calculateAxisLength(double pos1[3], double radius)
{
	double pos2[3]{};
	calculateAxisPositionInCircle(0.0, 1.0, pos1, radius, pos2); // axis and number of Axes are not important as we just want any point on the circle

	double newPos1[3]{};
	calculateCenterOffsetPos(pos1, pos2, newPos1);

	double newLine[3]{};
	vtkMath::Subtract(pos2, newPos1, newLine);
	iAVec3d temp = iAVec3d(newLine);

	return temp.length();
}

//! Creates as many Marks on the axis as defined by m_numberOfXBins and m_numberOfYBins
//! Y Range has m_numberOfYBins without the zero mark
//! The Marks have 2% of the length of the axis
void iAVRHistogramPairVis::createAxisMarks(size_t axis)
{
	m_axesMarksPoly->push_back(std::vector<vtkSmartPointer<vtkPolyData>>());

	iAVec3d pos1 = iAVec3d(m_axesPoly->at(axis)->GetPoint(0));
	iAVec3d pos2 = iAVec3d(m_axesPoly->at(axis)->GetPoint(1));
	iAVec3d pos3 = iAVec3d(m_axesPoly->at(axis)->GetPoint(2));

	//Divide X Range by bins
	iAVec3d normDirX = pos2 - pos1;
	double rangeX = normDirX.length();
	normDirX.normalize();
	double stepX = rangeX / m_numberOfXBins;

	//Divide Y Range by bins
	iAVec3d normDirY = pos3 - pos2;
	double rangeY = normDirY.length();
	normDirY.normalize();
	double stepY = rangeY / m_numberOfYBins;

	double markLengthXInside = rangeY * 0;
	double markLengthXOutside = rangeY * 0.025;
	double markLengthYInside = rangeX * 1;
	double markLengthYOutside = rangeX * 1.05;

	vtkSmartPointer<vtkPoints> markPoints = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> markLines = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkPolyData> markPoly = vtkSmartPointer<vtkPolyData>::New();
	//double markLengthX = rangeX * 0.02;
	vtkIdType pointID = 0;

	for (int x = 0; x < m_numberOfXBins; x++)
	{
		iAVec3d move = normDirX * (stepX * x);
		iAVec3d mark = pos1 + move;

		//markPoints->InsertNextPoint(mark[0], mark[1] + markLengthX, mark[2]); // upperPoint
		markPoints->InsertNextPoint(mark[0], mark[1] + markLengthXInside, mark[2]); // upperPoint ON Line
		markPoints->InsertNextPoint(mark[0], mark[1] - markLengthXOutside, mark[2]); // lowerPoint

		auto l = vtkSmartPointer<vtkLine>::New();
		//x Axis
		l->GetPointIds()->SetId(0, pointID);
		l->GetPointIds()->SetId(1, pointID + 1);
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
	//double markLengthY = rangeY * 0.02;
	pointID = 0;

	//Because Y Range starts at 0 we have +1 step in Y
	int numberOfDrawnYBins = m_numberOfYBins + 1;

	for (int y = 0; y < numberOfDrawnYBins; y++)
	{
		iAVec3d move = normDirY * (stepY * y);
		iAVec3d mark = pos2 + move; // Add here to pos2 the markLengthXInside offset

		double a[3] = { markLengthYInside ,markLengthYInside ,markLengthYInside };
		double b[3] = { markLengthYOutside ,markLengthYOutside ,markLengthYOutside };

		markPoints->InsertNextPoint(applyShiftToVector(iAVec3d(pos1[0], mark[1], pos1[2]).data(), mark.data(), a).data()); // leftPoint
		markPoints->InsertNextPoint(applyShiftToVector(mark.data(), iAVec3d(pos1[0], mark[1], pos1[2]).data(), b).data()); // rightPoint
		//markPoints->InsertNextPoint(mark[0] - markLengthYInside, mark[1], mark[2]); // leftPoint
		//markPoints->InsertNextPoint(mark[0] + markLengthOutside, mark[1], mark[2]); // rightPoint

		auto l = vtkSmartPointer<vtkLine>::New();
		//y Axis
		l->GetPointIds()->SetId(0, pointID);
		l->GetPointIds()->SetId(1, pointID + 1);
		markLines->InsertNextCell(l);
		pointID += 2;
	}

	//Store Y
	markPoly->SetPoints(markPoints);
	markPoly->SetLines(markLines);
	m_axesMarksPoly->at(axis).push_back(markPoly);
}

void iAVRHistogramPairVis::createAxisLabels(size_t axis)
{
	m_axisTitleActor->push_back(iAVR3DText(m_renderer));
	m_axisLabelActor->push_back(std::vector<std::vector<iAVR3DText>>());

	double offsetXAxis = m_radius * 0.04;
	double offsetYAxis = m_radius * 0.006;
	int loopCount = 0;

	auto minVal = vtkMath::Min(m_histogram01.at(axis).m_histogramParameters.minValue, m_histogram01.at(axis).m_histogramParameters.minValue);
	//X Dir
	m_axisLabelActor->at(axis).push_back(std::vector<iAVR3DText>());
	for (vtkIdType point = 1; point < m_axesMarksPoly->at(axis).at(0)->GetNumberOfPoints(); point += 2)
	{
		double* pos = m_axesMarksPoly->at(axis).at(0)->GetPoint(point);

		if (loopCount % 2 == 0) pos[1] -= offsetXAxis; //Add offset to y
		else pos[1] -= offsetXAxis * 2.1;

		auto tempText = iAVR3DText(m_renderer);
		QString textLabel = QString("%1-%2").arg(minVal, 0, 'f', 2).arg(minVal + (m_histogram01.at(axis).m_histogramParameters.binWidth), 0, 'f', 2);
		tempText.createSmall3DLabel(textLabel);
		tempText.setLabelPos(pos);
		m_axisLabelActor->at(axis).at(0).push_back(tempText);
		minVal += m_histogram01.at(axis).m_histogramParameters.binWidth;
		loopCount++;
	}

	//Y Dir
	m_axisLabelActor->at(axis).push_back(std::vector<iAVR3DText>());
	int count = 0;
	for (vtkIdType point = 1; point < m_axesMarksPoly->at(axis).at(1)->GetNumberOfPoints(); point += 2)
	{
		double* pos = m_axesMarksPoly->at(axis).at(1)->GetPoint(point);
		pos[0] += offsetYAxis; //Add offset to x

		auto occurence = (((double)getMaxBinOccurrences(axis)) / ((double)m_numberOfYBins)) * (double)count;

		auto tempText = iAVR3DText(m_renderer);
		tempText.createSmall3DLabel(QString("%1").arg(round(occurence)));
		tempText.setLabelPos(pos);

		m_axisLabelActor->at(axis).at(1).push_back(tempText);
		count++;
	}

	double titlePos[3]{};
	auto height = m_axesMarksPoly->at(axis).at(1)->GetPoint(m_axesMarksPoly->at(axis).at(1)->GetNumberOfPoints() - 1)[1];

	titlePos[0] = m_axesMarksPoly->at(axis).at(0)->GetPoint(m_axesMarksPoly->at(axis).at(0)->GetNumberOfPoints() / 2)[0];
	titlePos[1] = height + (0.05 * height);
	titlePos[2] = m_axesMarksPoly->at(axis).at(0)->GetPoint(m_axesMarksPoly->at(axis).at(0)->GetNumberOfPoints() / 2)[2];
	m_axisTitleActor->at(axis).create3DLabel(QString("%1").arg(m_histogramMetric->getFeatureName(m_histogram01.at(axis).m_histogramParameters.featureID)));
	m_axisTitleActor->at(axis).setLabelPos(titlePos);
}

//! Calculates with the 3 points of the two axes, the middle point of the plane which they create
void iAVRHistogramPairVis::calculateAxesViewPoint(size_t axis)
{
	//Calc middle point of plane
	iAVec3d pos1 = iAVec3d(m_axesPoly->at(axis)->GetPoint(0));
	iAVec3d pos2 = iAVec3d(m_axesPoly->at(axis)->GetPoint(1)); // origin
	iAVec3d pos3 = iAVec3d(m_axesPoly->at(axis)->GetPoint(2));

	iAVec3d planeVec1 = pos2 - pos1;
	iAVec3d planeVec2 = pos2 - pos3;
	iAVec3d middlePoint = iAVec3d(pos1[0] + (planeVec1[0] / 2), pos2[1] + (planeVec2[1] / 2), pos1[2] + (planeVec1[2] / 2));
	m_AxesViewDir->insert(std::make_pair(static_cast<vtkIdType>(axis), middlePoint));
}

void iAVRHistogramPairVis::calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor)
{
	//At least one cube (occurrence)
	if (stackSize > 0)
	{
		double* p = markPos; // m_axesMarksPoly->at(axis).at(0)->GetPoint(point);
		barPoints->InsertNextPoint(p[0], p[1] + (cubeSize[1] / 2), p[2]);

		colorArray->InsertNextTuple4(barColor.red(), barColor.green(), barColor.blue(), barColor.alpha());

		double nextCubeHeight = cubeSize[1] / 2;
		for (int y = 0; y < stackSize - 1; y++)
		{
			nextCubeHeight += cubeSize[1];
			barPoints->InsertNextPoint(p[0], p[1] + nextCubeHeight, p[2]);

			colorArray->InsertNextTuple4(barColor.red(), barColor.green(), barColor.blue(), barColor.alpha());
		}
	}
}

//! Calculates the cube (x,z) size based on the axis length and number of x bins
//! X and Z are the same
double iAVRHistogramPairVis::getXZCubeSize()
{
	return (m_axisLength / m_numberOfXBins) / 2.2;
}

int iAVRHistogramPairVis::getMaxBinOccurrences(size_t axis)
{
	auto r1 = std::max_element(m_histogram01.at(axis).m_histogramParameters.observations.begin(), m_histogram01.at(axis).m_histogramParameters.observations.end());
	auto r2 = std::max_element(m_histogram02.at(axis).m_histogramParameters.observations.begin(), m_histogram02.at(axis).m_histogramParameters.observations.end());

	int maxVal = std::max((int)*r1, (int)*r2);
	return maxVal;
}

//! Calculates the smallest y size by dividing the y axis length by the max occurences of that feature
double iAVRHistogramPairVis::getMinYCubeSize(size_t axis)
{
	double ySize = (m_axisLength / (double)getMaxBinOccurrences(axis));
	return ySize;
}

//! Calculates the shifted coordinates based on the vector between two points
//! The point p2 is shifted in the direction of p1
iAVec3d iAVRHistogramPairVis::applyShiftToVector(double point1[3], double point2[3], double shift[3])
{
	iAVec3d p1 = iAVec3d(point1);
	iAVec3d p2 = iAVec3d(point2);
	iAVec3d ray = p1 - p2;
	iAVec3d ray_normalized = ray;
	ray_normalized.normalize();

	iAVec3d move = ray_normalized * iAVec3d(shift);
	iAVec3d shiftedVec = p2 + move;

	return shiftedVec;
}
