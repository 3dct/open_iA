// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterPreviewModuleInterface.h"

#include "iAChannelData.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iATransferFunction.h"
#include "iAVolumeViewer.h"
#include "iASlicerImpl.h"
#include <iALog.h>


#include "vtkColorTransferFunction.h"

#include <QAction>
#include <QLabel>
#include <QSlider>
#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QInputDialog>



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

void iAFilterPreviewModuleInterface::filterPreview()
{
	QMessageBox::information(m_mainWnd, "Filter with preview", "You will be able to run image filters with a preview here soon!");

	// list available filters:
	auto const& filterFactories = iAFilterRegistry::filterFactories();
	QStringList filterNames;
	for (auto factory : filterFactories)
	{
		auto filter = factory();
		QString filterName = filter->name();
		filterNames.append(filter->name());

		/*QStringList parameterNames;
		for (auto const& p : filter->parameters())
		{
			parameterNames.append(p->name());
		}*/
		/*LOG(lvlDebug, QString("------FILTER NAME------: %1").arg(filterName));
		LOG(lvlDebug, QString("Available Parameters: %1").arg(parameterNames.join(",")));
		LOG(lvlDebug, QString("------END  FILTER------"));*/

		//LOG(lvlDebug,QString("------FILTER NAME------: %1; Available Parameters: %2; ------END  FILTER------").arg(filterName).arg(parameterNames.join(",")));

	}
	//LOG(lvlDebug, QString("Available filters: %1").arg(filterNames.join(",")));

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

	bool ok;
	QString filterName =
		QInputDialog::getItem(m_mainWnd, tr("Select filter"), tr("Filter:"), filterNames, 0, false, &ok);
	if (!ok)
	{
		return;
	}

	// run a specific filter:
	//auto filter = iAFilterRegistry::filter("Discrete Gaussian");
	auto filter = iAFilterRegistry::filter(filterName);
	auto child = m_mainWnd->activeMdiChild();

	//Choose only slider-capable parameters 
	

	QStringList parameterNames;
	QList<double> minValues;  // List to store the minimum values of the parameters
	QList<double> maxValues;  // List to store the maximum values of the parameters

	for (auto const& p : filter->parameters())
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
				maxValues.append(defaultVal == 0 ? 1 : defaultVal*3 / 2);
			}
			
			
		}
	}

	if (parameterNames.isEmpty())
	{
		QMessageBox::critical(m_mainWnd, "Error", "No valid parameters were found for the chosen filter.");
		return;
	}

	
	filter->addInput(child->dataSet(child->firstImageDataSetIdx()));
	



	//QWidget* mainContentWidget = new QWidget(m_mainWnd);
	//mainContentWidget->setWindowTitle(filterName + tr(" Filter Preview"));
	//QGroupBox* mainGroup = new QGroupBox(mainContentWidget);
	//mainGroup->setTitle(tr("Affine Transformations"));

	//

 //   QGroupBox* rotateGroup = new QGroupBox(mainGroup);
	//rotateGroup->setTitle(tr("Rotate"));
	//QSlider* rotateSlider = new QSlider(Qt::Horizontal, rotateGroup);
	//rotateSlider->setRange(0, 3600);
	//rotateSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	//QGroupBox* scaleGroup = new QGroupBox(mainGroup);
	//scaleGroup->setTitle(tr("Scale"));
	//QSlider* scaleSlider = new QSlider(Qt::Horizontal, scaleGroup);
	//scaleSlider->setRange(1, 4000);
	//scaleSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	//QGroupBox* shearGroup = new QGroupBox(mainGroup);
	//shearGroup->setTitle(tr("Shear"));
	//QSlider* shearSlider = new QSlider(Qt::Horizontal, shearGroup);
	//shearSlider->setRange(-990, 990);
	//shearSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	//QVBoxLayout* rotateGroupLayout = new QVBoxLayout(rotateGroup);
	//rotateGroupLayout->addWidget(rotateSlider);

	//QVBoxLayout* scaleGroupLayout = new QVBoxLayout(scaleGroup);
	//scaleGroupLayout->addWidget(scaleSlider);

	//QVBoxLayout* shearGroupLayout = new QVBoxLayout(shearGroup);
	//shearGroupLayout->addWidget(shearSlider);

	//QVBoxLayout* mainGroupLayout = new QVBoxLayout(mainGroup);
	//mainGroupLayout->addWidget(rotateGroup);
	//mainGroupLayout->addWidget(scaleGroup);
	//mainGroupLayout->addWidget(shearGroup);
	//mainGroupLayout->addStretch(1);

	//mainGroup->setLayout(mainGroupLayout);



	//mainContentWidget->show();



	QDialog* dialog = new QDialog(m_mainWnd);
	dialog->setWindowTitle(filterName + tr(" Filter Preview"));

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
	//imageLayout->



	/*QSlider* slider1 = new QSlider(Qt::Horizontal, dialog);
	slider1->setRange(0, 10);
	slider1->setValue(5);

	QSlider* slider2 = new QSlider(Qt::Horizontal, dialog);
	slider2->setRange(1, 99);
	slider2->setValue(50);

	QSlider* slider3 = new QSlider(Qt::Horizontal, dialog);
	slider3->setRange(0, 10);
	slider3->setValue(50);

	QVBoxLayout* sliderLayout = new QVBoxLayout;
	sliderLayout->addWidget(new QLabel(tr("Parameter 1:"), dialog));
	sliderLayout->addWidget(slider1);
	sliderLayout->addWidget(new QLabel(tr("Parameter 2:"), dialog));
	sliderLayout->addWidget(slider2);
	sliderLayout->addWidget(new QLabel(tr("Parameter 3:"), dialog));
	sliderLayout->addWidget(slider3);

	QHBoxLayout* mainLayout = new QHBoxLayout(dialog);
	mainLayout->addLayout(imageLayout);
	mainLayout->addLayout(sliderLayout);*/

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


	//		QVariantMap paramValues

	//connect(slider1, &QSlider::valueChanged, this,
	//	[=](int value)
	//	{
	//		QVariantMap paramValues = {{"Parameter 1", value}};
	//		applyFilter(filter, child, imageLabel, paramValues, slicer);
	//	});

	//connect(slider2, &QSlider::valueChanged, this,
	//	[=](int value)
	//	{
	//		QVariantMap paramValues = {{"Parameter 2", value}};
	//		applyFilter(filter, child, imageLabel, paramValues);
	//	});

	//connect(slider3, &QSlider::valueChanged, this,
	//	[=](int value)
	//	{
	//		QVariantMap paramValues = {{"Parameter 3", value}};
	//		applyFilter(filter, child, imageLabel, paramValues);
	//	});

	//QVariantMap initialParamValues = {{"Parameter 1", 50}, {"Parameter 2", 50}, {"Parameter 3", 50}};
	//applyFilter(filter, child, imageLabel, initialParamValues);

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
	imageLabel->setText("IMAGE PREVIEW DONE!");
	
	

	/*filter->addInput(child->dataSet(child->firstImageDataSetIdx()));
	QVariantMap paramValues;
	paramValues["Variance"] = 5;
	paramValues["Maximum error"] = 0.01;
	paramValues["Convert back to input type"] = false;
	filter->run(paramValues);
	
	for (auto o : filter->outputs())
	{
		child->addDataSet(o);
	}*/
}



