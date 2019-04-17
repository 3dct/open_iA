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

#include <QScrollArea>
#include <QSharedPointer>
#include <QVector>

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
	void ProbeUpdate(int x, int y, int z, int mode);
private:
	iALabelInfo const * m_labelInfo;
	QVector<iAChartWidget *> m_charts;
	QVector<QSharedPointer<iAParamHistogramData> > m_labelDistributionChartData;
	QVector<QSharedPointer<iAParamHistogramData> > m_probabilitiesChartData;
	QSharedPointer<iAParamHistogramData> m_entropyChartData;
	QVector<QSharedPointer<iAPlot> > m_drawers;
	iAImageTreeNode const * m_selectedNode;
	QLabel* m_lbInfo;
	int m_probChartStart;
};