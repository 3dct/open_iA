// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QColor>

class vtkActor;
class vtkAssembly;
class vtkLookupTable;
class vtkRenderer;
class vtkTextActor3D;

//! This class calculates color transfer functions and LUT for Metrics and allows the display of a color legend
class iAVRColorLegend
{
public:
	iAVRColorLegend(vtkRenderer* renderer);
	//! Creates a LUT with a choosen colorScheme and min/max values
	vtkSmartPointer<vtkLookupTable> createLut(double min, double max, int colorScheme);
	//! Returns for a value its color in the current LUT
	QColor getColor(double value);
	//! Returns a rgba coloring vector for the current LUT for every region in the given octree level for a given feature
	std::vector<QColor> getColors(vtkIdType octreeLevel, int feature, std::vector<std::vector<std::vector<double>>> const & calculatedValues);
	//! Creates a color bar which acts as legend for the current LUT. The color Bar is sized based on the world physicalScale and gets
	//! calculated as plane with uniform division based on the available colors in the LUT. The function also creates the labels for the color sections.
	void calculateLegend(double physicalScale);
	//! Displays the color bar legend and its value labels (add actor to renderer)
	void show();
	//! Hides the color bar legend and its value labels (remove actor to renderer)
	void hide();
	//! Moves the color legend and its value labels to the given pos
	void setPosition(double* pos);
	//! Sets the orientation of the color legend and its value labels to the given coordinates
	void setOrientation(double x, double y, double z);
	//! Resizes the color bar legend and its value labels based on the given scale
	void setScale(double scale);
	//! Sets the title for the header of the color bar legend
	void setTitle(QString title);

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkActor> m_colorBarLegend;
	vtkSmartPointer<vtkAssembly> m_legend;
	vtkSmartPointer<vtkTextActor3D> m_textSource;
	vtkSmartPointer<vtkTextActor3D> m_titleTextSource;
	double m_initialTextOffset;
	double m_titleFieldScale[3];
	double m_textFieldScale[3];
	bool m_colorLegendVisible;

	//! Calculates the Lookuptable (LUT) based on the range from min to max and a given color scheme
	vtkSmartPointer<vtkLookupTable> calculateLUT(double min, double max, std::vector<QColor> colorScheme);
	std::vector<QColor> colorScheme01();
	std::vector<QColor> colorScheme02();
};
