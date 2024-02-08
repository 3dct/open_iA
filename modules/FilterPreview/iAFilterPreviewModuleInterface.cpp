// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterPreviewModuleInterface.h"

#include "iAChannelData.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAFilterRegistry.h"
#include "iATransferFunction.h"
#include "iAVolumeViewer.h"
#include "iAImageData.h"
#include "iASlicerImpl.h"
#include <iALog.h>
#include <iAQSplom.h>
#include <iASPLOMData.h>


#include "vtkColorTransferFunction.h"

#include <QAction>
#include <QLabel>
#include <QSlider>
#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QPushButton>

#include <vector>
#include <random>
#include <algorithm>


void iAFilterPreviewModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	{
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	}
	QAction * actionPreview = new QAction(tr("Run filter with preview"), m_mainWnd);
	connect(actionPreview, &QAction::triggered, this, &iAFilterPreviewModuleInterface::filterPreview);
	m_mainWnd->makeActionChildDependent(actionPreview);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionPreview);
}

//apply filter
void iAFilterPreviewModuleInterface::updateFilterAndSlicer(iASlicerImpl* slicer, QVariantMap& paramValues)
{
	if (!currentFilter)
	{
		return;  // No filter set
	}

	currentFilter->run(paramValues);  // Run the filter with new parameters

	auto outDataSet = currentFilter->outputs()[0];

	auto imgData = dynamic_cast<iAImageData*>(outDataSet.get());
	auto newChannelData = std::make_shared<iAChannelData>("",
		imgData->vtkImage(),  // get image data directly from filter
		// color transfer function still the one from the analysis window:
		dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(child->firstImageDataSetIdx()))->transfer()->colorTF());

	if (slicer->hasChannel(0))
	{
		// Channel with ID 0 exists, so update it
		slicer->updateChannel(0, *newChannelData);
	}
	else
	{
		// Channel with ID 0 does not exist, so add it
		slicer->addChannel(0, *newChannelData, true);
	}

	slicer->setMinimumSize(QSize(slicerWidth, slicerHeight));  // Set a minimum size for visibility
	slicer->resetCamera();    // Update the slicer
}

void iAFilterPreviewModuleInterface::generateLatinHypercubeSamples(int numSamples, std::vector<std::vector<double>>& samplesMatrix)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);

	int dimensions = minValues.size();

	samplesMatrix.resize(dimensions, std::vector<double>(numSamples));

	for (int i = 0; i < dimensions; ++i)
	{
		for (int j = 0; j < numSamples; ++j)
		{
			// Example of using member variables minValues and maxValues
			double range = maxValues[i] - minValues[i];
			samplesMatrix[i][j] = minValues[i] + range * ((j + dis(gen)) / numSamples);
			LOG(lvlDebug, QString("Sample [%1][%2] = %3").arg(i).arg(j).arg(samplesMatrix[i][j]));
		}
		std::shuffle(samplesMatrix[i].begin(), samplesMatrix[i].end(), gen);
	}
}

void iAFilterPreviewModuleInterface::openSplitView(iASlicerImpl* slicer, const QVariantMap& originalParamValues)
{
	if (dialog)
	{
		dialog->close();
	}

	QDialog* splitViewDialog = new QDialog(m_mainWnd);
	splitViewDialog->setWindowTitle(tr("Detailed Filter View"));

	QVBoxLayout* imageLayout = new QVBoxLayout;
	QLabel* imageLabel = new QLabel();
	imageLabel->setAlignment(Qt::AlignCenter);
	imageLabel->setText("IMAGE PREVIEW");
	/*imageLabel->setMinimumSize(200, 200);*/
	imageLayout->addWidget(slicer,9);
	imageLayout->addWidget(imageLabel,1);

	// Create scatter plot matrix widget and set up data iAQSplom* chartsSpmWidget = new iAQSplom();
	iAQSplom* chartsSpmWidget = new iAQSplom();
	auto chartsSpmData = std::make_shared<iASPLOMData>();

	// Convert QStringList to std::vector<QString>
	std::vector<QString> paramNamesVector;
	for (const QString& paramName : parameterNames)
	{
		paramNamesVector.push_back(paramName);
	}
	chartsSpmData->setParameterNames(paramNamesVector);

	// Populate the data vectors with actual parameter values
	// This is just an example, and you would replace it with the actual parameter values
	for (int i = 0; i < parameterNames.size(); ++i)
	{
		chartsSpmData->data()[i].clear();  // Clear the previous data
		chartsSpmData->data()[i].push_back(minValues[i]);
		chartsSpmData->data()[i].push_back(maxValues[i]);
	}

	chartsSpmData->updateRanges();
	std::vector<char> columnVisibility(parameterNames.size(), true);
	chartsSpmWidget->showAllPlots(false);
	chartsSpmWidget->setData(chartsSpmData, columnVisibility);
	chartsSpmWidget->setHistogramVisible(false);
	chartsSpmWidget->setPointRadius(2);
	chartsSpmWidget->setMinimumWidth(400);
	chartsSpmWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QVariantMap paramValues = originalParamValues; //Make a copy of the parameter values to allow modifications
	
	connect(chartsSpmWidget, &iAQSplom::chartClick, this,
		[this,slicer, chartsSpmData, chartsSpmWidget, columnVisibility, &paramValues](
			size_t paramX, size_t paramY, double x, double y, Qt::KeyboardModifiers modifiers)
		{
			LOG(lvlInfo,
				QString("params : %1...%2")
					.arg(paramValues[parameterNames[paramX]].toString())
					.arg(paramValues[parameterNames[paramY]].toString()));
			paramValues[parameterNames[paramX]] = x;
			chartsSpmData->data()[paramX][1] = x;
			paramValues[parameterNames[paramY]] = y;
			chartsSpmData->data()[paramY][1] = y;
			chartsSpmWidget->setData(chartsSpmData, columnVisibility);

			this->updateFilterAndSlicer(slicer, paramValues);

		});

	
	
	// Add scatter plot matrix widget to layout
	QVBoxLayout* controlLayout = new QVBoxLayout;
	controlLayout->addWidget(chartsSpmWidget);

	// Set up the main layout
	/*QHBoxLayout* mainLayout = new QHBoxLayout(splitViewDialog);
	mainLayout->addLayout(imageLayout,5);
	mainLayout->addLayout(controlLayout,5);*/

	QHBoxLayout* splitLayout = new QHBoxLayout;
	splitLayout->addLayout(imageLayout, 5);
	splitLayout->addLayout(controlLayout, 5);


	QHBoxLayout* imageListLayout = new QHBoxLayout;

	for (int i = 0; i < 5; ++i)
	{
		QLabel* placeholderLabel = new QLabel();
		placeholderLabel->setAlignment(Qt::AlignCenter);
		placeholderLabel->setText(QString("Placeholder %1").arg(i + 1));  // Numbered placeholders from 1 to 5
		placeholderLabel->setMinimumSize(100, 100);                       // Set a minimum size for the label

		imageListLayout->addWidget(placeholderLabel, 2);
	}

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(splitLayout,7);
	mainLayout->addLayout(imageListLayout,3);

	QHBoxLayout* mainLayoutWithParams = new QHBoxLayout;

	// This will only be useful for debugging, afterwards, it will be deleted

	QString paramDisplay;
	for (const auto& paramName : parameterNames)
	{
		if (paramValues.contains(paramName))  // Make sure the parameter is in paramValues
		{
			paramDisplay += paramName + ": " + QString::number(paramValues[paramName].toDouble()) + "\n";
		}
	}

	// Create a label for displaying the parameter values
	QLabel* paramLabel = new QLabel(paramDisplay);
	mainLayoutWithParams->addLayout(mainLayout, 5);
	mainLayoutWithParams->addWidget(paramLabel, 5);  // Use controlsAndParamsLayout here




	splitViewDialog->setLayout(mainLayoutWithParams);
	splitViewDialog->exec();
}


void iAFilterPreviewModuleInterface::filterPreview()
{
	//QMessageBox::information(m_mainWnd, "Filter with preview", "You will be able to run image filters with a preview here soon!");

	QStringList validParameterNames = {"Alpha", "Beta", "Radius", "Number of histogram bins", "Samples", "Levels",
		"Control points", "Spline order", "Mean", "Standard deviation", "Range sigma", "Domain sigma",
		"Lower threshold", "Upper threshold", "Outside value", "Inside value", "Variance", "Maximum error",
		"Maximum RMS error", "Number of iterations", "Advection scaling", "Propagation scaling", "Curvature scaling",
		"Iso surface value", "Time step", "Conductance", "Time step", "Order", "Radius", "Maximum Iterations",
		"Number of Classes", "Number of Threads", "Outside value", "Time step", "Conductance", "Direction",
		"Order of Accuracy", "Window Minimum", "Window Maximum", "Output Minimum", "Output Maximum",
		"Maximum Iterations", "Number of Classes", "StructRadius X", "StructRadius Y", "StructRadius Z", "Sigma",
		"Maximum RMS error", "Propagation scaling", "Curvature scaling", "Iso surface value", "Width of histogram bin",
		"Low intensity", "Kernel radius X", "Kernel radius Y", "Kernel radius Z", "Patch radius", "Outside value",
		"Lower X padding", "Lower Y padding", "Lower Z padding", "Upper X padding", "Upper Y padding",
		"Upper Z padding", "Value", "Sigma", "Output Minimum", "Output Maximum", "Probability", "Distance Threshold",
		"Shift", "Scale", "Scale", "Sigma", "Level", "Threshold", "Background value", "Foreground value"};

	// list available filters:
	auto const& filterFactories = iAFilterRegistry::filterFactories();
	QStringList filterNames;
	for (auto factory : filterFactories)
	{
		auto filter = factory();
		QStringList filterParameterNames;

		for (auto const& p : filter->parameters())
		{
			if (validParameterNames.contains(p->name()))
			{
				filterParameterNames.append(p->name());
			}
		}

		if (!filterParameterNames.isEmpty())
		{
			// Only add the filter name if it has valid parameters
			filterNames.append(filter->name());
		}
	}

	bool ok;
	QString filterName =
		QInputDialog::getItem(m_mainWnd, tr("Select filter"), tr("Filter:"), filterNames, 0, false, &ok);
	if (!ok)
	{
		return;
	}

	// run a specific filter:
	//auto filter = iAFilterRegistry::filter("Discrete Gaussian");
	currentFilter = iAFilterRegistry::filter(filterName);
	child = m_mainWnd->activeMdiChild();
	
	currentFilter->addInput(child->dataSet(child->firstImageDataSetIdx()));
	

	if (filterName.isEmpty())
	{
		QMessageBox::critical(m_mainWnd, "Error", "No filter selected");
		return;
	}

	// Clear previous values
	parameterNames.clear();
	minValues.clear();
	maxValues.clear();

	for (auto const& p : currentFilter->parameters())
	{
		if (validParameterNames.contains(p->name()))
		{
			parameterNames.append(p->name());
			double defaultVal = p->defaultValue().toDouble();
			if (p->min() != std::numeric_limits<double>::lowest())
			{
				minValues.append(p->min());
			}
			else
			{
				minValues.append(defaultVal == 0 ? -1 : defaultVal / 2);
			}

			if (p->max() != std::numeric_limits<double>::max())
			{
				maxValues.append(p->max());
			}
			else
			{
				maxValues.append(defaultVal == 0 ? 1 : defaultVal * 3 / 2);
			}
			
		}
	}


	dialog = new QDialog(m_mainWnd);
	dialog->setWindowTitle(filterName + tr(" Filter Preview"));

	// Create a grid layout to hold the preview slicers
	QGridLayout* gridLayout = new QGridLayout;

	// Get the size of the main window and calculate the relative size for the slicers
	mainWindowSize = m_mainWnd->size();
	slicerWidth = mainWindowSize.width() / 5;    
	slicerHeight = mainWindowSize.height() / 5;  

	int numSamples = 9;
	std::vector<std::vector<double>> lhsSamples;
	generateLatinHypercubeSamples(numSamples, lhsSamples);
	// Generate the 3x3 matrix of slicers
	int counter = 0;
	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; ++col)
		{
			QWidget* container = new QWidget(dialog);
			QVBoxLayout* layout = new QVBoxLayout(container); //main layout
			QHBoxLayout* hLayout = new QHBoxLayout(); // This will contain the slicer and the parameter values

			iASlicerImpl* slicer = new iASlicerImpl(container, iASlicerMode::XY, false);

			QVariantMap paramValues;
			for (int i = 0; i < parameterNames.size(); ++i)
			{
				paramValues[parameterNames[i]] =
					lhsSamples[i][counter];  // Map the parameter to its corresponding value
				LOG(lvlDebug, QString("Param Name: %1").arg(parameterNames[i]));
				LOG(lvlDebug, QString("Sample [%1][%2] = %3").arg(counter).arg(i).arg(lhsSamples[i][counter]));

				//paramValues[parameterNames[i]] = sliders[i]->value();
			}

			currentFilter->run(paramValues);

			// Create a string to display the parameter values
			QString paramDisplay;
			for (const auto& paramName : parameterNames)
			{
				paramDisplay += paramName + ": " + QString::number(paramValues[paramName].toDouble()) + "\n";
			}

			// Create a label to show the parameter values
			QLabel* paramLabel = new QLabel(paramDisplay, container);

			updateFilterAndSlicer(slicer, paramValues);

			// Overlay the TransparentClickableWidget on top of the slicer
			QPushButton* selectButton = new QPushButton("Select", container);
			connect(selectButton, &QPushButton::clicked, [this, slicer, paramValues]() { this->openSplitView(slicer, paramValues); });

			hLayout->addWidget(slicer);      // Add the slicer to the horizontal layout
			hLayout->addWidget(paramLabel);  // Add the label to the horizontal layout

			layout->addLayout(hLayout);
			layout->addWidget(selectButton);  // Add the "Select" button below the slicer

			gridLayout->addWidget(container, row, col);

			counter++;
			
		}
	}

	// Set the layout of the dialog to the grid layout
	dialog->setLayout(gridLayout);

	// Show the dialog
	dialog->exec();











	/*
	
	filter->addInput(child->dataSet(child->firstImageDataSetIdx()));
	

	

	QLabel* imageLabel = new QLabel(dialog);
	imageLabel->setAlignment(Qt::AlignCenter);
	//imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	imageLabel->setText("IMAGE PREVIEW");

	iASlicerImpl* slicer = new iASlicerImpl(dialog, iASlicerMode::XY);
	iAChannelData* channel =
		new iAChannelData("", child->firstImageData(), dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(child->firstImageDataSetIdx()))->transfer()->colorTF());

	slicer->addChannel(0,*channel, true);
	slicer->resetCamera();
	slicer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	QVBoxLayout* imageLayout = new QVBoxLayout;
	imageLayout->addWidget(slicer);
	imageLayout->addWidget(imageLabel);


	QVBoxLayout* sliderLayout = new QVBoxLayout;
	QList<QSlider*> sliders;  // Use a list to hold all the sliders for later access

	for (int i = 0; i < parameterNames.size(); ++i)
	{
		QSlider* slider = new QSlider(Qt::Horizontal, dialog);

		slider->setRange(1, 99);  // All sliders will have the same 0 to 100 range
		slider->setValue(50);      // All sliders will start at the midpoint
		sliders.append(slider);

		sliderLayout->addWidget(new QLabel(parameterNames[i], dialog));  // Use the parameter name as label
		sliderLayout->addWidget(slider);
	}

	QHBoxLayout* mainLayout = new QHBoxLayout(dialog);
	mainLayout->addLayout(imageLayout);
	mainLayout->addLayout(sliderLayout);

	dialog->exec();

	//QVariantMap paramValues;
	//paramValues["Variance"] = slider1->value();
	//double slide2Val = slider2->value();
	//paramValues["Maximum error"] = double(slide2Val / 100);
	//paramValues["Convert back to input type"] = false;
	//filter->run(paramValues);

	//filter->parameters();

	//Apply parameter values to filter

	QVariantMap paramValues;
	for (int i = 0; i < parameterNames.size(); ++i)
	{
		// Map the slider value (from 0 to 100) to the parameter's range (from min to max)
		double mappedValue = minValues[i] + (maxValues[i] - minValues[i]) * (double(sliders[i]->value()) / 100);
		paramValues[parameterNames[i]] = mappedValue;  // Map the parameter to its corresponding value
		LOG(lvlDebug, QString("Param Name: %1").arg(parameterNames[i]));
		LOG(lvlDebug, QString("Param Mapped Value: %1").arg(mappedValue));
		LOG(lvlDebug, QString("Min Value: %1").arg(minValues[i]));
		LOG(lvlDebug, QString("Max Value: %1").arg(maxValues[i]));

		//paramValues[parameterNames[i]] = sliders[i]->value();
	}

	filter->run(paramValues);


	for (auto o : filter->outputs())
	{
		child->addDataSet(o);
	}
	
	iAChannelData* channelMod = new iAChannelData("", child->firstImageData(),
		dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(child->firstImageDataSetIdx()))->transfer()->colorTF());

	//slicer->addChannel(0, *channelMod, true);
	slicer->updateChannel(0, *channelMod);
	imageLabel->setText("IMAGE PREVIEW DONE!");*/
	
	

}



