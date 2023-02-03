// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
