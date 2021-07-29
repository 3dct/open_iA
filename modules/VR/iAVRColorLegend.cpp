/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAVRColorLegend.h"

#include <iALog.h>

#include <vtkPolyData.h>
#include <vtkCellData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkPlaneSource.h>

iAVRColorLegend::iAVRColorLegend(vtkRenderer* renderer) :m_renderer(renderer)
{
	titleTextSource = vtkSmartPointer<vtkTextActor3D>::New();
	textSource = vtkSmartPointer<vtkTextActor3D>::New();
	m_colorBarLegend = vtkSmartPointer<vtkActor>::New();
	m_legend = vtkSmartPointer<vtkAssembly>::New();

	//Initialize LUT for min/max = 0
	createLut(0, 0, 1);

	m_colorLegendVisible = false;
}

//! Creates a LUT with a choosen colorScheme and min/max values
vtkSmartPointer<vtkLookupTable> iAVRColorLegend::createLut(double min, double max, int colorScheme)
{
	std::vector<QColor> scheme;

	switch (colorScheme)
	{
	case 1:
		scheme = colorScheme01();
		break;
	case 2:
		scheme = colorScheme02();
		break;
	default:
		scheme = colorScheme01();
		LOG(lvlWarn, QString("No valid color scheme selected - used scheme 1"));
		break;
	}

	return calculateLUT(min, max, scheme);
}

//! Returns for a value its color in the current LUT
QColor iAVRColorLegend::getColor(double value)
{
	double rgba[3] = { 0,0,0 };
	m_lut->GetColor(value, rgba);

	QColor color = QColor();
	color.setRgbF(rgba[0], rgba[1], rgba[2], m_lut->GetOpacity(value));

	return color;
}

//! Returns a rgba coloring vector for the current LUT for every region in the given octree level for a given feature
std::vector<QColor>* iAVRColorLegend::getColors(int octreeLevel, int feature, std::vector<std::vector<std::vector<double>>>* calculatedValues)
{
	std::vector<QColor>* colors = new std::vector<QColor>(calculatedValues->at(octreeLevel).at(feature).size(), QColor());

	for (int region = 0; region < calculatedValues->at(octreeLevel).at(feature).size(); region++)
	{
		double rgba[3] = { 0,0,0 };
		//double val = histogramNormalization(m_calculatedAverage->at(octreeLevel).at(feature).at(region),0,1,min,max);
		double val = calculatedValues->at(octreeLevel).at(feature).at(region);
		m_lut->GetColor(val, rgba);

		colors->at(region).setRgbF(rgba[0], rgba[1], rgba[2], m_lut->GetOpacity(val));
	}

	return colors;
}

//! Creates a color bar which acts as legend for the current LUT. The color Bar is sized based on the world physicalScale and gets
//! calculated as plane with uniform division based on the available colors in the LUT. The function also creates the labels for the color sections.
void iAVRColorLegend::calculateLegend(double physicalScale)
{
	auto width = physicalScale * 0.05; //5%
	auto height = physicalScale * 0.15; //15%

	//Remove old colorBar
	hide();

	vtkSmartPointer<vtkPlaneSource> colorBarPlane = vtkSmartPointer<vtkPlaneSource>::New();
	colorBarPlane->SetXResolution(1);
	colorBarPlane->SetYResolution(m_lut->GetNumberOfAvailableColors());

	colorBarPlane->SetOrigin(0, 0, 0.0);
	colorBarPlane->SetPoint1(width, 0, 0.0); //width
	colorBarPlane->SetPoint2(0, height, 0.0); // height
	colorBarPlane->Update();

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(4);

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
	
	m_colorBarLegend = vtkSmartPointer<vtkActor>::New();
	m_colorBarLegend->SetMapper(mapper);
	m_colorBarLegend->GetProperty()->EdgeVisibilityOn();
	m_colorBarLegend->GetProperty()->SetLineWidth(3);

	//title
	titleTextSource = vtkSmartPointer<vtkTextActor3D>::New();
	titleTextSource->GetTextProperty()->SetColor(0, 0, 0);
	titleTextSource->GetTextProperty()->SetBackgroundColor(0.6, 0.6, 0.6);
	titleTextSource->GetTextProperty()->SetBackgroundOpacity(1.0);
	titleTextSource->GetTextProperty()->SetFontSize(30);

	// Create text
	textSource = vtkSmartPointer<vtkTextActor3D>::New();
	textSource->SetInput(text.toUtf8());
	textSource->GetTextProperty()->SetColor(0, 0, 0);
	textSource->GetTextProperty()->SetBackgroundColor(0.6, 0.6, 0.6);
	textSource->GetTextProperty()->SetBackgroundOpacity(1.0);
	textSource->GetTextProperty()->SetFontSize(19);

	double actorBounds[6];
	m_colorBarLegend->GetBounds(actorBounds);

	initialTextOffset = physicalScale * 0.001;// 0.000021;

	textSource->SetPosition(actorBounds[1] + initialTextOffset, actorBounds[2] + initialTextOffset, actorBounds[4]);
	textSource->SetScale(physicalScale * 0.0005, physicalScale * 0.001, 1);

	titleTextSource->SetPosition(actorBounds[0], actorBounds[3] + initialTextOffset, actorBounds[4]);
	titleTextSource->SetScale(physicalScale * 0.0008, physicalScale * 0.00085, 1);

	for (int i = 0; i < 3; i++)
	{
		titleFieldScale[i] = titleTextSource->GetScale()[i];
		textFieldScale[i] = textSource->GetScale()[i];
	}

	m_legend = vtkSmartPointer<vtkAssembly>::New();
	m_legend->AddPart(m_colorBarLegend);
	m_legend->AddPart(titleTextSource);
	m_legend->AddPart(textSource);
	m_legend->Modified();
}

//! Displays the color bar legend and its value labels
void iAVRColorLegend::show()
{
	if (m_colorLegendVisible)
	{
		return;
	}
	m_renderer->AddActor(m_legend);
	m_colorLegendVisible = true;
}

//! Hides the color bar legend and its value labels
void iAVRColorLegend::hide()
{
	if (!m_colorLegendVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_legend);
	m_colorLegendVisible = false;
}

//! Moves the color legend and its value labels to the given pos
void iAVRColorLegend::setPosition(double* pos)
{
	m_legend->SetPosition(pos);
}

//! Sets the orientation of the color legend and its value labels to the given coordinates
void iAVRColorLegend::setOrientation(double x, double y, double z)
{
	m_legend->SetOrientation(x, y, z);
}

//! Resizes the color bar legend and its value labels based on the given scale
void iAVRColorLegend::setScale(double scale)
{
	m_legend->SetScale(scale);
}

//! Sets the title for the header of the color bar legend
void iAVRColorLegend::setTitle(QString title)
{
	titleTextSource->SetInput(title.toUtf8());
}

//! Calculates the Lookuptable (LUT) based on the range from min to max and a given color scheme
vtkSmartPointer<vtkLookupTable> iAVRColorLegend::calculateLUT(double min, double max, std::vector<QColor> colorScheme)
{
	auto schemeSize = colorScheme.size();
	m_lut = vtkSmartPointer<vtkLookupTable>::New();

	m_lut->SetNumberOfTableValues(schemeSize);
	m_lut->Build();

	for (size_t i = 0; i < schemeSize; i++)
	{
		auto temp = schemeSize - 1 - i;
		m_lut->SetTableValue(i, colorScheme.at(temp).redF(), colorScheme.at(temp).greenF(), colorScheme.at(temp).blueF());
	}

	m_lut->SetTableRange(min, max);
	m_lut->SetBelowRangeColor(1, 1, 1, 1);
	m_lut->UseBelowRangeColorOn();

	return m_lut;
}

// Predefined 8 color diverging Scheme: https://colorbrewer2.org/?type=diverging&scheme=RdYlBu&n=8
std::vector<QColor> iAVRColorLegend::colorScheme01()
{
	std::vector<QColor> colorScheme = std::vector<QColor>(8, QColor());

	colorScheme.at(0).setRgb(215, 48, 39, 255);
	colorScheme.at(1).setRgb(244, 109, 67, 255);
	colorScheme.at(2).setRgb(253, 174, 97, 255);
	colorScheme.at(3).setRgb(254, 224, 144, 255);
	colorScheme.at(4).setRgb(224, 243, 248, 255);
	colorScheme.at(5).setRgb(171, 217, 233, 255);
	colorScheme.at(6).setRgb(116, 173, 209, 255);
	colorScheme.at(7).setRgb(69, 117, 180, 255);

	return colorScheme;
}

// Predefined 4 color qualitative Scheme: https://colorbrewer2.org/?type=diverging&scheme=RdYlBu&n=8#type=qualitative&scheme=Set1&n=4
std::vector<QColor> iAVRColorLegend::colorScheme02()
{
	std::vector<QColor> colorScheme = std::vector<QColor>(4, QColor());

	colorScheme.at(0).setRgb(228, 26, 28, 255);
	colorScheme.at(1).setRgb(55, 126, 184, 255);
	colorScheme.at(2).setRgb(77, 175, 74, 255);
	colorScheme.at(3).setRgb(152, 78, 163, 255);

	return colorScheme;
}