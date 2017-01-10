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

#include <QMap>
#include <QSet>
#include <QSharedPointer>
#include <QSplitter>
#include <QVector>

class iAAttributes;
class iAChartAttributeMapper;
class iAChartFilter;
class iAChartSpanSlider;
class iAImageTreeNode;

class QGridLayout;
class QLabel;
class QSplitter;

class iAHistogramContainer: public QSplitter
{
	Q_OBJECT
public:
	iAHistogramContainer(
		QSharedPointer<iAAttributes> chartAttributes,
		iAChartAttributeMapper const & chartAttributeMapper,
		iAImageTreeNode const * root,
		QStringList const & pipelineNames
	);
	bool ChartExists(int chartID) const;
	void CreateCharts();
	void UpdateClusterChartData(QVector<QSharedPointer<iAImageTreeNode> > const & selection);
	void UpdateFilteredChartData(iAChartFilter const & chartFilter);
	void UpdateClusterFilteredChartData(iAImageTreeNode const * selectedNode, iAChartFilter const & chartFilter);
	void ResetFilters();
	void UpdateAttributeRangeAttitude();
	void ExportAttributeRangeRanking(QString const & fileName);
	int GetSelectedCount();
	int GetSelectedChartID(int selectionIdx);
	void SetMarker(int chartID, double value);
	void SetSpanValues(int chartID, double min, double max);
	void SelectHistograms();
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
	QMap<int, iAChartSpanSlider*> m_charts;
	QVector< QVector<float> > m_attitudes;
	QVector<QLabel*> m_labels;
	QVector<int> m_selected;
	QSharedPointer<iAAttributes> m_chartAttributes;
	iAChartAttributeMapper const & m_chartAttributeMapper;
	iAImageTreeNode const * m_root;
	QSet<int> m_disabledCharts;
	QStringList m_pipelineNames;
};