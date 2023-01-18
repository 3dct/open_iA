/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
		filter->addInput(child->dataSets()[child->firstImageDataSetIdx()]);  // TODO: should take first image dataset!
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
