/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <iAVtkWidget.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAAttributes;
class iAChartAttributeMapper;
class iAChartFilter;
class iAImageTreeNode;

class vtkChartXY;
class vtkPlot;
class vtkTable;

class QColor;

class iAGEMSeScatterplot : public iAVtkWidget
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
