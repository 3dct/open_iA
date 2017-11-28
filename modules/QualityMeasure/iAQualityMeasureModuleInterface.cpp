/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iAQualityMeasureModuleInterface.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iAImageInfo.h"
#include "iAModality.h"
#include "iATypedCallHelper.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAQualityMeasureModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuSegmEnsembles = getMenuWithTitle(toolsMenu, QString("Quality Measures"), false);

	QAction * actionQ = new QAction(m_mainWnd);
	actionQ->setText(QApplication::translate("MainWindow", "Q Metric", 0));
	AddActionToMenuAlphabeticallySorted(menuSegmEnsembles, actionQ, true);
	connect(actionQ, SIGNAL(triggered()), this, SLOT(CalculateQ()));
}


#include <itkImage.h>
#include <itkImageToHistogramFilter.h>


template <typename T>
void calculateQ_template(iAConnector& con, double histogramBinFactor, double minVal, double maxVal,
	unsigned int numberOfPeaks)
{
	// 1. calculate histogram:
	const int ChannelCount = 1;
	typedef itk::Image<T, DIM>  InputImageType;
	typedef itk::Statistics::ImageToHistogramFilter<InputImageType> ImageHistogramFilterType;
	auto itkImg = con.GetITKImage();
	auto size = itkImg->GetLargestPossibleRegion().GetSize();
	size_t voxelCount = size[0] * size[1] * size[2];
	size_t binCount = static_cast<size_t>(histogramBinFactor * std::sqrt(voxelCount));
	auto histogramFilter = ImageHistogramFilterType::New();
	histogramFilter->ReleaseDataFlagOff();
	histogramFilter->SetInput(dynamic_cast<InputImageType*>(itkImg));
	typename ImageHistogramFilterType::HistogramType::MeasurementVectorType	binMin(ChannelCount);
	binMin.Fill(minVal);
	typename ImageHistogramFilterType::HistogramType::MeasurementVectorType	binMax(ChannelCount);
	binMax.Fill(maxVal);
	typename ImageHistogramFilterType::HistogramType::SizeType binCountMulti(ChannelCount);
	binCountMulti.Fill(binCount);
	histogramFilter->SetHistogramBinMinimum(binMin);
	histogramFilter->SetHistogramBinMaximum(binMax);
	histogramFilter->SetAutoMinimumMaximum(false);
	histogramFilter->SetHistogramSize(binCountMulti);
	histogramFilter->Update();
	auto histogram = histogramFilter->GetOutput();
	size_t b = 0;
	for (auto it = histogram->Begin(); it != histogram->End(); ++it)
	{
		DEBUG_LOG(QString(" Bin %1: %2").arg(b).arg(it.GetFrequency()));
		++b;
	}

	// 2. convolute with gaussian to smooth:


}


void iAQualityMeasureModuleInterface::CalculateQ()
{
	double HistogramBinFactor = 0.25;
	unsigned int NumberOfPeaks = 2;
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAConnector con;
	con.SetImage(m_mdiChild->GetModality(0)->GetImage());
	ITK_TYPED_CALL(calculateQ_template, con.GetITKScalarPixelType(), con, HistogramBinFactor,
		m_mdiChild->GetModality(0)->Info().Min(), m_mdiChild->GetModality(0)->Info().Max(),
		NumberOfPeaks);
}
