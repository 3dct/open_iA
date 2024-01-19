// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <iAGUIModuleInterface.h>
#include <QWidget>
#include "iASlicerImpl.h"
#include "iAFilter.h" 
#include <QSlider>

class iAFilterPreviewModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
	QDialog* dialog;  // Add this line to declare 'dialog' as a member variable
	QStringList parameterNames;
	QList<double> minValues;  // Store the minimum and maximum values of the parameters
	QList<double> maxValues;
	std::shared_ptr<iAFilter> currentFilter;
	QList<QSlider*> sliders; 
	iAMdiChild* child;

public:
	void Initialize() override;
private slots:
	void filterPreview();
	void openSplitView(iASlicerImpl* slicer);
	void updateFilterAndSlicer(iASlicerImpl* slicer);  // Method to update filter and slicer
	void generateLatinHypercubeSamples(int samples, std::vector<std::vector<double>>& samplesMatrix);
};