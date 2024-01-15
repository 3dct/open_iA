// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iADataSetViewer.h"

#include <array>

class iAChartWidget;
class iAChartWithFunctionsWidget;
class iADockWidgetWrapper;
class iAImageData;
class iAHistogramData;
struct iAProfileProbe;
class iAProgress;
class iASlicer;
class iATransferFunction;
class iATransferFunctionOwner;
class iAVolumeRenderer;

//! Class for managing all viewing aspects of volume datasets (3D renderer, slicers, histogram, line profile).
class iAguibase_API iAVolumeViewer : public iADataSetViewer
{
public:
	iAVolumeViewer(iADataSet * dataSet);
	~iAVolumeViewer();
	void prepare(iAProgress* p) override;
	void createGUI(iAMdiChild* child, size_t dataSetIdx) override;
	QString information() const override;
	uint slicerChannelID() const override;
	void slicerRegionSelected(double minVal, double maxVal, uint channelID) override;
	void setPickable(bool pickable) override;
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren, QVariantMap const& overrideValues) override;
	//! Access to the chart widget used for displaying the histogram
	iAChartWithFunctionsWidget* histogram();
	//! Access to the histogram data
	std::shared_ptr<iAHistogramData> histogramData(int component) const;  // should return a const raw pointer or reference
	//! Access to the transfer function used in renderer and slicer view
	iATransferFunction* transfer();
	//! Access to the displayed dataset
	iAImageData const* volume() const;
	//! convenience function for showing/hiding dataset in slicer:
	void showInSlicers(bool show);
private:
	//! updates the line profile plot
	void updateProfilePlot();
	void applyAttributes(QVariantMap const& values) override;
	QVariantMap additionalState() const override;

	//! @{ slicer
	uint m_slicerChannelID;
	std::array<iASlicer*, 3> m_slicer;
	//! @}
	//! @{ histogram:
	std::vector<std::shared_ptr<iAHistogramData>> m_histogramData;
	iAChartWithFunctionsWidget* m_histogram;
	std::shared_ptr<iADockWidgetWrapper> m_dwHistogram;
	QAction* m_histogramAction;
	//! @}
	//! @{ line profile
	iAChartWidget* m_profileChart;
	std::shared_ptr<iAProfileProbe> m_profileProbe;
	std::shared_ptr<iADockWidgetWrapper> m_dwProfile;
	//! @}

	std::shared_ptr<iATransferFunctionOwner> m_transfer; //!< transfer function used in 2D slicer and 3D renderer
	QString m_imgStatistics;                      //!< image statistics, for display in data info widget
	std::shared_ptr<iAVolumeRenderer> m_renderer; //!< the 3D renderer
};
