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
	void setProfilePlot();

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
