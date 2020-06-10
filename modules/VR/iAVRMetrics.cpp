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

#include <vtkVariant.h>

iAVRMetrics::iAVRMetrics(vtkTable* objectTable, iACsvIO io, std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage):m_objectTable(objectTable),
m_io(io), m_fiberCoverage(fiberCoverage)
{
}

//! Calculates the weighted average of every octree region for a given feature at a given octree level
//! Calculates:  1/#Fiber * SUM[#Fiber]( Attribut * weight)
void iAVRMetrics::calculateWeightedAverage(int octreeLevel, int feature)
{
	for(int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
	{
		double metricResultPerRegion;
		int fibersInRegion = 0;

		for (auto element : *m_fiberCoverage->at(octreeLevel).at(region))
		{
			double fiberAttribute = m_objectTable->GetValue(element.first, m_io.getOutputMapping()->value(feature)).ToFloat();
			double weightedAttribute = fiberAttribute * element.second;
			metricResultPerRegion += weightedAttribute;
			fibersInRegion++;
		}
		m_calculatedStatistic->at(feature).at(octreeLevel).at(region) = (metricResultPerRegion/ fibersInRegion);
	}
}

vtkLookupTable * iAVRMetrics::calculateLUT(double min, double max)
{
	vtkSmartPointer<vtkColorTransferFunction> ctf = createColorTransferFunction();
	vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();

	lut->SetNumberOfTableValues(7);
	lut->Build();

	for (size_t i = 0; i < 8; ++i)
	{
		double *rgb;
		rgb = ctf->GetColor(static_cast<double>(i) / 8);
		lut->SetTableValue(i, rgb);
	}
	//Add colors in Method?
	return nullptr;
}

vtkColorTransferFunction* iAVRMetrics::createColorTransferFunction()
{
	vtkSmartPointer<vtkColorTransferFunction> ctf =	vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToDiverging();

	// CREATE some different Color Transferfunctions and add them to a ? vector ?. To let the user choose
	// With int user can select in calculateLUT the color the want to use
	// Color: https://colorbrewer2.org/?type=diverging&scheme=RdYlBu&n=8 

	ctf->AddRGBPoint(0.0, 215 / 255, 48 / 255, 39 / 255);
	ctf->AddRGBPoint(1/7, 244 / 255, 109 / 255, 67 / 255);
	ctf->AddRGBPoint(2/7, 253 / 255, 174 / 255, 97 / 255);
	ctf->AddRGBPoint(3/7, 254 / 255, 224 / 255, 144 / 255);
	ctf->AddRGBPoint(4/7, 224 / 255, 243 / 255, 248 / 255);
	ctf->AddRGBPoint(5/7, 171 / 255, 217 / 255, 233 / 255);
	ctf->AddRGBPoint(6/7, 116 / 255, 173 / 255, 209 / 255);
	ctf->AddRGBPoint(7/7, 69 / 255, 117 / 255, 180 / 255);

	return ctf;
}
