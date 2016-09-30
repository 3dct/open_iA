/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include <QVTKWidget2.h>

#include <QSharedPointer>

class iAAttributes;
class iAChartAttributeMapper;
class iAChartFilter;
class iAImageTreeNode;

class vtkChartXY;
class vtkPlot;
class vtkTable;

class QColor;

class iAGEMSeScatterplot : public QVTKWidget2
{
public:
	iAGEMSeScatterplot(QWidget* parent);
	void Update(
		iAImageTreeNode const * root,
		int chart1ID, int chart2ID,
		QSharedPointer<iAAttributes> m_chartAttributes,
		iAChartAttributeMapper const & m_chartAttributeMapper,
		iAChartFilter const & chartFilter,
		iAImageTreeNode const * cluster,
		iAImageTreeNode const * leaf);
	void UpdateFilteredAllPlot(
		iAImageTreeNode const * root,
		iAChartFilter const & chartFilter,
		int chart1ID, int chart2ID,
		QString chart1Name, QString chart2Name,
		iAChartAttributeMapper const & chartAttributeMapper);
	void UpdateClusterPlot(
		iAImageTreeNode const * selectedCluster,
		iAChartFilter const & chartFilter,
		int chart1ID, int chart2ID,
		QString chart1Name, QString chart2Name,
		iAChartAttributeMapper const & chartAttributeMapper);
	void UpdateLeafPlot(iAImageTreeNode const * selectedLeaf,
		int chart1ID, int chart2ID,
		QString chart1Name, QString chart2Name,
		iAChartAttributeMapper const & chartAttributeMapper);
private:
	vtkSmartPointer<vtkTable> GetComparisonTable(
		iAImageTreeNode const * node,
		iAChartFilter const & filter,
		int chart1ID, int chart2ID,
		QString chart1Name, QString chart2Name,
		iAChartAttributeMapper const & m_chartAttributeMapper);
	void Update(
		vtkSmartPointer<vtkTable> & table,
		vtkSmartPointer<vtkPlot> & plot,
		iAImageTreeNode const * node,
		QColor const & color, iAChartFilter const & filter,
		int chart1ID, int chart2ID,
		QString chart1Name, QString chart2Name,
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
};