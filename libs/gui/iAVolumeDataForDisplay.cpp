#include "iAVolumeDataForDisplay.h"

#include "iADataSet.h"
#include "iAProgress.h"

#include "iADockWidgetWrapper.h"

#include "iAModalityTransfer.h"
#include "iAChartWithFunctionsWidget.h"
#include "iAChartFunctionTransfer.h"
#include "iAHistogramData.h"
#include "iAPlotTypes.h"

#include "iAMdiChild.h"

#include <vtkImageData.h>

#include <QApplication>

iAVolumeDataForDisplay::iAVolumeDataForDisplay(iAImageData* data, iAProgress* p, size_t binCount) :
	m_transfer(std::make_shared<iAModalityTransfer>(data->image()->GetScalarRange())),
	iADataForDisplay(data),
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
	return iADataForDisplay::information() + QString("Statistics: %1").arg(m_imgStatistics);
}

void iAVolumeDataForDisplay::show(iAMdiChild* child)
{
	Q_UNUSED(child);
	// show histogram
	QString histoName = "Histogram " + dataSet()->name();
	m_histogram = new iAChartWithFunctionsWidget(child, histoName, "Frequency");
	QObject::connect(m_histogram, &iAChartWithFunctionsWidget::transferFunctionChanged,
		child, &iAMdiChild::changeTransferFunction);

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
}

iAModalityTransfer* iAVolumeDataForDisplay::transfer()
{
	return m_transfer.get();
}