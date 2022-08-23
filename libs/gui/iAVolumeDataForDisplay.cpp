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
#include "iAVolumeDataForDisplay.h"

#include "iADataSet.h"
#include "iAProgress.h"

#include "iADockWidgetWrapper.h"

#include "iAModalityTransfer.h"
#include "iAChartWithFunctionsWidget.h"
#include "iAChartFunctionTransfer.h"
#include "iAHistogramData.h"
#include "iAPlotTypes.h"
#include "iAPreferences.h"

#include "iAMdiChild.h"

#include <vtkImageData.h>

#include <QApplication>

iAVolumeDataForDisplay::iAVolumeDataForDisplay(iAImageData* data, iAProgress* p, size_t binCount) :
	iADataForDisplay(data),
	m_transfer(std::make_shared<iAModalityTransfer>(data->image()->GetScalarRange())),
	m_histogram(nullptr),
	m_imgStatistics("Computing...")
{
	p->setStatus(QString("%1: Computing scalar range").arg(data->name()));
	m_transfer->computeRange(data->image());
	iAImageStatistics stats;
	// TODO: handle multiple components; but for that, we need to extract the vtkImageAccumulate part,
	//       as it computes the histograms for all components at once!
	p->emitProgress(50);
	p->setStatus(QString("%1: Computing histogram and statistics.").arg(data->name()));
	m_histogramData = iAHistogramData::create("Frequency", data->image(), binCount, &stats);
	p->emitProgress(100);
	m_imgStatistics = QString("min=%1, max=%2, mean=%3, stddev=%4")
		.arg(stats.minimum)
		.arg(stats.maximum)
		.arg(stats.mean)
		.arg(stats.standardDeviation);
}

QString iAVolumeDataForDisplay::information() const
{
	return iADataForDisplay::information() + "\n" + QString("Statistics: %1").arg(m_imgStatistics);
}

void iAVolumeDataForDisplay::applyPreferences(iAPreferences const& prefs)
{
	auto img = dynamic_cast<iAImageData*>(dataSet())->image();
	size_t newBinCount = iAHistogramData::finalNumBin(img, prefs.HistogramBins);
	if (m_histogramData->valueCount() != newBinCount)
	{
		iAImageStatistics stats;
		m_histogramData = iAHistogramData::create("Frequency", img, prefs.HistogramBins, &stats);
	}
}
void iAVolumeDataForDisplay::updatedPreferences()
{
	if (m_histogramData.get() != m_histogram->plots()[0]->data().get())
	{
		m_histogram->clearPlots();
		auto histogramPlot = QSharedPointer<iABarGraphPlot>::create(
			m_histogramData,
			QApplication::palette().color(QPalette::Shadow));
		m_histogram->addPlot(histogramPlot);
		m_histogram->update();
	}
}

void iAVolumeDataForDisplay::show(iAMdiChild* child)
{
	Q_UNUSED(child);
	// show histogram
	QString histoName = "Histogram " + dataSet()->name();
	m_histogram = new iAChartWithFunctionsWidget(child, histoName, "Frequency");

	auto histogramPlot = QSharedPointer<iABarGraphPlot>::create(
		m_histogramData,
		QApplication::palette().color(QPalette::Shadow));
	m_histogram->addPlot(histogramPlot);
	m_histogram->setTransferFunction(m_transfer.get());
	m_histogram->update();
	// TODO: better unique widget name!
	static int histoNum = -1;
	m_histogramDW = std::make_shared<iADockWidgetWrapper>(m_histogram, histoName, QString("Histogram%1").arg(++histoNum));
	child->splitDockWidget(child->renderDockWidget(), m_histogramDW.get(), Qt::Vertical);

	QObject::connect(m_histogram, &iAChartWithFunctionsWidget::transferFunctionChanged,
		child, &iAMdiChild::changeTransferFunction);
}

iAModalityTransfer* iAVolumeDataForDisplay::transfer()
{
	return m_transfer.get();
}

void iAVolumeDataForDisplay::update()
{
	m_histogram->update();
}
