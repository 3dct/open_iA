// Copyright (c) open_iA contributors
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
	QSize mainWindowSize;  // Stores the size of the main window
	int slicerWidth;       // Stores the calculated width for slicers
	int slicerHeight;      // Stores the calculated height for slicers
	std::shared_ptr<iAFilter> currentFilter;
	QList<QSlider*> sliders; 
	iAMdiChild* child;

public:
	void Initialize() override;
private slots:
	void filterPreview();
	void openSplitView(iASlicerImpl* slicer, const QVariantMap& originalParamValues);
	void updateFilterAndSlicer(
		iASlicerImpl* slicer, QVariantMap& paramValues);  // Method to update filter and slicer
	void generateLatinHypercubeSamples(int samples, std::vector<std::vector<double>>& samplesMatrix);
};