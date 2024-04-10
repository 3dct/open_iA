// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterPreviewModuleInterface.h"

#include <iAChannelData.h>
#include <iAColorTheme.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAQSplom.h>
#include <iASlicerImpl.h>
#include <iASPLOMData.h>
#include <iAToolsITK.h>
#include <iATransferFunction.h>
#include <iAVolumeViewer.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>

#include <QAction>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>
#include <random>


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
		slicer->update();
		LOG(lvlInfo, QString("Channel Updated"));
	}
	else
	{
		// Channel with ID 0 does not exist, so add it
		slicer->addChannel(0, *newChannelData, true);
	}

	slicer->setSliceNumber(0);
	slicer->setMinimumSize(QSize(slicerWidth, slicerHeight));  // Set a minimum size for visibility
	slicer->resetCamera();    // Update the slicer
}

void iAFilterPreviewModuleInterface::generateLatinHypercubeSamples(int numSamples, std::vector<std::vector<double>>& samplesMatrix)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);

	auto dimensions = minValues.size();

	samplesMatrix.resize(dimensions, std::vector<double>(numSamples));

	for (qsizetype i = 0; i < dimensions; ++i)
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
	paramNamesVector.push_back("Color");
	chartsSpmData->setParameterNames(paramNamesVector);

	// Populate the data vectors with actual parameter values
	// This is just an example, and you would replace it with the actual parameter values
	for (int i = 0; i < parameterNames.size(); ++i)
	{
		chartsSpmData->data()[i].clear();  // Clear the previous data
		chartsSpmData->data()[i].push_back(minValues[i]);
		chartsSpmData->data()[i].push_back(maxValues[i]);
	}
	// one distinct value for each data point:
	chartsSpmData->data()[parameterNames.size()].push_back(0);
	chartsSpmData->data()[parameterNames.size()].push_back(1);

	chartsSpmData->updateRanges();
	// there is an additional column now, make space for it in the visibility vector as well:
	std::vector<char> columnVisibility(parameterNames.size()+1, true);
	columnVisibility[parameterNames.size()] = false;  // and make sure the "color" data is not visible
	chartsSpmWidget->showAllPlots(false);
	chartsSpmWidget->setData(chartsSpmData, columnVisibility);
	chartsSpmWidget->setHistogramVisible(false);
	//chartsSpmWidget->settings.enableColorSettings = true;  // to be able to see and change color settings also from settings dialog
	chartsSpmWidget->setColorParam("Color");// using a "qualitative" color scheme, i.e., distinct colors for each integer value:
	chartsSpmWidget->setColorParameterMode(iAQSplom::pmQualitative);
	// using the "Set1" color scheme from Color Brewer (https://colorbrewer2.org/)
	auto ColorThemeName = "Brewer Set1 (max. 9)";
	// for a list of other available color themes, see libs/base/iAColorTheme.cpp:
	// the themes are defined in the iAColorThemeManager constructor
	// to use the colors elsewhere, use:
	auto colorTheme = iAColorThemeManager::instance().theme(ColorThemeName);
	chartsSpmWidget->setColorThemeQual(ColorThemeName);
	
	// improve point visibilility:
	chartsSpmWidget->setPointOpacity(1.0);  // since we have only two points, set them to fully opaque
	chartsSpmWidget->setPointRadius(4.0);   // and make them a little larger
	chartsSpmWidget->setMinimumWidth(400);

	chartsSpmWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QVariantMap paramValues = originalParamValues; //Make a copy of the parameter values to allow modifications

	// Add scatter plot matrix widget to layout
	QVBoxLayout* controlLayout = new QVBoxLayout;
	controlLayout->addWidget(chartsSpmWidget);

	QHBoxLayout* splitLayout = new QHBoxLayout;
	splitLayout->addLayout(imageLayout, 1);
	splitLayout->addLayout(controlLayout, 3);

	QHBoxLayout* imageListLayout = new QHBoxLayout;

	std::vector<iASlicerImpl*> slicerCopies; //Define a container for slicer copies
	std::vector<QVariantMap> slicerParameters;  // New container for parameters

	for (int i = 0; i < 5; ++i)
	{
		iASlicerImpl* slicerCopy = new iASlicerImpl(splitViewDialog, slicer->mode(), false);
		QVariantMap slicerParamValues = paramValues;

		// Container to hold both the slicer and the button
		QWidget* container = new QWidget();
		QVBoxLayout* containerLayout = new QVBoxLayout(container);  // Use QVBoxLayout to stack the slicer and button
	
		for (int j = 0; j < parameterNames.size(); ++j)
		{
			if (i == 0)
			{
				// Use minValues for the first slicer
				slicerParamValues[parameterNames[j]] = minValues[j];
			}
			else if (i == 4)
			{
				// Use maxValues for the last slicer
				slicerParamValues[parameterNames[j]] = maxValues[j];
			}
			else
			{
				// Use the midpoint between minValues and maxValues for other slicers
				auto midpointValue = minValues[j] + (maxValues[j] - minValues[j]) / 2.0;
				slicerParamValues[parameterNames[j]] = midpointValue;
			}
		}

		updateFilterAndSlicer(slicerCopy, slicerParamValues);
		slicerCopies.push_back(slicerCopy);
		slicerParameters.push_back(slicerParamValues);  // Store the corresponding parameters

		containerLayout->addWidget(slicerCopy);

		// Create a button and add it to the container's layout
		QPushButton* selectButton = new QPushButton("Select", container);
		containerLayout->addWidget(selectButton);  // This adds the button below the slicer in the container

		// Capture i by value to know which slicer's parameters to update
		connect(selectButton, &QPushButton::clicked,
			[this, i, chartsSpmData, chartsSpmWidget, columnVisibility, slicer, &slicerCopies, &slicerParameters,
				&paramValues]()
			{
				// Assume slicerParamOldValues accurately represents the old parameter state
				QVariantMap paramOldValues = paramValues;  // Last slicer's parameters as "old" values
				paramValues = slicerParameters[i];

				// Direct comparison for simplicity; refine as needed
				bool isDifferent = false;
				for (const QString& key : paramValues.keys())
				{
					if (paramValues[key] != paramOldValues.value(key))
					{
						isDifferent = true;
						break;
					}
				}

				if (!isDifferent)
				{
					return;  // No changes detected, so do nothing
				}

				// Changes detected; proceed with updates

				for (int j = 0; j < parameterNames.size(); ++j)
				{
					const QString& paramName = parameterNames[j];
					if (paramValues.contains(paramName))
					{
						double value = paramValues[paramName].toDouble();
						chartsSpmData->data()[j][0] = chartsSpmData->data()[j][1];
						chartsSpmData->data()[j][1] = value;
					}
				}
				chartsSpmWidget->setData(chartsSpmData, columnVisibility);

				this->updateFilterAndSlicer(slicer, paramValues);

				if (!slicerCopies.empty())
				{
					for (size_t k = 0; k < slicerCopies.size(); ++k)
					{
						iASlicerImpl* slicerCopy = slicerCopies[k];  // Access the slicer copy by index

						if (k == 0)
						{
							// Now, update the slicerCopy with the new paramValues
							updateFilterAndSlicer(slicerCopy, paramOldValues);
							slicerParameters[k] = paramOldValues;
						}
						else if (k == 4)
						{
							updateFilterAndSlicer(slicerCopy, paramValues);
							slicerParameters[k] = paramValues;
						}
						else
						{
							// Interpolate or adjust values for slicers in between
							QVariantMap interpolatedParamValues;
							for (const QString& paramName : parameterNames)
							{
								bool isIncreasing = paramValues[paramName].toDouble() >
									paramOldValues[paramName].toDouble();
								double startValue = paramOldValues[paramName].toDouble();
								double endValue = paramValues[paramName].toDouble();
								double range = std::abs(endValue - startValue);
								double step = range /
									4;  // Assuming 5 steps (0 to 4), adjust the denominator according to your total steps

								double interpolatedValue;
								if (isIncreasing)
								{
									interpolatedValue = startValue + step * k;  // For increasing values
								}
								else
								{
									interpolatedValue = startValue - step * k;  // For decreasing values
								}

								// Ensure interpolatedValue does not exceed the bounds
								if (isIncreasing)
								{
									interpolatedValue = std::min(interpolatedValue, endValue);
								}
								else
								{
									interpolatedValue = std::max(interpolatedValue, endValue);
								}

								interpolatedParamValues[paramName] = interpolatedValue;
							}

							// Now, update the slicerCopy with the interpolated paramValues
							updateFilterAndSlicer(slicerCopy, interpolatedParamValues);
							slicerParameters[k] = interpolatedParamValues;
						}
					}
				}
			});

		// Now add the container to your overall layout
		imageListLayout->addWidget(container);
	}

	slicerCopies[0]->setBackground(colorTheme->color(0));
	slicerCopies[4]->setBackground(colorTheme->color(1));

	connect(chartsSpmWidget, &iAQSplom::chartClick, this,
		[this, slicer, &slicerCopies, &slicerParameters, chartsSpmData, chartsSpmWidget, columnVisibility,
			&paramValues](
			size_t paramX, size_t paramY, double x, double y, Qt::KeyboardModifiers modifiers)
		{
			Q_UNUSED(modifiers);
			LOG(lvlInfo,
				QString("params : %1...%2")
					.arg(paramValues[parameterNames[paramX]].toString())
					.arg(paramValues[parameterNames[paramY]].toString()));

			QVariantMap paramOldValues = paramValues;

			paramValues[parameterNames[paramX]] = x;
			double x_previous = chartsSpmData->data()[paramX][1];
			chartsSpmData->data()[paramX][0] = x_previous;  //Update points in the chart
			chartsSpmData->data()[paramX][1] = x;
			paramValues[parameterNames[paramY]] = y;
			double y_previous = chartsSpmData->data()[paramY][1];
			chartsSpmData->data()[paramY][0] = y_previous;
			chartsSpmData->data()[paramY][1] = y;
			chartsSpmWidget->setData(chartsSpmData, columnVisibility);

			this->updateFilterAndSlicer(slicer, paramValues);

			if (!slicerCopies.empty())
			{

				for (size_t i = 0; i < slicerCopies.size(); ++i)
				{
					iASlicerImpl* slicerCopy = slicerCopies[i];  // Access the slicer copy by index

					if (i == 0)
					{
						// Now, update the slicerCopy with the new paramValues
						updateFilterAndSlicer(slicerCopy, paramOldValues);
						slicerParameters[i] = paramOldValues;
					}
					else if (i == 4)
					{
						updateFilterAndSlicer(slicerCopy, paramValues);
						slicerParameters[i] = paramValues;
					}
					else
					{
						// Interpolate or adjust values for slicers in between
						QVariantMap interpolatedParamValues;
						for (const QString& paramName : parameterNames)
						{
							bool isIncreasing = paramValues[paramName].toDouble() >
								paramOldValues[paramName].toDouble();
							double startValue = paramOldValues[paramName].toDouble();
							double endValue = paramValues[paramName].toDouble();
							double range = std::abs(endValue - startValue);
							double step = range /
								4;  // Assuming 5 steps (0 to 4), adjust the denominator according to your total steps

							double interpolatedValue;
							if (isIncreasing)
							{
								interpolatedValue = startValue + step * i;  // For increasing values
							}
							else
							{
								interpolatedValue = startValue - step * i;  // For decreasing values
							}

							// Ensure interpolatedValue does not exceed the bounds
							if (isIncreasing)
							{
								interpolatedValue = std::min(interpolatedValue, endValue);
							}
							else
							{
								interpolatedValue = std::max(interpolatedValue, endValue);
							}

							interpolatedParamValues[paramName] = interpolatedValue;
						}
						// Now, update the slicerCopy with the interpolated paramValues
						updateFilterAndSlicer(slicerCopy, interpolatedParamValues);	
						slicerParameters[i] = interpolatedParamValues;
					}
				}
			}
		});

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(splitLayout,7);
	mainLayout->addLayout(imageListLayout,3);

	splitViewDialog->setLayout(mainLayout);
	splitViewDialog->exec();
}



void iAFilterPreviewModuleInterface::filterPreview()
{
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
	/*
	OPTIONAL:
	// extract a region from the image:
	auto inImgLarge = dynamic_cast<iAImageData*>(child->dataSet(child->firstImageDataSetIdx()).get());
	int dim[3];
	inImgLarge->vtkImage()->GetDimensions(dim);
	// Potential for future improvements:
	//     - Adapt the initial size of the region such that the filter runs in under ~100ms or so
	//     - Allow adaptation of the image region (input fields, or even better, region selection in the image)
	const int MinDim = 25;
	if (dim[0] > MinDim || dim[1] > MinDim || dim[2] > MinDim)
	{
		size_t idx[3] = { 0, 0, 0 };
		size_t size[3] = { static_cast<size_t>(std::min(dim[0], MinDim)), static_cast<size_t>(std::min(dim[1], MinDim)), static_cast<size_t>(std::min(dim[2], MinDim)) };
		auto img = extractImage(inImgLarge->itkImage(), idx, size);
		inputImg = std::make_shared<iAImageData>(img);
	}
	else
	{
		inputImg = child->dataSet(child->firstImageDataSetIdx());
	}
	currentFilter->addInput(inputImg);
	*/

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
				minValues.append(defaultVal == 0 ? -1 : defaultVal / 5);
			}

			if (p->max() != std::numeric_limits<double>::max())
			{
				maxValues.append(p->max());
			}
			else
			{
				maxValues.append(defaultVal == 0 ? 1 : defaultVal * 5);
			}
			
		}
	}

	dialog = new QDialog(m_mainWnd);
	dialog->setWindowTitle(filterName + tr(" Filter Preview"));

	// Create a grid layout to hold the preview slicers
	QGridLayout* gridLayout = new QGridLayout;

	// Get the size of the main window and calculate the relative size for the slicers
	auto mainWindowSize = m_mainWnd->size();
	slicerWidth = mainWindowSize.width() / 10;
	slicerHeight = mainWindowSize.height() / 10;

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
	dialog->setLayout(gridLayout);
	dialog->exec();
}
