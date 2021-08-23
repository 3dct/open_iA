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
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkFollower.h>
#include <vtkAssembly.h>
#include <vtkTextActor3D.h>
#include <vtkLookupTable.h>

#include <qcolor.h>

/*
* This class calculates color transfer functions and LUT for Metrics and allows the display of a color legend
*/
class iAVRColorLegend
{
public:
	iAVRColorLegend(vtkRenderer* renderer);
	vtkSmartPointer<vtkLookupTable> createLut(double min, double max, int colorScheme);
	QColor getColor(double value);
	std::vector<QColor>* getColors(int octreeLevel, int feature, std::vector<std::vector<std::vector<double>>>* calculatedValues);
	void calculateLegend(double physicalScale);
	void show();
	void hide();
	void setPosition(double* pos);
	void setOrientation(double x, double y, double z);
	void setScale(double scale);
	void setTitle(QString title);

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkActor> m_colorBarLegend;
	vtkSmartPointer<vtkAssembly> m_legend;
	vtkSmartPointer<vtkTextActor3D> textSource;
	vtkSmartPointer<vtkTextActor3D> titleTextSource;
	double initialTextOffset;
	double titleFieldScale[3];
	double textFieldScale[3];
	bool m_colorLegendVisible;

	vtkSmartPointer<vtkLookupTable> calculateLUT(double min, double max, std::vector<QColor> colorScheme);
	std::vector<QColor> colorScheme01();
	std::vector<QColor> colorScheme02();
};

