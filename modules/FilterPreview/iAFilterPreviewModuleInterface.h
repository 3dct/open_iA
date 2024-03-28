// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGUIModuleInterface.h>

class iADataSet;
class iAFilter;
class iASlicerImpl;

class QDialog;

class iAFilterPreviewModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private:
	QDialog* dialog;  // Add this line to declare 'dialog' as a member variable
	QStringList parameterNames;
	QList<double> minValues;  // Store the minimum and maximum values of the parameters
	QList<double> maxValues;
	int slicerWidth;       // Stores the calculated width for slicers
	int slicerHeight;      // Stores the calculated height for slicers
	std::shared_ptr<iAFilter> currentFilter;
	std::shared_ptr<iADataSet> inputImg;
	iAMdiChild* child;
private slots:
	void filterPreview();
	void openSplitView(iASlicerImpl* slicer, const QVariantMap& originalParamValues);
	void updateFilterAndSlicer(
		iASlicerImpl* slicer, QVariantMap& paramValues);  // Method to update filter and slicer
	void generateLatinHypercubeSamples(int samples, std::vector<std::vector<double>>& samplesMatrix);
};
