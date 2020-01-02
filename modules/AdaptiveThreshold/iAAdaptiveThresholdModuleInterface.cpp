#include "iAAdaptiveThresholdModuleInterface.h"

#include "dlg_AdaptiveThreshold.h"
#include "ImageProcessingHelper.h"

#include <charts/iADiagramFctWidget.h>
#include <charts/iAPlot.h>
#include <charts/iAPlotData.h>
#include <iAConsole.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <vtkImageData.h>

#include <QMessageBox>

void iAAdaptiveThresholdModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QAction * determineThreshold = new QAction( m_mainWnd );
	determineThreshold->setText( QApplication::translate( "MainWindow", "AdaptiveThresholding", 0 ) );
	AddActionToMenuAlphabeticallySorted(toolsMenu,  determineThreshold, false );
	connect( determineThreshold, SIGNAL( triggered() ), this, SLOT( determineThreshold() ) );
}


void iAAdaptiveThresholdModuleInterface::determineThreshold()
{
	try
	{
		//perform determination of threshold from ui
		//perform segmentation
	
		AdaptiveThreshold dlg_thres;
		
		if (!m_mainWnd->activeMdiChild())
		{
			DEBUG_LOG("data not avaiable, please load a data set before");
			return; 
		}

		auto hist = m_mainWnd->activeMdiChild()->histogram();
		if (!hist || hist->plots().empty())
		{
			DEBUG_LOG("Current data does not have a histogram or histogram not ready"); 
			return;
		}
		
		auto data = hist->plots()[0]->data();
		dlg_thres.setHistData(data);
		
		//load histogram data
		dlg_thres.buttonLoadHistDataClicked(); 

		/*
		*Major Actions in dlg_AdaptiveThreshold.cpp: 
		*1: Moving Average: calculateMovingAverage
		*2: Select Peak Ranges: buttonSelectRangesClickedAndComputePeaks
		*3: Determine final threshold- go for decision rule proposed in paper: determineIntersectionAndFinalThreshold 
		*4 perform Segmentation see below
		*
		*/
		//determineIntersectionAndFinalThreshold 
		if (dlg_thres.exec() != QDialog::Accepted)
		{
			return;
		}

		ImageProcessingHelper imgSegmenter(m_mainWnd->activeMdiChild()); 
		// resulting threshold: lower and upper limit to obtain for segmentation

		imgSegmenter.performSegmentation(dlg_thres.SegmentationStartValue(),dlg_thres.getResultingThreshold()); 
	}
	catch (std::exception& ex)
	{
		DEBUG_LOG(ex.what());
	}
}