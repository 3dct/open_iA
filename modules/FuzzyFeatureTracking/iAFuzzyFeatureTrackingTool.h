// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <QObject>

class dlg_trackingGraph;
class dlg_dataView4DCT;
class dlg_trackingGraph;
class dlg_eventExplorer;
class iAVolumeStack;

class iAFuzzyFeatureTrackingTool: public QObject, public iATool
{
public:
	iAFuzzyFeatureTrackingTool(iAMainWindow* mainWnd, iAMdiChild* child);
	~iAFuzzyFeatureTrackingTool();

private slots:
	void updateViews();

protected:
	bool create4DCTDataViewWidget();
	bool create4DCTTrackingGraphWidget();
	bool create4DCTEventExplorerWidget();

	dlg_dataView4DCT * m_dlgDataView4DCT;
	dlg_trackingGraph * m_dlgTrackingGraph;
	dlg_eventExplorer * m_dlgEventExplorer;
	iAVolumeStack * m_volumeStack;
};
