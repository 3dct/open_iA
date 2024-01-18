// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterPreviewModuleInterface.h"

#include "iAChannelData.h"
#include "iAImageData.h"
#include "iAMainWindow.h"
#include "iAMathUtility.h"
#include "iAMdiChild.h"
#include "iAFilterRegistry.h"
#include "iATransferFunction.h"
#include "iAVolumeViewer.h"
#include "iASlicerImpl.h"
#include <iALog.h>
#include <iAQSplom.h>
#include <iASPLOMData.h>

#include <iAToolsITK.h>
#include <iAToolsVTK.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>

#include <QAction>
#include <QLabel>
#include <QSlider>
#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QPushButton>


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
void iAFilterPreviewModuleInterface::updateFilterAndSlicer(iASlicerImpl* slicer)
{
	

	if (!currentFilter)
	{
		return;  // No filter set
	}

	QVariantMap paramValues;
	for (int i = 0; i < parameterNames.size(); ++i)
	{
		double mappedValue = minValues[i] + (maxValues[i] - minValues[i]) * (double(sliders[i]->value()) / 100);
		//double mappedValue = mapValue(1, 99, minValues[i], maxValues[i], sliders[i]->value());
		paramValues[parameterNames[i]] = mappedValue;
		//LOG(lvlDebug, QString("%1 (min=%2, max=%3): %4").arg(parameterNames[i]).arg(minValues[i]).arg(maxValues[i]).arg(mappedValue));
	}
	
	currentFilter->run(paramValues);  // Run the filter with new parameters

	// check if there is at least one output available:
	if (currentFilter->outputs().size() < 1)
	{
		return;
	}
	auto outDataSet = currentFilter->outputs()[0];
	filterOutputs.push_back(outDataSet);
	auto imgData = dynamic_cast<iAImageData*>(outDataSet.get());
	//storeImage(imgData->vtkImage(), "C:/Users/p41143/Desktop/test.mhd", false);
	// output might not be an image:
	if (!imgData)
	{
		return;
	}
	auto ctf = dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(child->firstImageDataSetIdx()))->transfer()->colorTF();
	auto newChannelData = std::make_shared<iAChannelData>("", imgData->vtkImage(), ctf);
	slicer->updateChannel(0, *newChannelData);  // Update the slicer
	channelData = newChannelData;

	slicer->update();
}

void iAFilterPreviewModuleInterface::openSplitView(iASlicerImpl* slicer)
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
	
	connect(chartsSpmWidget, &iAQSplom::chartClick, this,
		[this](size_t paramX, size_t paramY, double x, double y, Qt::KeyboardModifiers modifiers)
		{ LOG(lvlInfo, QString("params : %1...%2").arg(x).arg(y)); });

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

	splitViewDialog->setLayout(mainLayout);
	splitViewDialog->exec();
}



#include <iAQSplom.h>
#include <iASPLOMData.h>


#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkScatterPlotMatrix.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkTable.h>


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

	// extract a region of maximum size 10x10x10 from image:
	auto inImgLarge = dynamic_cast<iAImageData*>(child->dataSet(child->firstImageDataSetIdx()).get());
	int dim[3];
	inImgLarge->vtkImage()->GetDimensions(dim);
	const int MinDim = 25;
	if (dim[0] > MinDim || dim[1] > MinDim || dim[2] > MinDim)
	{
		size_t idx[3] = { 0, 0, 0 };
		size_t size[3] = { std::min(dim[0], MinDim), std::min(dim[1], MinDim), std::min(dim[2], MinDim) };
		auto img = extractImage(inImgLarge->itkImage(), idx, size);
		inputImg = std::make_shared<iAImageData>(img);
	}
	else
	{
		inputImg = child->dataSet(child->firstImageDataSetIdx());
	}
	currentFilter->addInput(inputImg);

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
	QSize mainWindowSize = m_mainWnd->size();
	int slicerWidth = mainWindowSize.width() / 5;    
	int slicerHeight = mainWindowSize.height() / 5;  


	// Generate the 3x3 matrix of slicers
	for (int row = 0; row < 3; ++row)
	{
		for (int col = 0; col < 3; ++col)
		{
			QWidget* container = new QWidget(dialog);
			QVBoxLayout* layout = new QVBoxLayout(container);

			iASlicerImpl* slicer = new iASlicerImpl(container, iASlicerMode::XY, false);
			iAChannelData* channel = new iAChannelData("", dynamic_cast<iAImageData*>(inputImg.get())->vtkImage(),
				dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(child->firstImageDataSetIdx()))->transfer()->colorTF());

			slicer->addChannel(0, *channel, true);
			slicer->setMinimumSize(QSize(slicerWidth, slicerHeight));  // Set a minimum size for visibility
			slicer->resetCamera();

			// Overlay the TransparentClickableWidget on top of the slicer
			QPushButton* selectButton = new QPushButton("Select", container);
			connect(selectButton, &QPushButton::clicked, [this, slicer]() { this->openSplitView(slicer); });

			layout->addWidget(slicer);
			layout->addWidget(selectButton);  // Add the "Select" button below the slicer

			gridLayout->addWidget(container, row, col);


			
		}
	}

	iAQSplom* chartsSpmWidget = new iAQSplom();
	auto chartsSpmData = std::make_shared<iASPLOMData>();
	std::vector<QString> paramNames;
	paramNames.push_back("Param 1");
	paramNames.push_back("Param 2");
	paramNames.push_back("Param 3");
	chartsSpmData->setParameterNames(paramNames);   // also sets number of columns, so that the "column vectors" below are available; 3 param names, therefore 0..2 are available:
	chartsSpmData->data()[0].push_back(1);
	chartsSpmData->data()[0].push_back(1.1);
	chartsSpmData->data()[0].push_back(1.5);
	chartsSpmData->data()[1].push_back(2);
	chartsSpmData->data()[1].push_back(3);
	chartsSpmData->data()[1].push_back(5);
	chartsSpmData->data()[2].push_back(15);
	chartsSpmData->data()[2].push_back(10);
	chartsSpmData->data()[2].push_back(5);
	chartsSpmData->updateRanges();
	std::vector<char> columnVisibility = { true, true, true };
	chartsSpmWidget->showAllPlots(false);
	chartsSpmWidget->setData(chartsSpmData, columnVisibility);
	gridLayout->addWidget(chartsSpmWidget, 3, 0);
	
	auto vtkSpmWidget = new iAQVTKWidget();
	auto vtkSpmData = vtkSmartPointer<vtkTable>::New();
	vtkSpmData->Initialize();
	auto arr1 = vtkSmartPointer<vtkFloatArray>::New();
	arr1->SetName("Param 1");
	vtkSpmData->AddColumn(arr1);
	auto arr2 = vtkSmartPointer<vtkFloatArray>::New();
	arr2->SetName("Param 2");
	vtkSpmData->AddColumn(arr2);
	auto arr3 = vtkSmartPointer<vtkFloatArray>::New();
	arr3->SetName("Param 3");
	vtkSpmData->AddColumn(arr3);
	vtkSpmData->SetNumberOfRows(3);
	vtkSpmData->SetValue(0, 0, 1);
	vtkSpmData->SetValue(1, 0, 1.1);
	vtkSpmData->SetValue(2, 0, 1.5);
	vtkSpmData->SetValue(0, 1, 2);
	vtkSpmData->SetValue(1, 1, 3);
	vtkSpmData->SetValue(2, 1, 5);
	vtkSpmData->SetValue(0, 2, 15);
	vtkSpmData->SetValue(1, 2, 10);
	vtkSpmData->SetValue(2, 2, 5);
	vtkNew<vtkContextView> view;
	view->SetRenderWindow(vtkSpmWidget->renderWindow());
	view->SetInteractor(vtkSpmWidget->interactor());
	vtkNew<vtkScatterPlotMatrix> matrix;
	view->GetScene()->AddItem(matrix);
	matrix->SetInput(vtkSpmData);
	matrix->SetNumberOfBins(10);
	matrix->SetSelectionMode(vtkContextScene::SELECTION_DEFAULT);
	matrix->SetNumberOfFrames(2);
	//matrix->GetMainChart()->SetActionToButton(vtkChart::SELECT_POLYGON, vtkContextMouseEvent::RIGHT_BUTTON);
	view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
	gridLayout->addWidget(vtkSpmWidget, 3, 1);

	//gridLayout->addWidget(qtSpmWidget, 3, 2);

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



