/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAAdaptiveThresholdModuleInterface.h"

#include "iAAdaptiveThresholdDlg.h"
#include "iAImageProcessingHelper.h"

#include <charts/iAChartWithFunctionsWidget.h>
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
	try
	{
		iAAdaptiveThresholdDlg dlg_thres;

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
		*/
		//determineIntersectionAndFinalThreshold
		if (dlg_thres.exec() != QDialog::Accepted)
		{
			return;
		}

		iAImageProcessingHelper imgSegmenter(m_mainWnd->activeMdiChild());
		// resulting threshold: lower and upper limit to obtain for segmentation

		imgSegmenter.performSegmentation(dlg_thres.SegmentationStartValue(),dlg_thres.getResultingThreshold());
	}
	catch (std::exception& ex)
	{
		DEBUG_LOG(ex.what());
	}
}