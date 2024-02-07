// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAttributes.h>

#include <QMap>
#include <QSet>
#include <QSplitter>
#include <QVector>

#include <memory>

class iAChartAttributeMapper;
class iAChartFilter;
class iAClusterAttribChart;
class iAImageTreeNode;

class QGridLayout;
class QLabel;
class QSplitter;

class iAHistogramContainer: public QSplitter
{
	Q_OBJECT
public:
	iAHistogramContainer(
		std::shared_ptr<iAAttributes> chartAttributes,
		iAChartAttributeMapper const & chartAttributeMapper,
		iAImageTreeNode const * root,
		QStringList const & pipelineNames
	);
	bool ChartExists(int chartID) const;
	void CreateCharts();
	void UpdateClusterChartData(QVector<std::shared_ptr<iAImageTreeNode> > const & selection);
	void UpdateFilteredChartData(iAChartFilter const & chartFilter);
	void UpdateClusterFilteredChartData(iAImageTreeNode const * selectedNode, iAChartFilter const & chartFilter);
	void ResetFilters();
	void UpdateAttributeRangeAttitude();
	void ExportAttributeRangeRanking(QString const & fileName);
	int GetSelectedCount();
	int GetSelectedChartID(int selectionIdx);
	void SetMarker(int chartID, double value);
	void RemoveMarker(int chartID);
	void SetSpanValues(int chartID, double min, double max);
	void selectHistograms();
	QString GetSerializedHiddenCharts() const;
	void SetSerializedHiddenCharts(QString const & hiddenCharts);
signals:
	void ChartSelectionUpdated();
	void ChartDblClicked(int chartID);
	void FilterChanged(int chartID, double min, double max);
private slots:
	void ChartSelected(bool selected);
	void ChartDblClicked();
	void FilterChanged(double min, double max);
private:
	void RemoveAllCharts();
	void CreateGridLayout();
	QWidget * m_paramChartWidget, *m_derivedOutputChartWidget;
	QWidget * m_paramChartContainer, *m_derivedOutputChartContainer;
	QGridLayout* m_paramChartLayout;
	QSplitter* m_chartContainer;
	QMap<int, iAClusterAttribChart*> m_charts;
	QVector< QVector<float> > m_attitudes;
	QVector<QLabel*> m_labels;
	QVector<int> m_selected;
	std::shared_ptr<iAAttributes> m_chartAttributes;
	iAChartAttributeMapper const & m_chartAttributeMapper;
	iAImageTreeNode const * m_root;
	QSet<int> m_disabledCharts;
	QStringList m_pipelineNames;
};
