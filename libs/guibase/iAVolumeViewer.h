// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include "iADataSetViewer.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAChartWidget;
class iAChartWithFunctionsWidget;
class iADockWidgetWrapper;
class iAImageData;
class iAHistogramData;
class iAPreferences;
struct iAProfileProbe;
class iAProgress;
class iASlicer;
class iATransferFunction;
class iATransferFunctionOwner;
class iAVolRenderer;

class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

class iAguibase_API iAVolumeViewer : public iADataSetViewer
{
public:
	iAVolumeViewer(iADataSet * dataSet);
	~iAVolumeViewer();
	void prepare(iAPreferences const& pref, iAProgress* p) override;
	void createGUI(iAMdiChild* child, size_t dataSetIdx) override;
	QString information() const override;
	void slicerRegionSelected(double minVal, double maxVal, uint channelID) override;
	void setPickable(bool pickable) override;
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren) override;
	QVector<QAction*> additionalActions(iAMdiChild* child) override;
	iAChartWithFunctionsWidget* histogram();
	QSharedPointer<iAHistogramData> histogramData() const;  // should return a const raw pointer or reference
	iATransferFunction* transfer();
	void removeFromSlicer();
private:
	void applyAttributes(QVariantMap const& values) override;
	void updateProfilePlot();

	std::shared_ptr<iATransferFunctionOwner> m_transfer;

	//! @{ slicer
	uint m_slicerChannelID;
	std::array<iASlicer*, 3> m_slicer;
	//! @}
	//! @{ histogram:
	QSharedPointer<iAHistogramData> m_histogramData;
	iAChartWithFunctionsWidget* m_histogram;
	iADockWidgetWrapper* m_dwHistogram;
	QAction* m_histogramAction;
	//! @}

	//! @{ line profile
	iAChartWidget* m_profileChart;
	std::shared_ptr<iAProfileProbe> m_profileProbe;
	iADockWidgetWrapper* m_dwProfile;
	//! @}
	QString m_imgStatistics;
	std::shared_ptr<iAVolRenderer> m_renderer;
};
