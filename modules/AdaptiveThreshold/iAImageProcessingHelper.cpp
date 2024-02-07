// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImageProcessingHelper.h"

#include <iADataSet.h>
#include <iAFilterRegistry.h>
#include <iAFilter.h>
#include <iALog.h>
#include <iAMdiChild.h>
#include <iAMainWindow.h>

#include <QString>

#include <vtkImageData.h>

void performSegmentation(iAMdiChild* child, double greyThresholdLower, double greyThresholdUpper)
{
	if (!child)
	{
		LOG(lvlError, "Child data is null, cannnot perform segmentation");
		//throw std::invalid_argument("Threshold not valid %1 or negative, aborted segmentation ");
		return;
	}

	if ((greyThresholdUpper < 0) || (greyThresholdUpper == std::numeric_limits<double>::infinity()) || (greyThresholdUpper == -std::numeric_limits<double>::infinity()))
	{
		LOG(lvlError, QString("Threshold not valid %1 or negative, please report to developer, if negative values should be valid, aborted segmentation ").arg(0));
		throw std::invalid_argument("Threshold not valid %1 or negative, aborted segmentation ");

	}
	else if ((greyThresholdUpper > 0) && (greyThresholdUpper < 1))
	{
		LOG(lvlError,
			QString("grey threshold: %1 is close to zero, please check parametrisation, or normalized values are used").arg(greyThresholdUpper));
	}
	try
	{
		if (greyThresholdLower > greyThresholdUpper)
		{
			throw std::invalid_argument("Change order of values");
		}
		auto filter = iAFilterRegistry::filter("Binary Thresholding");
		if (!filter)
		{
			throw std::invalid_argument("Could not retrieve Binary Thresholding filter. Make sure Segmentation plugin was loaded correctly!");
		}
		filter->addInput(child->dataSet(child->firstImageDataSetIdx()));  // TODO: should take first image dataset!
		QVariantMap parameters;
		parameters["Lower threshold"] = greyThresholdLower;
		parameters["Upper threshold"] = greyThresholdUpper;
		parameters["Inside value"] = 1;
		parameters["Outside value"] = 0;
		filter->run(parameters);

		auto newChild = iAMainWindow::get()->createMdiChild(true);
		// TODO: check if we need to apply preferences...
		//dynamic_cast<MdiChild*>(targetChild)->applyPreferences(m_defaultPreferences);
		newChild->show();
		newChild->addDataSet(filter->output(0));
		newChild->updateViews();
	}
	catch (std::invalid_argument& iav)
	{
		LOG(lvlError, iav.what());
	}
}
