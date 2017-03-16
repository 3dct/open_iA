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

#include <QScrollArea>
#include <QSharedPointer>
#include <QVector>

class iAFilterChart;
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
	QVector<iAFilterChart *> m_charts;
	QVector<QSharedPointer<iAParamHistogramData> > m_labelDistributionChartData;
	QVector<QSharedPointer<iAParamHistogramData> > m_probabilitiesChartData;
	QSharedPointer<iAParamHistogramData> m_entropyChartData;
	iAImageTreeNode const * m_selectedNode;
	QLabel* m_lbInfo;
	int m_probChartStart;
};