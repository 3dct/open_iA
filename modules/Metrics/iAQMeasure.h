// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAutoRegistration.h>
#include <iAFilterDefault.h>
#include <iAFilterRunnerGUI.h>
#include <iAFilterRunnerRegistry.h>

class iAChartWithFunctionsWidget;
class iAMdiChild;

class iAQMeasure : public iAFilter, iAAutoRegistration<iAFilter, iAQMeasure, iAFilterRegistry>
{
public:
	iAQMeasure();
	void performWork(QVariantMap const & parameters) override;
	void setupDebugGUI(iAChartWithFunctionsWidget* chart);
	iAChartWithFunctionsWidget* m_chart;
};


class iAQMeasureRunner : public iAFilterRunnerGUI
{
public:
	static std::shared_ptr<iAFilterRunnerGUI> create();
	void filterGUIPreparations(std::shared_ptr<iAFilter> filter,
		iAMdiChild* mdiChild, iAMainWindow* mainWnd, QVariantMap const& params) override;
};


IAFILTER_DEFAULT_CLASS(iACNR);
IAFILTER_DEFAULT_CLASS(iASNR);
