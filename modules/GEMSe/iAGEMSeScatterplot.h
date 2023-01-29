// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAAttributes.h>

#include <iAQVTKWidget.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAChartAttributeMapper;
class iAChartFilter;
class iAImageTreeNode;

class vtkChartXY;
class vtkPlot;
class vtkTable;

class QColor;

class iAGEMSeScatterplot : public iAQVTKWidget
{
public:
	iAGEMSeScatterplot(QWidget* parent);
	void SetDataSource(
		int chart1ID, int chart2ID,
		QString const & chart1Name, QString const & chart2Name,
		bool chart1Log, bool chart2Log,
		iAChartAttributeMapper const & chartAttributeMapper,
		iAChartFilter const & chartFilter,
		iAImageTreeNode const * root,
		iAImageTreeNode const * cluster,
		iAImageTreeNode const * leaf);
	void UpdateFilteredAllPlot(
		iAImageTreeNode const * root,
		iAChartFilter const & chartFilter,
		iAChartAttributeMapper const & chartAttributeMapper);
	void UpdateClusterPlot(
		iAImageTreeNode const * selectedCluster,
		iAChartFilter const & chartFilter,
		iAChartAttributeMapper const & chartAttributeMapper);
	void UpdateLeafPlot(
		iAImageTreeNode const * selectedLeaf,
		iAChartAttributeMapper const & chartAttributeMapper);
private:
	vtkSmartPointer<vtkTable> GetComparisonTable(
		iAImageTreeNode const * node,
		iAChartFilter const & filter,
		iAChartAttributeMapper const & m_chartAttributeMapper);
	void Update(
		vtkSmartPointer<vtkTable> & table,
		vtkSmartPointer<vtkPlot> & plot,
		iAImageTreeNode const * node,
		QColor const & color, iAChartFilter const & filter,
		iAChartAttributeMapper const & chartAttributeMapper);
	vtkSmartPointer<vtkChartXY> m_chart;
	vtkSmartPointer<vtkTable> m_allTable;
	vtkSmartPointer<vtkTable> m_clusterTable;
	vtkSmartPointer<vtkTable> m_allFilteredTable;
	vtkSmartPointer<vtkTable> m_clusterFilteredTable;
	vtkSmartPointer<vtkTable> m_singleTable;
	vtkSmartPointer<vtkPlot> m_allFilteredPlot;
	vtkSmartPointer<vtkPlot> m_clusterPlot;
	vtkSmartPointer<vtkPlot> m_clusterFilteredPlot;
	vtkSmartPointer<vtkPlot> m_singlePlot;
	QWidget * m_parent;

	int m_chart1ID, m_chart2ID;
	QString m_chart1Name, m_chart2Name;
	bool m_chart1Log, m_chart2Log;
};
