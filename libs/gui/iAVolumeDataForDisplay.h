#pragma once

#include "iAgui_export.h"

#include "iADataForDisplay.h"

#include <QSharedPointer>

class iAChartWithFunctionsWidget;
class iADockWidgetWrapper;
class iAImageData;
class iAHistogramData;
class iAModalityTransfer;
class iAProgress;

class iAgui_API iAVolumeDataForDisplay : public iADataForDisplay
{
public:
	iAVolumeDataForDisplay(iAImageData* data, iAProgress* p, size_t binCount);
	void show(iAMdiChild* child) override;
	QString information() const override;
	iAModalityTransfer* transfer();
private:
	std::shared_ptr<iAModalityTransfer> m_transfer;
	QSharedPointer<iAHistogramData> m_histogramData;
	iAChartWithFunctionsWidget* m_histogram;
	std::shared_ptr<iADockWidgetWrapper> m_histogramDW;
	QString m_imgStatistics;
};