#include "iAConsole.h"
#include "ImageProcessingHelper.h"
#include "iAConnector.h"
#include "QScopedPointer"
#include <Qsharedpointer>
#include "iAProgress.h"
#include "iAFilterRegistry.h"
#include "iAFilter.h"
#include <vtkImageData.h>

void ImageProcessingHelper::performSegmentation(double greyThreshold)
{
	if (!m_childData) {
		DEBUG_LOG("Child data is null, cannnot perform segmentation"); 
		return;
	}

	iAConnector con; //image reingeben
	con.setImage(m_childData->imageData());
	QScopedPointer<iAProgress> pObserver(new iAProgress());
	//connect(pObserver.data(), SIGNAL(pprogress(const int&)), this, SLOT(slotObserver(const int&)));
	auto filter = iAFilterRegistry::filter("Binary Thresholding");
	filter->setLogger(iAConsoleLogger::get());
	filter->setProgress(pObserver.data());
	filter->addInput(&con);
	QMap<QString, QVariant> parameters;
	parameters["Lower threshold"] = 0;
	parameters["Upper threshold"] = greyThreshold;
	parameters["Inside value"] = 1;
	parameters["Outside value"] = 0;
	filter->run(parameters);

	vtkSmartPointer<vtkImageData> data = vtkSmartPointer<vtkImageData>::New();
	data->DeepCopy(filter->output()[0]->vtkImage());
	
	//m_childData->displayResult("Adaptive thresholding segmentation", m_childData->imagePointer(), nullptr);
	//m_childData->displayResult("Adaptive thresholding segmentation", filter->output()[0]->vtkImage(), nullptr);
	m_childData->setImageData("Adaptive thresholding segmentation.mhd",false, data);

	//m_childData->create 
	//createResultchild
	m_childData->updateViews();

}
