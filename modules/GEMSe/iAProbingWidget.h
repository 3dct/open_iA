// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QScrollArea>
#include <QVector>

#include <memory>

class iAPlot;
class iAChartWidget;
class iAImageTreeNode;
class iALabelInfo;
class iAParamHistogramData;

class QLabel;

class iAProbingWidget : public QScrollArea
{
	Q_OBJECT
public:
	iAProbingWidget(iALabelInfo const * labelInfo);
	void SetSelectedNode(iAImageTreeNode const * node);
	void SetLabelInfo(iALabelInfo const * labelInfo);
public slots:
	void ProbeUpdate(double x, double y, double z, int mode);
private:
	iALabelInfo const * m_labelInfo;
	QVector<iAChartWidget *> m_charts;
	QVector<std::shared_ptr<iAParamHistogramData>> m_labelDistributionChartData;
	QVector<std::shared_ptr<iAParamHistogramData>> m_probabilitiesChartData;
	std::shared_ptr<iAParamHistogramData> m_entropyChartData;
	QVector<std::shared_ptr<iAPlot>> m_drawers;
	iAImageTreeNode const * m_selectedNode;
	QLabel* m_lbInfo;
	qsizetype m_probChartStart;
};
