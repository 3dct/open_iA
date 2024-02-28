// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRHistogramPairVis.h"

#include "iAVRHistogramMetric.h"
#include "iAVROctreeMetrics.h"

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkAssembly.h>
#include <vtkCubeSource.h>
#include <vtkDoubleArray.h>
#include <vtkGlyph3DMapper.h>
#include <vtkLine.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTable.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
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
	m_activeAxisActor = vtkSmartPointer<vtkActor>::New();
	m_inactiveAxisActor = vtkSmartPointer<vtkActor>::New();
	m_activeHistogramActor = vtkSmartPointer<vtkActor>::New();
	m_inactiveHistogramActor = vtkSmartPointer<vtkActor>::New();
	m_visualizationActor = vtkSmartPointer<vtkAssembly>::New();
	m_outlineActor = vtkSmartPointer<vtkActor>::New();

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

void iAVRHistogramPairVis::createVisualization(double* pos, double visSize, double offset, int level, std::vector<vtkIdType> const & regions, std::vector<int> const& featureList)
{
	if (regions.size() < 2)
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
	vtkNew<vtkRegularPolygonSource> polygonSource;
	//polygonSource->GeneratePolygonOff(); // Uncomment this line to generate only the outline of the circle
	polygonSource->SetNumberOfSides(36);
	polygonSource->SetRadius(m_radius);
	polygonSource->SetCenter(pos);
	polygonSource->SetNormal(0, 1, 0);

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputConnection(polygonSource->GetOutputPort());;

	m_sphereActor = vtkSmartPointer<vtkActor>::New();
	m_sphereActor->SetMapper(mapper);
	m_sphereActor->GetProperty()->SetOpacity(0.05);
	m_sphereActor->GetProperty()->SetLineWidth(2.8);

	//Create Bounding Sphere (for interaction)
	vtkNew<vtkSphereSource> boundingSphere;
	boundingSphere->SetCenter(m_centerOfVis);
	boundingSphere->SetRadius(m_radius);

	vtkNew<vtkPolyDataMapper> outlineMapper;
	outlineMapper->SetInputConnection(boundingSphere->GetOutputPort());
	m_outlineActor->SetMapper(outlineMapper);
	m_outlineActor->GetProperty()->SetOpacity(0.0001);

	m_axesPoly.clear();
	m_axesMarksPoly.clear();
	m_axisLabelActor.clear();
	m_axisTitleActor.clear();
	m_axesViewDir.clear();
	m_histogramBars.clear();

	//Calculate Histogram Values and Bins for every feature
	for(int feature : featureList)
	{
		//LOG(lvlImportant, QString("\n Feature: %1 (%2)").arg(feature).arg(m_octreeMetric->getFeatureName(feature)));

		auto val01 = m_octreeMetric->getRegionValues(level, regions.at(0), feature);
		auto val02 = m_octreeMetric->getRegionValues(level, regions.at(1), feature);
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

vtkAssembly* iAVRHistogramPairVis::getVisAssembly()
{
	return m_visualizationActor;
}

void iAVRHistogramPairVis::show()
{
	if (m_visible)
	{
		return;
	}

	mergeActors();
	m_renderer->AddActor(m_visualizationActor);

	m_visible = true;
}

void iAVRHistogramPairVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_visualizationActor);

	for (size_t i = 0; i < m_axisLabelActor.size(); i++)
	{
		m_axisTitleActor.at(i).hide();

		for (size_t j = 0; j < m_axisLabelActor.at(i).size(); j++)
		{
			for (size_t k = 0; k < m_axisLabelActor.at(i).at(j).size(); k++)
			{
				m_axisLabelActor.at(i).at(j).at(k).hide();
			}
		}
	}

	m_axisInView = -1;
	m_visible = false;
}

void iAVRHistogramPairVis::determineHistogramInView(double* viewDir)
{
	if (m_visible)
	{
		double minDistance = std::numeric_limits<double>::infinity();
		int newAxisInView = -1;

		for (size_t i = 0; i < m_axesViewDir.size(); i++)
		{
			auto focalP = iAVec3d(viewDir[0], viewDir[1], viewDir[2]);
			auto ray = (m_axesViewDir.at(i) - (focalP));
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

void iAVRHistogramPairVis::applyTransform(vtkTransform* transform, size_t i)
{
	m_axesViewDir.at(i) = iAVec3d(transform->TransformPoint(m_axesViewDir.at(i).data()));

	vtkNew<vtkTransformPolyDataFilter> transformAxesPoly;
	transformAxesPoly->SetInputData(m_axesPoly.at(i));
	transformAxesPoly->SetTransform(transform);
	transformAxesPoly->Update();
	m_axesPoly.at(i) = transformAxesPoly->GetOutput();

	for (size_t d = 0; d < m_axesMarksPoly.at(i).size(); d++)
	{
		vtkNew<vtkTransformPolyDataFilter> transformAxesMarksPoly;
		transformAxesMarksPoly->SetInputData(m_axesMarksPoly.at(i).at(d));
		transformAxesMarksPoly->SetTransform(transform);
		transformAxesMarksPoly->Update();
		m_axesMarksPoly.at(i).at(d) = transformAxesMarksPoly->GetOutput();
	}

	vtkNew<vtkTransformPolyDataFilter> transformHistogramBars;
	transformHistogramBars->SetInputData(m_histogramBars.at(i));
	transformHistogramBars->SetTransform(transform);
	transformHistogramBars->Update();
	m_histogramBars.at(i) = transformHistogramBars->GetOutput();

	m_axisTitleActor.at(i).transformPosition(transform);
	for (size_t k = 0; k < m_axisLabelActor.at(i).size(); k++)
	{
		for (size_t j = 0; j < m_axisLabelActor.at(i).at(k).size(); j++)
		{
			m_axisLabelActor.at(i).at(k).at(j).transformPosition(transform);
		}
	}
}

void iAVRHistogramPairVis::rotateVisualization(double y)
{
	if (!m_visible)
	{
		return;
	}
	vtkNew<vtkTransform> transform;
	transform->PostMultiply(); //this is the key line
	transform->Translate(-m_centerOfVis[0], -m_centerOfVis[1], -m_centerOfVis[2]);
	transform->RotateY(-y);
	transform->Translate(m_centerOfVis[0], m_centerOfVis[1], m_centerOfVis[2]);

	for (size_t i = 0; i < m_axesPoly.size(); i++)
	{
		applyTransform(transform, i);
	}

	drawAxes(m_axisInView);
	drawHistogram(m_axisInView);
}

void iAVRHistogramPairVis::flipThroughHistograms(double flipDir)
{
	int currentFlip = (flipDir > 0) ? 0 : 1;

	vtkNew<vtkTransform> transformFront;
	transformFront->PostMultiply(); //this is the key line
	transformFront->Translate(-m_centerOfVis[0], -m_centerOfVis[1], -m_centerOfVis[2]);
	transformFront->RotateY(180 * flipDir);
	transformFront->Translate(m_centerOfVis[0], m_centerOfVis[1], m_centerOfVis[2]);

	vtkNew<vtkTransform> transform;
	transform->PostMultiply(); //this is the key line
	transform->Translate(-m_centerOfVis[0], -m_centerOfVis[1], -m_centerOfVis[2]);
	transform->RotateY(vtkMath::DegreesFromRadians(axisAngle) * flipDir);
	transform->Translate(m_centerOfVis[0], m_centerOfVis[1], m_centerOfVis[2]);

	for (size_t i = 0; i < m_axesPoly.size(); i++)
	{
		auto tr = (static_cast<int>(i) == m_frontAxes[currentFlip]) ?//The axis with 0 or 180 degree angle
			transformFront.Get() : transform.Get();
		applyTransform(tr, i);
	}

	m_frontAxes[0] += flipDir;
	m_frontAxes[1] += flipDir;

	for (int pos = 0; pos < 2; pos++)
	{
		if (m_frontAxes[pos] < 0) m_frontAxes[pos] = static_cast<int>(m_axesPoly.size() - 1);
		if (m_frontAxes[pos] > static_cast<int>(m_axesPoly.size()) - 1) m_frontAxes[pos] = 0;
	}

	drawAxes(m_axisInView);
	drawHistogram(m_axisInView);
}

double iAVRHistogramPairVis::getRadius() const
{
	return m_radius;
}

std::vector<QColor> iAVRHistogramPairVis::getBarColors() const
{
	std::vector<QColor> barCol = std::vector<QColor>();
	barCol.push_back(barColorR1);
	barCol.push_back(barColorR2);

	return barCol;
}

void iAVRHistogramPairVis::mergeActors()
{
	m_visualizationActor->AddPart(m_sphereActor);
	m_visualizationActor->AddPart(m_outlineActor);
	m_visualizationActor->AddPart(m_activeAxisActor);
	m_visualizationActor->AddPart(m_inactiveAxisActor);
	m_visualizationActor->AddPart(m_activeHistogramActor);
	m_visualizationActor->AddPart(m_inactiveHistogramActor);
	m_visualizationActor->Modified();
}

void iAVRHistogramPairVis::showHistogramInView()
{
	if (m_axisInView != -1 && m_axisInView != currentlyShownAxis)
	{
		if (currentlyShownAxis != -1)
		{
			//Delete old
			m_axisTitleActor.at(currentlyShownAxis).hide();
			for (size_t j = 0; j < m_axisLabelActor.at(currentlyShownAxis).size(); j++)
			{
				for (size_t k = 0; k < m_axisLabelActor.at(currentlyShownAxis).at(j).size(); k++)
				{
					m_axisLabelActor.at(currentlyShownAxis).at(j).at(k).hide();
				}
			}
		}

		drawAxes(m_axisInView);
		drawHistogram(m_axisInView);

		//Create new
		m_axisTitleActor.at(m_axisInView).show();
		for (size_t j = 0; j < m_axisLabelActor.at(m_axisInView).size(); j++)
		{
			for (size_t k = 0; k < m_axisLabelActor.at(m_axisInView).at(j).size(); k++)
			{
				m_axisLabelActor.at(m_axisInView).at(j).at(k).show();
			}
		}

		m_visualizationActor->Modified();
	}
	currentlyShownAxis = m_axisInView;
}

void iAVRHistogramPairVis::calculateAxisPositionInCircle(double axis, double numberOfAxes, double centerPos[3], double radius, double pointOnCircle[3])
{
	//axisAngle = (2.0 * vtkMath::Pi()) / double(numberOfAxes); // 360° = 2*PI
	axisAngle = (vtkMath::Pi()) / numberOfAxes; // 180° = PI
	double currentAngle = axis * axisAngle;

	pointOnCircle[0] = centerPos[0] + (radius * cos(currentAngle));
	pointOnCircle[1] = centerPos[1];
	pointOnCircle[2] = centerPos[2] + (radius * sin(currentAngle));
}

void iAVRHistogramPairVis::drawAxes(int visibleAxis)
{
	if (visibleAxis != -1)
	{
		vtkNew<vtkAppendPolyData> appendActiveFilter;
		vtkNew<vtkAppendPolyData> appendInactiveFilter;

		for (size_t axisPoly = 0; axisPoly < m_axesPoly.size(); axisPoly++)
		{
			if (static_cast<int>(axisPoly) == visibleAxis)
			{
				appendActiveFilter->AddInputData(m_axesPoly.at(axisPoly));

				appendActiveFilter->AddInputData(m_axesMarksPoly.at(axisPoly).at(0)); //xMarks
				appendActiveFilter->AddInputData(m_axesMarksPoly.at(axisPoly).at(1)); //yMarks
			}
			else
			{
				appendInactiveFilter->AddInputData(m_axesPoly.at(axisPoly));

				appendInactiveFilter->AddInputData(m_axesMarksPoly.at(axisPoly).at(0)); //xMarks
				appendInactiveFilter->AddInputData(m_axesMarksPoly.at(axisPoly).at(1)); //yMarks
			}
		}

		appendActiveFilter->Update();
		appendInactiveFilter->Update();

		// Create mappers
		vtkNew<vtkPolyDataMapper> axesActiveMapper;
		axesActiveMapper->SetInputConnection(appendActiveFilter->GetOutputPort());
		m_activeAxisActor->SetMapper(axesActiveMapper);
		m_activeAxisActor->SetPickable(false);
		m_activeAxisActor->GetProperty()->SetLineWidth(3);
		m_activeAxisActor->GetProperty()->SetColor(0.4, 0.4, 0.4);

		vtkNew<vtkPolyDataMapper> axesInactiveMapper;
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
		vtkNew<vtkAppendPolyData> appendActiveFilter;
		vtkNew<vtkAppendPolyData> appendInactiveFilter;

		for (int axis = 0; axis < static_cast<int>(m_histogramBars.size()); axis++)
		{
			if (axis == visibleAxis)
			{
				appendActiveFilter->AddInputData(m_histogramBars.at(axis));
			}
			else
			{
				appendInactiveFilter->AddInputData(m_histogramBars.at(axis));
			}
		}

		appendActiveFilter->Update();
		appendInactiveFilter->Update();

		vtkNew<vtkGlyph3DMapper> activeHistogramGlyphs;
		activeHistogramGlyphs->SetInputData(appendActiveFilter->GetOutput());

		vtkNew<vtkGlyph3DMapper> inactiveHistogramGlyphs;
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
	vtkNew<vtkCubeSource> cubeSource;
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
	vtkNew<vtkPoints> barPoints;
	vtkNew<vtkPolyData> bars;

	vtkNew<vtkDoubleArray> glyphScale;
	glyphScale->SetName("scale");
	glyphScale->SetNumberOfComponents(3);

	vtkNew<vtkDoubleArray> glyphRotation;
	glyphRotation->SetName("rotate");
	glyphRotation->SetNumberOfComponents(3);

	vtkNew<vtkUnsignedCharArray> glyphColor;
	glyphColor->SetName("colors");
	glyphColor->SetNumberOfComponents(4);

	double cubeXZSize = getXZCubeSize(); //Should depend on size between two marks
	double minYBarSize = getMinYCubeSize(axis);
	double rotAngle = -(vtkMath::DegreesFromRadians(axisAngle * static_cast<double>(axis)));

	iAVec3d direction = iAVec3d(0, 0, 0);
	direction = iAVec3d(m_axesPoly.at(axis)->GetPoint(1)) - iAVec3d(m_axesPoly.at(axis)->GetPoint(0));
	direction.normalize();
	//iAVec3d cubeHalfSize = iAVec3d(cubeSize[0] / 2.0 ,cubeSize[1] / 2.0 ,cubeSize[2] / 2.0);
	direction = direction * (cubeXZSize / 2.0);

	int binCount = 0;
	for (vtkIdType point = 0; point < m_axesMarksPoly.at(axis).at(0)->GetNumberOfPoints(); point += 2)
	{
		// Move first cube a little bit to the left and second to the right
		auto markPos = m_axesMarksPoly.at(axis).at(0)->GetPoint(point);

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

	m_histogramBars.push_back(bars);
}

void iAVRHistogramPairVis::calculateAxis(double pos1[3], double pos2[3])
{
	m_axesPoly.push_back(vtkSmartPointer<vtkPolyData>::New());
	vtkNew<vtkPoints> axesPoints;
	vtkNew<vtkCellArray> axesLines;

	axesPoints->InsertNextPoint(pos1);
	axesPoints->InsertNextPoint(pos2);
	auto yAxisLength = m_radius - m_offsetFromCenter;
	axesPoints->InsertNextPoint(pos2[0], pos2[1] + yAxisLength, pos2[2]); // y Axis

	vtkNew<vtkLine> l;
	//x Axis
	l->GetPointIds()->SetId(0, 0);
	l->GetPointIds()->SetId(1, 1);
	axesLines->InsertNextCell(l);
	//y Axis
	l->GetPointIds()->SetId(0, 1);
	l->GetPointIds()->SetId(1, 2);
	axesLines->InsertNextCell(l);

	m_axesPoly.back()->SetPoints(axesPoints);
	m_axesPoly.back()->SetLines(axesLines);
}

void iAVRHistogramPairVis::calculateCenterOffsetPos(double pos1[3], double pos2[3], double newPos[3])
{
	double direction[3]{};
	vtkMath::Subtract(pos2, pos1, direction);

	iAVec3d normDir = iAVec3d(direction).normalized();
	normDir = normDir * m_offsetFromCenter;

	newPos[0] = pos1[0] + normDir[0];
	newPos[1] = pos1[1] + normDir[1];
	newPos[2] = pos1[2] + normDir[2];
}

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

void iAVRHistogramPairVis::createAxisMarks(size_t axis)
{
	m_axesMarksPoly.push_back(std::vector<vtkSmartPointer<vtkPolyData>>());

	iAVec3d pos1 = iAVec3d(m_axesPoly.at(axis)->GetPoint(0));
	iAVec3d pos2 = iAVec3d(m_axesPoly.at(axis)->GetPoint(1));
	iAVec3d pos3 = iAVec3d(m_axesPoly.at(axis)->GetPoint(2));

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

	vtkNew<vtkPoints> markPointsX;
	vtkNew<vtkCellArray> markLinesX;
	vtkNew<vtkPolyData> markPolyX;
	//double markLengthX = rangeX * 0.02;
	vtkIdType xPointID = 0;

	for (int x = 0; x < m_numberOfXBins; x++)
	{
		iAVec3d move = normDirX * (stepX * x);
		iAVec3d mark = pos1 + move;

		//markPoints->InsertNextPoint(mark[0], mark[1] + markLengthX, mark[2]); // upperPoint
		markPointsX->InsertNextPoint(mark[0], mark[1] + markLengthXInside, mark[2]); // upperPoint ON Line
		markPointsX->InsertNextPoint(mark[0], mark[1] - markLengthXOutside, mark[2]); // lowerPoint

		vtkNew<vtkLine> l;
		//x Axis
		l->GetPointIds()->SetId(0, xPointID);
		l->GetPointIds()->SetId(1, xPointID + 1);
		markLinesX->InsertNextCell(l);
		xPointID += 2;
	}
	//Store X
	markPolyX->SetPoints(markPointsX);
	markPolyX->SetLines(markLinesX);
	m_axesMarksPoly.at(axis).push_back(markPolyX);

	vtkNew<vtkPoints> markPointsY;
	vtkNew<vtkCellArray> markLinesY;
	vtkNew<vtkPolyData> markPolyY;
	//double markLengthY = rangeY * 0.02;
	vtkIdType yPointID = 0;

	//Because Y Range starts at 0 we have +1 step in Y
	int numberOfDrawnYBins = m_numberOfYBins + 1;

	for (int y = 0; y < numberOfDrawnYBins; y++)
	{
		iAVec3d move = normDirY * (stepY * y);
		iAVec3d mark = pos2 + move; // Add here to pos2 the markLengthXInside offset

		double a[3] = { markLengthYInside ,markLengthYInside ,markLengthYInside };
		double b[3] = { markLengthYOutside ,markLengthYOutside ,markLengthYOutside };

		markPointsY->InsertNextPoint(applyShiftToVector(iAVec3d(pos1[0], mark[1], pos1[2]).data(), mark.data(), a).data()); // leftPoint
		markPointsY->InsertNextPoint(applyShiftToVector(mark.data(), iAVec3d(pos1[0], mark[1], pos1[2]).data(), b).data()); // rightPoint
		//markPoints->InsertNextPoint(mark[0] - markLengthYInside, mark[1], mark[2]); // leftPoint
		//markPoints->InsertNextPoint(mark[0] + markLengthOutside, mark[1], mark[2]); // rightPoint

		vtkNew<vtkLine> l;
		//y Axis
		l->GetPointIds()->SetId(0, yPointID);
		l->GetPointIds()->SetId(1, yPointID + 1);
		markLinesY->InsertNextCell(l);
		yPointID += 2;
	}

	//Store Y
	markPolyY->SetPoints(markPointsY);
	markPolyY->SetLines(markLinesY);
	m_axesMarksPoly.at(axis).push_back(markPolyY);
}

void iAVRHistogramPairVis::createAxisLabels(size_t axis)
{
	m_axisLabelActor.push_back(std::vector<std::vector<iAVR3DText>>());

	double offsetXAxis = m_radius * 0.04;
	double offsetYAxis = m_radius * 0.006;
	int loopCount = 0;

	auto minVal = vtkMath::Min(m_histogram01.at(axis).m_histogramParameters.minValue, m_histogram01.at(axis).m_histogramParameters.minValue);
	//X Dir
	m_axisLabelActor.at(axis).push_back(std::vector<iAVR3DText>());
	for (vtkIdType point = 1; point < m_axesMarksPoly.at(axis).at(0)->GetNumberOfPoints(); point += 2)
	{
		double* pos = m_axesMarksPoly.at(axis).at(0)->GetPoint(point);

		if (loopCount % 2 == 0) pos[1] -= offsetXAxis; //Add offset to y
		else pos[1] -= offsetXAxis * 2.1;

		QString textLabel = QString("%1-%2").arg(minVal, 0, 'f', 2).arg(minVal + (m_histogram01.at(axis).m_histogramParameters.binWidth), 0, 'f', 2);
		iAVR3DText tempText(m_renderer, textLabel, true);
		tempText.setLabelPos(pos);
		m_axisLabelActor.at(axis).at(0).push_back(tempText);
		minVal += m_histogram01.at(axis).m_histogramParameters.binWidth;
		loopCount++;
	}

	//Y Dir
	m_axisLabelActor.at(axis).push_back(std::vector<iAVR3DText>());
	int count = 0;
	for (vtkIdType point = 1; point < m_axesMarksPoly.at(axis).at(1)->GetNumberOfPoints(); point += 2)
	{
		double* pos = m_axesMarksPoly.at(axis).at(1)->GetPoint(point);
		pos[0] += offsetYAxis; //Add offset to x

		auto occurence = (((double)getMaxBinOccurrences(axis)) / ((double)m_numberOfYBins)) * (double)count;

		iAVR3DText tempText(m_renderer, QString("%1").arg(round(occurence)), true);
		tempText.setLabelPos(pos);

		m_axisLabelActor.at(axis).at(1).push_back(tempText);
		count++;
	}

	double titlePos[3]{};
	auto height = m_axesMarksPoly.at(axis).at(1)->GetPoint(m_axesMarksPoly.at(axis).at(1)->GetNumberOfPoints() - 1)[1];

	titlePos[0] = m_axesMarksPoly.at(axis).at(0)->GetPoint(m_axesMarksPoly.at(axis).at(0)->GetNumberOfPoints() / 2)[0];
	titlePos[1] = height + (0.05 * height);
	titlePos[2] = m_axesMarksPoly.at(axis).at(0)->GetPoint(m_axesMarksPoly.at(axis).at(0)->GetNumberOfPoints() / 2)[2];
	m_axisTitleActor.push_back(iAVR3DText(m_renderer, QString("%1").arg(m_histogramMetric->getFeatureName(m_histogram01.at(axis).m_histogramParameters.featureID))));
	m_axisTitleActor.at(axis).setLabelPos(titlePos);
}

void iAVRHistogramPairVis::calculateAxesViewPoint(size_t axis)
{
	//Calc middle point of plane
	iAVec3d pos1 = iAVec3d(m_axesPoly.at(axis)->GetPoint(0));
	iAVec3d pos2 = iAVec3d(m_axesPoly.at(axis)->GetPoint(1)); // origin
	iAVec3d pos3 = iAVec3d(m_axesPoly.at(axis)->GetPoint(2));

	iAVec3d planeVec1 = pos2 - pos1;
	iAVec3d planeVec2 = pos2 - pos3;
	iAVec3d middlePoint = iAVec3d(pos1[0] + (planeVec1[0] / 2), pos2[1] + (planeVec2[1] / 2), pos1[2] + (planeVec1[2] / 2));
	m_axesViewDir.insert(std::make_pair(static_cast<vtkIdType>(axis), middlePoint));
}

void iAVRHistogramPairVis::calculateBarsWithCubes(double* markPos, double* cubeSize, int stackSize, vtkPoints* barPoints, vtkUnsignedCharArray* colorArray, QColor barColor)
{
	//At least one cube (occurrence)
	if (stackSize > 0)
	{
		double* p = markPos; // m_axesMarksPoly.at(axis).at(0)->GetPoint(point);
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

double iAVRHistogramPairVis::getXZCubeSize() const
{
	return (m_axisLength / m_numberOfXBins) / 2.2;
}

int iAVRHistogramPairVis::getMaxBinOccurrences(size_t axis)
{
	auto r1 = std::max_element(m_histogram01.at(axis).m_histogramParameters.observations.begin(), m_histogram01.at(axis).m_histogramParameters.observations.end());
	auto r2 = std::max_element(m_histogram02.at(axis).m_histogramParameters.observations.begin(), m_histogram02.at(axis).m_histogramParameters.observations.end());
	return std::max(*r1, *r2);
}

double iAVRHistogramPairVis::getMinYCubeSize(size_t axis)
{
	return m_axisLength / getMaxBinOccurrences(axis);
}

iAVec3d iAVRHistogramPairVis::applyShiftToVector(double point1[3], double point2[3], double shift[3])
{
	iAVec3d p2 = iAVec3d(point2);
	iAVec3d ray = iAVec3d(point1) - p2;
	iAVec3d move = ray.normalized() * iAVec3d(shift);
	return p2 + move;
}
