// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAdaptiveThresholdModuleInterface.h"

#include "iAAdaptiveThresholdDlg.h"
#include "iAImageProcessingHelper.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAVolumeViewer.h>

#include <iAChartWithFunctionsWidget.h>
#include <iAPlot.h>
#include <iAHistogramData.h>

#include <vtkImageData.h>

#include <QMenu>
#include <QMessageBox>

void iAAdaptiveThresholdModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * determineThresholdAction = new QAction(tr("Adaptive Thresholding"), m_mainWnd);
	connect(determineThresholdAction, &QAction::triggered, this, &iAAdaptiveThresholdModuleInterface::determineThreshold);
	m_mainWnd->makeActionChildDependent(determineThresholdAction);
	addToMenuSorted(m_mainWnd->toolsMenu(), determineThresholdAction);
}

/*
based on precalculated values:
lokal min max, iso 50, and intersection of fair/2 with greyscale curve
stored in iAThresMinMax

algoritm description see paper:
Iryna Tretiak, Robert A. Smith,
A parametric study of segmentation thresholds for X-ray CT porosity characterisation in composite materials,
Composites Part A: Applied Science and Manufacturing,Volume 123,2019,Pages 10-24,
https://doi.org/10.1016/j.compositesa.2019.04.029
*/

void iAAdaptiveThresholdModuleInterface::determineThreshold()
{
	auto child = m_mainWnd->activeMdiChild();
	if (!child)
	{
		LOG(lvlInfo, "No dataset avaiable, please load a dataset before.");
		return;
	}
	auto dsIdx = child->firstImageDataSetIdx();
	if (dsIdx == iAMdiChild::NoDataSet)
	{
		LOG(lvlInfo, "No image data loaded!");
		return;
	}
	auto viewer = dynamic_cast<iAVolumeViewer*>(child->dataSetViewer(dsIdx));
	if (!viewer || !viewer->histogramData())
	{
		LOG(lvlInfo, "No histogram available!");
		return;
	}
	try
	{

		iAAdaptiveThresholdDlg dlg_thres;
		auto data = viewer->histogramData();
		dlg_thres.setHistData(data);

		/*
		*Major Actions in dlg_AdaptiveThreshold.cpp:
		*1: Moving Average: calculateMovingAverage
		*2: Select Peak Ranges: buttonSelectRangesClickedAndComputePeaks
		*3: Determine final threshold- go for decision rule proposed in paper: determineIntersectionAndFinalThreshold
		*4 perform Segmentation see below
		*/
		if (dlg_thres.exec() != QDialog::Accepted)
		{
			return;
		}
		performSegmentation(m_mainWnd->activeMdiChild(), dlg_thres.segmentationStartValue(), dlg_thres.resultingThreshold());
	}
	catch (std::exception& ex)
	{
		LOG(lvlError, ex.what());
	}
}
