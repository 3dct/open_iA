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

	m_colorBar = vtkSmartPointer<vtkScalarBarActor>::New();
	m_colorBarVisible = false;
}

//! Has to be called *before* getting any Metric data
void iAVRMetrics::setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage)
{
	m_fiberCoverage = fiberCoverage;
}

//! Calculates the weighted average of every octree region for a given feature at a given octree level.
//! In the first call the metric has to calculate the values, for later calls the metric is saved
//! Calculates:  1/N * SUM[N]( Attribut * weight)  with N = all Fibers
void iAVRMetrics::calculateWeightedAverage(int octreeLevel, int feature)
{
	if(!isAlreadyCalculated->at(octreeLevel).at(feature)){
		
		for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
		{
			double metricResultPerRegion = 0;
			int fibersInRegion = 0;

			for (auto element : *m_fiberCoverage->at(octreeLevel).at(region))
			{
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
			m_calculatedStatistic->at(octreeLevel).at(feature).push_back(metricResultPerRegion / fibersInRegion);
		}
		isAlreadyCalculated->at(octreeLevel).at(feature) = true;
	}
}

//! Returns a rgba coloring (between 0 and 1) vector for every region in the current octree level
std::vector<std::vector<double>>* iAVRMetrics::getHeatmapColoring(int octreeLevel, int feature)
{
	std::vector<std::vector<double>>* heatmapColors = new std::vector<std::vector<double>>();
	calculateWeightedAverage(octreeLevel, feature);

	storeMinMaxValues();

	double min =  m_minMaxValues->at(feature).at(0);
	double max = m_minMaxValues->at(feature).at(1);

	calculateLUT(min, max, 8); //8 Colors
	
	for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
	{
		std::vector<double> temp = std::vector<double>();
		double rgba[4] = {0,0,0,1};
		//double val = histogramNormalization(m_calculatedStatistic->at(octreeLevel).at(feature).at(region),0,1,min,max);
		double val = m_calculatedStatistic->at(octreeLevel).at(feature).at(region);

		m_lut->GetColor(val, rgba);
		rgba[3] = m_lut->GetOpacity(val);

		for (int i = 0; i < 4; i++)
		{
			temp.push_back(rgba[i]);
			//temp.push_back(m_lut->MapValue(val)[i]/255);
		}
		heatmapColors->push_back(temp);
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

void iAVRMetrics::calculateColorBarLegend()
{
	m_colorBar->SetLookupTable(m_lut);
	m_colorBar->SetNumberOfLabels(8);
	m_colorBar->DrawBelowRangeSwatchOn();
	m_colorBar->DrawBackgroundOn();
	m_colorBar->DrawFrameOn();
	m_colorBar->GetFrameProperty()->SetLineWidth(5);
	m_colorBar->GetFrameProperty()->SetColor(0.4, 0.4, 0.4);
	m_colorBar->SetTextPad(2);
	m_colorBar->SetVerticalTitleSeparation(2);
	m_colorBar->GetBackgroundProperty()->SetColor(0.4, 0.4, 0.4);
	m_colorBar->GetProperty()->SetLineWidth(4);
	m_colorBar->GetProperty()->SetColor(1, 1, 1);
	m_colorBar->GetTitleTextProperty()->SetColor(1, 1, 1);
	m_colorBar->GetTitleTextProperty()->SetFontSize(10);
	m_colorBar->SetHeight(0.3);

	m_colorBar->Modified();
}

vtkSmartPointer<vtkScalarBarActor> iAVRMetrics::getColorBar()
{
	return m_colorBar;
}

void iAVRMetrics::setColorBarLegendTitle(const char* title)
{
	m_colorBar->SetTitle(title);
}

void iAVRMetrics::showColorBarLegend()
{
	if (m_colorBarVisible)
	{
		return;
	}
	m_renderer->AddActor2D(m_colorBar);
	m_colorBarVisible = true;
}

void iAVRMetrics::hideColorBarLegend()
{
	if (!m_colorBarVisible)
	{
		return;
	}
	m_renderer->RemoveActor2D(m_colorBar);
	m_colorBarVisible = false;
}

int iAVRMetrics::getNumberOfFeatures()
{
	return numberOfFeatures;
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

QString iAVRMetrics::getFeatureName(int feature)
{
	//QString featureName = m_objectTable->GetColumnName(m_io.getOutputMapping()->value(feature));
	QString featureName = m_objectTable->GetColumnName(feature);
	return featureName;
}

