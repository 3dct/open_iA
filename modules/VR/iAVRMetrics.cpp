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
#include "iAVRMetrics.h"

#include <iAConsole.h>
#include <vtkVariant.h>
#include <vtkProperty2D.h>
#include <vtkTextProperty.h>
#include <vtkAssembly.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPlaneSource.h>
#include <vtkCellData.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkGlyph3D.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkAppendPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

iAVRMetrics::iAVRMetrics(vtkRenderer* renderer, vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees):m_renderer(renderer), m_objectTable(objectTable), m_io(io),
m_octrees(octrees)
{
	//numberOfFeatures = iACsvConfig::MappedCount;
	numberOfFeatures = m_objectTable->GetNumberOfColumns();

	//Initialize vectors
	isAlreadyCalculated = new std::vector<std::vector<bool>>(m_octrees->size(), std::vector<bool>(numberOfFeatures, false));
	
	std::vector<double> region = std::vector<double>();
	std::vector<std::vector<double>> feature = std::vector<std::vector<double>>(numberOfFeatures, region);
	m_calculatedStatistic = new std::vector<std::vector<std::vector<double>>>(m_octrees->size(), feature);
	m_maxCoverage = new std::vector<std::vector<std::vector<vtkIdType>>>();
	m_jaccardValues = new std::vector<std::vector<std::vector<double>>>(m_octrees->size());

	mipPlanes = std::vector<vtkPolyData*>();

	storeMinMaxValues();

	titleTextSource = vtkSmartPointer<vtkTextActor3D>::New();
	textSource = vtkSmartPointer<vtkTextActor3D>::New();
	m_ColorBar = vtkSmartPointer<vtkActor>::New();

	m_colorBarVisible = false;
}

//! Has to be called *before* getting any Metric data
void iAVRMetrics::setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage)
{
	m_fiberCoverage = fiberCoverage;
}

//! Calculates the weighted average of every octree region for a given feature at a given octree level.
//! In the first call the metric has to calculate the values, for later calls the metric is saved
//! Calculates:  1/N * SUM[N]( Attribut * weight)  with N = all Fibers in the region
void iAVRMetrics::calculateWeightedAverage(int octreeLevel, int feature)
{
	if(!isAlreadyCalculated->at(octreeLevel).at(feature)){

		//DEBUG_LOG(QString("Possible Regions: %1\n").arg(m_fiberCoverage->at(octreeLevel).size()));

		for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
		{
			double metricResultPerRegion = 0;
			int fibersInRegion = 0;
			//DEBUG_LOG(QString(">>> REGION %1 <<<\n").arg(region));


			for (auto element : *m_fiberCoverage->at(octreeLevel).at(region))
			{
				//DEBUG_LOG(QString("[%1] --- %2 \%").arg(element.first).arg(element.second));
				
				//double fiberAttribute = m_objectTable->GetValue(element.first, m_io.getOutputMapping()->value(feature)).ToFloat();
				double fiberAttribute = m_objectTable->GetValue(element.first, feature).ToFloat();
				double weightedAttribute = fiberAttribute * element.second;

				metricResultPerRegion += weightedAttribute;
				fibersInRegion++;
			}
			if(fibersInRegion == 0)
			{
				fibersInRegion = 1; // Prevent Division by zero
			}
			
			//DEBUG_LOG(QString("Region [%1] --- %2 \%\n").arg(region).arg(metricResultPerRegion / fibersInRegion));
			
			m_calculatedStatistic->at(octreeLevel).at(feature).push_back(metricResultPerRegion / fibersInRegion);
		}
		isAlreadyCalculated->at(octreeLevel).at(feature) = true;
	}
}

//! Returns a rgba coloring (between 0 and 1) vector for every region in the current octree level
//! Value mapping defines which min/max values are taken. 
//! 0: min/max Fiber Attribute
//! 1: min/max Region Value
std::vector<QColor>* iAVRMetrics::getHeatmapColoring(int octreeLevel, int feature, int valueMapping)
{
	std::vector<QColor>* heatmapColors = new std::vector<QColor>(m_fiberCoverage->at(octreeLevel).size(), QColor());
	calculateWeightedAverage(octreeLevel, feature);

	double min = -1;
	double max = -1;

	if (valueMapping == 0)
	{
		min = m_minMaxValues->at(feature).at(0);
		max = m_minMaxValues->at(feature).at(1);
	}
	else if (valueMapping == 1)
	{
		auto minMax = std::minmax_element(m_calculatedStatistic->at(octreeLevel).at(feature).begin(), m_calculatedStatistic->at(octreeLevel).at(feature).end());
		min = *minMax.first;
		max = *minMax.second;
	}
	else
	{
		DEBUG_LOG(QString("Mapping type not found"));
	}

	calculateLUT(min, max, 8); //8 Colors

	for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
	{
		
		double rgba[3] = {0,0,0};
		//double val = histogramNormalization(m_calculatedStatistic->at(octreeLevel).at(feature).at(region),0,1,min,max);
		double val = m_calculatedStatistic->at(octreeLevel).at(feature).at(region);
		m_lut->GetColor(val, rgba);

		heatmapColors->at(region).setRgbF(rgba[0], rgba[1], rgba[2], m_lut->GetOpacity(val));
	}

	return heatmapColors;
}

vtkSmartPointer<vtkLookupTable> iAVRMetrics::calculateLUT(double min, double max, int tableSize)
{
	// Color Scheme: https://colorbrewer2.org/?type=diverging&scheme=RdYlBu&n=8 
	QColor a = QColor(215, 48, 39, 255);
	QColor b = QColor(244, 109, 67, 255);
	QColor c = QColor(253, 174, 97, 255);
	QColor d = QColor(254, 224, 144, 255);
	QColor e = QColor(224, 243, 248, 255);
	QColor f = QColor(171, 217, 233, 255);
	QColor g = QColor(116, 173, 209, 255);
	QColor h = QColor(69, 117, 180, 255);

	//vtkSmartPointer<vtkColorTransferFunction> ctf = createColorTransferFunction(min, max);
	m_lut = vtkSmartPointer<vtkLookupTable>::New();

	m_lut->SetNumberOfTableValues(tableSize);
	m_lut->Build();

	m_lut->SetTableValue(0, h.redF(), h.greenF(), h.blueF());
	m_lut->SetTableValue(1, g.redF(), g.greenF(), g.blueF());
	m_lut->SetTableValue(2, f.redF(), f.greenF(), f.blueF());
	m_lut->SetTableValue(3, e.redF(), e.greenF(), e.blueF());
	m_lut->SetTableValue(4, d.redF(), d.greenF(), d.blueF());
	m_lut->SetTableValue(5, c.redF(), c.greenF(), c.blueF());
	m_lut->SetTableValue(6, b.redF(), b.greenF(), b.blueF());
	m_lut->SetTableValue(7, a.redF(), a.greenF(), a.blueF());

	m_lut->SetTableRange(min, max);
	m_lut->SetBelowRangeColor(1, 1, 1, 1);
	m_lut->UseBelowRangeColorOn();

	calculateColorBarLegend();

	return m_lut;
}

vtkSmartPointer<vtkLookupTable> iAVRMetrics::getLut()
{
	return m_lut;
}

std::vector<std::vector<std::vector<vtkIdType>>>* iAVRMetrics::getMaxCoverageFiberPerRegion()
{
	if(m_maxCoverage->empty()){
		calculateMaxCoverageFiberPerRegion();
		return m_maxCoverage;
	}
	else
	{
		return m_maxCoverage;
	}
}

void iAVRMetrics::calculateColorBarLegend()
{
	//Remove old colorBar
	hideColorBarLegend();

	vtkSmartPointer<vtkPlaneSource> colorBarPlane = vtkSmartPointer<vtkPlaneSource>::New();
	colorBarPlane->SetXResolution(1);
	colorBarPlane->SetYResolution(m_lut->GetNumberOfAvailableColors());

	colorBarPlane->SetOrigin(0, 0, 0.0);
	colorBarPlane->SetPoint1(35, 0, 0.0); //width
	colorBarPlane->SetPoint2(0, 100, 0.0); // height
	colorBarPlane->Update();

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(4);

	m_3DLabels = new std::vector<iAVR3DText*>();

	double min = m_lut->GetTableRange()[0];
	double max = m_lut->GetTableRange()[1];
	double subRange = (max - min) / (m_lut->GetNumberOfAvailableColors() - 1);

	QString text = "";

	for (int i = 0; i < m_lut->GetNumberOfAvailableColors(); i++)
	{

		double rgba[4];
		double value = max - (subRange * i);

		//Text Label
		//m_3DLabels->push_back(new iAVR3DText(m_renderer));
		//m_3DLabels->at(i)->create3DLabel(QString("- %1").arg(value));
		if (i == m_lut->GetNumberOfAvailableColors() - 1)
		{
			text.append(QString("- %1").arg(value));
		}
		else
		{
			text.append(QString("- %1\n").arg(value));
		}
		//Color
		m_lut->GetIndexedColor(i, rgba);
		colorData->InsertNextTuple4(rgba[0] * 255, rgba[1] * 255, rgba[2] * 255, rgba[3] * 255);
	}

	colorBarPlane->GetOutput()->GetCellData()->SetScalars(colorData);
	colorBarPlane->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(colorBarPlane->GetOutputPort());
	mapper->SetScalarModeToUseCellData();
	mapper->Update();

	m_ColorBar = vtkSmartPointer<vtkActor>::New();
	m_ColorBar->SetMapper(mapper);
	m_ColorBar->GetProperty()->EdgeVisibilityOn();

	m_ColorBar->GetProperty()->SetLineWidth(3);

	//title
	titleTextSource = vtkSmartPointer<vtkTextActor3D>::New();
	titleTextSource->GetTextProperty()->SetColor(0, 0, 0);
	titleTextSource->GetTextProperty()->SetBackgroundColor(0.6, 0.6, 0.6);
	titleTextSource->GetTextProperty()->SetBackgroundOpacity(1.0);
	titleTextSource->GetTextProperty()->SetFontSize(20);

	// Create text
	textSource = vtkSmartPointer<vtkTextActor3D>::New();
	textSource->SetInput(text.toUtf8());
	textSource->GetTextProperty()->SetColor(0, 0, 0);
	textSource->GetTextProperty()->SetBackgroundColor(0.6, 0.6, 0.6);
	textSource->GetTextProperty()->SetBackgroundOpacity(1.0);
	textSource->GetTextProperty()->SetFontSize(16);

	double actorBounds[6];
	double pos[3] = { 0, 0, 0 };
	m_ColorBar->GetBounds(actorBounds);
	// ((maxY - minY) / #Labels) and then /2 for the center of a row
	double subScale = ((actorBounds[3] - actorBounds[2]) / (m_lut->GetNumberOfAvailableColors() - 1));

	textSource->SetPosition(actorBounds[1] + 1, actorBounds[2] + 1, -1);
	textSource->SetScale(0.9, 0.72, 1);

	titleTextSource->SetPosition(actorBounds[0] + 1, actorBounds[3] + 5, 0);
}

void iAVRMetrics::showColorBarLegend()
{
	if (m_colorBarVisible)
	{
		return;
	}
	m_renderer->AddActor(m_ColorBar);
	m_renderer->AddActor(textSource);
	m_renderer->AddActor(titleTextSource);
	m_colorBarVisible = true;
}

void iAVRMetrics::hideColorBarLegend()
{
	if (!m_colorBarVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_ColorBar);
	m_renderer->RemoveActor(textSource);
	m_renderer->RemoveActor(titleTextSource);
	m_colorBarVisible = false;
}

int iAVRMetrics::getNumberOfFeatures()
{
	return numberOfFeatures;
}

QString iAVRMetrics::getFeatureName(int feature)
{
	//QString featureName = m_objectTable->GetColumnName(m_io.getOutputMapping()->value(feature));
	QString featureName = m_objectTable->GetColumnName(feature);
	return featureName;
}

void iAVRMetrics::moveColorBarLegend(double* pos)
{
	m_ColorBar->SetPosition(pos);

	double actorBounds[6];
	m_ColorBar->GetBounds(actorBounds);

	textSource->SetPosition(actorBounds[1] + 1, actorBounds[2] + 1, actorBounds[4] - 1);
	titleTextSource->SetPosition(actorBounds[0] + 1, actorBounds[3] + 5, actorBounds[4] - 1);
	
}

void iAVRMetrics::rotateColorBarLegend(double x, double y, double z)
{
	m_ColorBar->AddOrientation(x, y, z);
	titleTextSource->AddOrientation(x, y, z);
	textSource->AddOrientation(x, y, z);	
}

void iAVRMetrics::setLegendTitle(QString title)
{
	titleTextSource->SetInput(title.toUtf8());
}

std::vector<std::vector<std::vector<double>>>* iAVRMetrics::getJaccardIndex(int level)
{
	if(m_jaccardValues->at(level).empty())
	{
		calculateJaccardIndex(level);
		return m_jaccardValues;
	}
	else
	{
		return m_jaccardValues;
	}
	
}

//! Creates a plane for every possible MIP Projection (six planes)
//! The plane cells start at the lower left cell depending on the origin Point of the Plane
void iAVRMetrics::createMIPPanels(int octreeLevel, int feature)
{
	int gridSize = pow(2, octreeLevel);
	double direction[6] = {1,1,-1,-1,-1,1}; //normals
	int viewPlane[6] = {0,2,0,2,1,1}; //x,y,z

	std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
	m_octrees->at(octreeLevel)->createOctreeBoundingBoxPlanes(planePoints);

	vtkSmartPointer<vtkAppendPolyData> appendFilter =	vtkSmartPointer<vtkAppendPolyData>::New();

	for (int i = 0; i < 6; i++)
	{
		vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
		plane->SetXResolution(gridSize);
		plane->SetYResolution(gridSize);
		plane->SetOrigin(planePoints->at(i).at(0).data());
		plane->SetPoint1(planePoints->at(i).at(1).data());
		plane->SetPoint2(planePoints->at(i).at(2).data());
		plane->Push(600 * direction[i]);
		plane->Update();

		vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorData->SetName("colors");
		colorData->SetNumberOfComponents(4);

		std::vector<QColor>* col = calculateMIPColoring(viewPlane[i], octreeLevel, feature);
		//for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
		for (int gridCell = 0; gridCell < gridSize * gridSize; gridCell++)
		{
			colorData->InsertNextTuple4(col->at(gridCell).red(), col->at(gridCell).green(), col->at(gridCell).blue(), col->at(gridCell).alpha());
		}

		plane->GetOutput()->GetCellData()->SetScalars(colorData);

		mipPlanes.push_back(plane->GetOutput());
		appendFilter->AddInputData(plane->GetOutput());
	}

	vtkSmartPointer<vtkPolyDataMapper> mapper =	vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(appendFilter->GetOutputPort());

	mipPanel = vtkSmartPointer<vtkActor>::New();
	mipPanel->SetMapper(mapper);
	mipPanel->PickableOff();
	mipPanel->GetProperty()->EdgeVisibilityOn();
	mipPanel->GetProperty()->SetLineWidth(4);
	
	m_renderer->AddActor(mipPanel);

	//double scale = m_vrEnv->interactor()->GetPhysicalScale();
	//double *trans = m_vrEnv->interactor()->GetPhysicalTranslation(m_vrEnv->renderer()->GetActiveCamera());
}

void iAVRMetrics::hideMIPPanels()
{
	m_renderer->RemoveActor(mipPanel);
}


double iAVRMetrics::histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax)
{
	double result = ((newMax - newMin) * ((value - oldMin) / (oldMax - oldMin))) + newMin;
	return result;
}

void iAVRMetrics::storeMinMaxValues()
{
	m_minMaxValues = new std::vector<std::vector<double>>();

	//int numberOfFeatures = m_objectTable->GetNumberOfColumns();

	std::vector<double> minAttribute = std::vector<double>(); //= m_objectTable->GetColumn(feature)->GetVariantValue(0).ToFloat();
	std::vector<double> maxAttribute = std::vector<double>(); //= minAttribute;

	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		for (int feature = 0; feature < numberOfFeatures; feature++)
		{
			//double currentValue = m_objectTable->GetColumn(feature)->GetVariantValue(row).ToFloat();
			//double currentValue = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(feature)).ToFloat();
			double currentValue = m_objectTable->GetValue(row, feature).ToFloat();

			//initialize values at first fiber
			if (row == 0)
			{
				minAttribute.push_back(currentValue);
				maxAttribute.push_back(currentValue);
			}
			if (minAttribute[feature] > currentValue)
			{
				minAttribute[feature] = currentValue;
			}
			if (maxAttribute[feature] < currentValue)
			{
				maxAttribute[feature] = currentValue;
			}
		}
	}
	for (int feature = 0; feature < numberOfFeatures; feature++)
	{
		std::vector<double> tempVec = std::vector<double>();
		tempVec.push_back(minAttribute[feature]);
		tempVec.push_back(maxAttribute[feature]);
		m_minMaxValues->push_back(tempVec);
	}
}

//! Returns a vector which stores for each fiber its region with the strongest coverage fo every level
//! So every fiber is only stored once for a level!
void iAVRMetrics::calculateMaxCoverageFiberPerRegion()
{
	//Initialize new Vectors
	for (int level = 0; level < m_fiberCoverage->size(); level++)
	{
		//Initialize the region vec for every level
		m_maxCoverage->push_back(std::vector<std::vector<vtkIdType>>());

		for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			//Initialize a vec of IDs for every region
			m_maxCoverage->at(level).push_back(std::vector<vtkIdType>());
		}

	}
	
	for (int level = 0; level < m_fiberCoverage->size(); level++)
	{
		for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
		{
			findBiggestCoverage(level, row);
		}

	}
}

//! Returns the biggest coverage value for a specific fiber over all regions at the given octree level
//! Uses Threads
void iAVRMetrics::findBiggestCoverage(int level, int fiber)
{
	double currentMaxCoverage = 0;
	int regionWithMaxCoverage = 0;

	for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
	{
		auto it = m_fiberCoverage->at(level).at(region)->find(fiber);
		//If fiber has a coverage...
		if (it != m_fiberCoverage->at(level).at(region)->end())
		{
			if (currentMaxCoverage < it->second)
			{
				currentMaxCoverage = it->second;
				regionWithMaxCoverage = region;
			}
		}
	}

	if (currentMaxCoverage > 0)
	{
		m_maxCoverage->at(level).at(regionWithMaxCoverage).push_back(fiber);
	}
}

void iAVRMetrics::calculateJaccardIndex(int level)
{
	for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
	{
		m_jaccardValues->at(level).push_back(std::vector<double>());

		// If only one Region
		if(m_fiberCoverage->at(level).size() == 1)
		{
			m_jaccardValues->at(level).at(region).push_back(1);
			return;
		}

		for (int region2 = 0; region2 < m_fiberCoverage->at(level).size(); region2++)
		{
			auto index = calculateJaccardIndex(level, region, region2);
			m_jaccardValues->at(level).at(region).push_back(index);

			//DEBUG_LOG(QString("jaccardValue for [%1][%2] is %3").arg(region).arg(region2).arg(m_jaccardValues->at(level).at(region).at(region2)));
		}
	}
}

//! Calculates the size of the intersection divided by the size of the union of the chosen regions
//! Gets calculated from the fiber intersection data. Value lies between 0 and 1.
//! Returns 1 if the region are the same
double iAVRMetrics::calculateJaccardIndex(int level, int region1, int region2)
{
	if (region1 == region2) return 1.0;

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	double sizeRegion1 = fibersInRegion1->size();
	double sizeRegion2 = fibersInRegion2->size();

	if (sizeRegion1 == 0 || sizeRegion2 == 0) return 0.0;

	double sizeintersection = 0;

	for (auto fiber : *fibersInRegion1)
	{
		if(fibersInRegion2->count(fiber.first) == 1)
		{
			sizeintersection++;
		}
	}

	double jaccard_index = sizeintersection / (sizeRegion1 + sizeRegion2 - sizeintersection);

	return jaccard_index;
}

//! Measures the dissimilarity (1 - jaccard index)
double iAVRMetrics::calculateJaccardDistance(int level, int region1, int region2)
{
	return 1 - calculateJaccardIndex(level, region1, region2);
}

//! Calculates the maximum Intensity Projection for the chosen feature and direction (x = 0, y = 1, z = 2)
//! Saves the color for the maximum value found by shooting a parallel ray through a cube row.
std::vector<QColor>* iAVRMetrics::calculateMIPColoring(int direction, int level, int feature)
{
	int gridSize = pow(2, level);
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>> *regionsInLine = m_octrees->at(level)->getRegionsInLineOfRay();
	std::vector<QColor>* mipColors = new std::vector<QColor>();
	mipColors->reserve(gridSize * gridSize);

	for (int y = 0; y < gridSize; y++)
	{
		for (int x = 0; x < gridSize; x++)
		{
			double val = 0;
			double rgb[3] = { 0,0,0 };

			for(auto region : regionsInLine->at(direction).at(y).at(x))
			{
				double temp = m_calculatedStatistic->at(level).at(feature).at(region);
				if (val < temp) val = temp;
			}
			m_lut->GetColor(val, rgb);

			mipColors->push_back(QColor(rgb[0]*255, rgb[1] * 255, rgb[2] * 255, m_lut->GetOpacity(val) * 255));
		}
	}

	return mipColors;
}
