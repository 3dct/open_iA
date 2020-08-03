/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <iAChannelData.h>
#include <iAConsole.h>
#include <iAConnector.h>
#include <iAFilterRegistry.h>
#include <iAFilter.h>
#include <iAModality.h>
#include <iAModalityTransfer.h>
#include <iAProgress.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <mdichild.h>
#include <mainwindow.h>

#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkScalarsToColors.h>

iAImageProcessingHelper::iAImageProcessingHelper(MdiChild* child) :m_childData(child)
{
}

void iAImageProcessingHelper::performSegmentation(double greyThresholdMin, double greyThresholdUpper)
{
	if (!m_childData)
	{
		DEBUG_LOG("Child data is null, cannnot perform segmentation");
		//throw std::invalid_argument("Threshold not valid %1 or negative, aborted segmentation ");
		return;
	}

	if ((greyThresholdUpper < 0) || (greyThresholdUpper == std::numeric_limits<double>::infinity()) || (greyThresholdUpper == -std::numeric_limits<double>::infinity()))
	{
		DEBUG_LOG(QString("Threshold not valid %1 or negative, please report to developer, if negative values should be valid, aborted segmentation ").arg(0));
		throw std::invalid_argument("Threshold not valid %1 or negative, aborted segmentation ");

	}
	else if ((greyThresholdUpper > 0) && (greyThresholdUpper < 1))
	{
		DEBUG_LOG(
			QString("grey threshold: %1 is close to zero, please check parametrisation, or normalized values are used").arg(greyThresholdUpper));
	}
	try
	{
		prepareFilter(greyThresholdMin, greyThresholdUpper);
		m_childData->updateViews();
	}
	catch (std::invalid_argument& iav)
	{
		DEBUG_LOG(iav.what());
	}
	//TODO show result in new window
}

void iAImageProcessingHelper::prepareFilter(double greyThresholdLower, double greyThresholdUpper)
{
	if ( greyThresholdLower > greyThresholdUpper)
	{
		throw std::invalid_argument("Change order of values");
	}

	iAConnector con;
	con.setImage(m_childData->imageData());

	QScopedPointer<iAProgress> pObserver(new iAProgress());
	//connect(pObserver.data(), &iAProgress::pprogress, this, &iAImageProcessingHelper::slotObserver);
	auto filter = iAFilterRegistry::filter("Binary Thresholding");
	if (!filter)
	{
		throw std::invalid_argument("Could not retrieve Binary Thresholding filter. Make sure Segmentation plugin was loaded correctly!");
	}
	filter->setLogger(iAConsoleLogger::get());
	filter->setProgress(pObserver.data());
	filter->addInput(&con);
	QMap<QString, QVariant> parameters;
	parameters["Lower threshold"] = greyThresholdLower;
	parameters["Upper threshold"] = greyThresholdUpper;
	parameters["Inside value"] = 1;
	parameters["Outside value"] = 0;
	filter->run(parameters);

	MdiChild* newChild = m_childData->mainWnd()->createMdiChild(true);
	newChild->show();
	newChild->displayResult("Adaptive Thresholding", filter->output()[0]->vtkImage());
	newChild->enableRenderWindows();
}

void iAImageProcessingHelper::imageToReslicer()
{
	auto mod_0 = m_childData->modality(0);
	QSharedPointer<iAModalityTransfer> modTrans = mod_0->transfer();  //m_childData->modality(0)->transfer();
	for (int s = 0; s < 3; ++s)
	{
		m_childData->getSlicer(s)->removeChannel(0);
	}

	uint channelID = m_childData->createChannel();
	assert(channelID == 0); // first modality we create, there shouldn't be another channel yet!

	mod_0->setChannelID(channelID);

	for (int s = 0; s < 3; ++s)
	{
		auto channeldata = iAChannelData(mod_0->name(), mod_0->image(), dynamic_cast<vtkScalarsToColors*> (modTrans->colorTF()), nullptr);
		m_childData->getSlicer(s)->addChannel(0, channeldata, true);
		m_childData->getSlicer(s)->resetCamera();
		m_childData->getSlicer(s)->update();
		//m_childData->getSlicer(s)->updateChannelMappers()
	}
}
