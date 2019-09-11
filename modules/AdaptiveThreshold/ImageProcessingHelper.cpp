#include "iAConsole.h"
#include "ImageProcessingHelper.h"
#include "iAConnector.h"
#include "QScopedPointer"
#include <Qsharedpointer>
#include "iAProgress.h"
#include "iAFilterRegistry.h"
#include "iAFilter.h"
#include <vtkImageData.h>
//#include "iARenderer.h"
#include "iASlicer.h"
#include <mdichild.h>
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iAChannelData.h"
#include <QString>
#include <vtkScalarsToColors.h>
#include <vtkColorTransferFunction.h>

void ImageProcessingHelper::performSegmentation(double greyThresholdMin, double greyThresholdUpper)
{
	
	DEBUG_LOG(QString("final threshold for segmentation is %1").arg(greyThresholdUpper)); 


	if ((greyThresholdUpper < 0) || (greyThresholdUpper == std::numeric_limits<double>::infinity()) || (greyThresholdUpper == -std::numeric_limits<double>::infinity())) {
		DEBUG_LOG(QString("Threshold not valid %1 or negative, please report if negative values should be valid, aborted segmentation ").arg(0));
		throw std::invalid_argument("Threshold not valid %1 or negative, aborted segmentation ");
		
	}
	else if ((greyThresholdUpper > 0) && (greyThresholdUpper < 1)) {
		DEBUG_LOG(
			QString("grey threshold: %1 is close to zero, please check parametrisation, or normalized values are used").arg(greyThresholdUpper)); 
	}

	if (!m_childData) {
		DEBUG_LOG("Child data is null, cannnot perform segmentation"); 
		//throw std::invalid_argument("Threshold not valid %1 or negative, aborted segmentation ");
		return;
	}


	try {
		prepareFilter(greyThresholdMin, greyThresholdUpper);
		//imageToReslicer();
		m_childData->enableRenderWindows();
		m_childData->updateViews();
	}
	catch (std::invalid_argument& iav)
	{
		DEBUG_LOG(iav.what()); 

	}
	//TODO show result in new window
	
}

void ImageProcessingHelper::prepareFilter(double greyThresholdLower, double greyThresholdUpper)
{
	if ( greyThresholdLower > greyThresholdUpper) {
		throw std::invalid_argument("Change order of values");
	}

	DEBUG_LOG(QString("Using values for segmentation %1 %2 ").arg(greyThresholdLower).arg(greyThresholdUpper));

	iAConnector con; //image reingeben

	con.setImage(m_childData->imageData());



	QScopedPointer<iAProgress> pObserver(new iAProgress());
	//connect(pObserver.data(), SIGNAL(pprogress(const int&)), this, SLOT(slotObserver(const int&)));
	auto filter = iAFilterRegistry::filter("Binary Thresholding");
	filter->setLogger(iAConsoleLogger::get());
	filter->setProgress(pObserver.data());
	filter->addInput(&con);
	QMap<QString, QVariant> parameters;
	parameters["Lower threshold"] = greyThresholdLower;
	parameters["Upper threshold"] = greyThresholdUpper;
	parameters["Inside value"] = 1;
	parameters["Outside value"] = 0;
	filter->run(parameters);

	vtkSmartPointer<vtkImageData> data = vtkSmartPointer<vtkImageData>::New();
	data->DeepCopy(filter->output()[0]->vtkImage());
	//m_childData->displayResult("Adaptive thresholding segmentation", data, nullptr);
	m_childData->setImageData("adaptivethres.mhd", false, data);
}

void ImageProcessingHelper::imageToReslicer()
{
	auto mod_0 = m_childData->modality(0);

	QSharedPointer<iAModalityTransfer> modTrans = mod_0->transfer();  //m_childData->modality(0)->transfer();


	for (int s = 0; s < 3; ++s) {
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
