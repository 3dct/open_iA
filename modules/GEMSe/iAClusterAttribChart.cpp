/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAClusterAttribChart.h"

#include "charts/iAPlotTypes.h"
#include "iAMathUtility.h"
#include "iAFilterChart.h"
#include "iAParamHistogramData.h"

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

#include <cmath>

iAClusterAttribChart::iAClusterAttribChart(
	QString const & caption,
	int id,
	QSharedPointer<iAParamHistogramData> data,
	QSharedPointer<iANameMapper> nameMapper):
	m_ID(id),
	m_oldMin(-1),
	m_oldMax(-1)
{
	double dr0= data->XBounds()[0];
	double dr1= data->XBounds()[1];

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setMargin(0);
	mainLayout->setSpacing(5);

	m_checkbox = new QCheckBox(caption);
	QFont f(m_checkbox->font());
	f.setPointSize(FontSize);
	m_checkbox->setFont(f);
	m_checkbox->setMinimumWidth(10);
	mainLayout->addWidget(m_checkbox);
	connect(m_checkbox, SIGNAL(toggled(bool)), this, SIGNAL(Toggled(bool)));

	m_charts = new iAFilterChart(this, "", data, nameMapper);
	m_charts->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	mainLayout->addWidget(m_charts);

	setLayout(mainLayout);

	connect(m_charts, SIGNAL(dblClicked()), this,  SIGNAL(ChartDblClicked()));
	connect(m_charts, SIGNAL(selectionChanged()), this, SLOT(SelectionChanged()));
}

void iAClusterAttribChart::SetAdditionalDrawer(QSharedPointer<iAPlot>& drawer, QSharedPointer<iAPlot> newDrawer)
{
	if (drawer)
	{
		m_charts->removePlot(drawer);
	}
	drawer = newDrawer;
	m_charts->addPlot(drawer);
	m_charts->update();
}

void iAClusterAttribChart::SetFilteredData(QSharedPointer<iAParamHistogramData> data)
{
	SetAdditionalDrawer(m_filteredDrawer, m_charts->GetDrawer(data, DefaultColors::FilteredChartColor));
}

void iAClusterAttribChart::ClearClusterData()
{
	m_charts->RemoveMarker();
	foreach (QSharedPointer<iAPlot> drawer, m_clusterDrawer)
	{
		m_charts->removePlot(drawer);
	}
	m_clusterDrawer.clear();
}

void iAClusterAttribChart::RemoveFilterData()
{
	bool redraw = (m_filteredDrawer || m_filteredClusterDrawer);
	if (m_filteredDrawer)
	{
		m_charts->removePlot(m_filteredDrawer);
		m_filteredDrawer.clear();
	}
	if (m_filteredClusterDrawer)
	{
		m_charts->removePlot(m_filteredClusterDrawer);
		m_filteredClusterDrawer.clear();
	}
	if (redraw)
	{
		m_charts->update();
	}
}

QColor iAClusterAttribChart::GetClusterColor(int nr) const
{
	assert(nr < MaxSelectedClusters);
	return DefaultColors::ClusterChartColor[nr];
}

void iAClusterAttribChart::AddClusterData(QSharedPointer<iAParamHistogramData> data)
{
	m_clusterDrawer.push_back(m_charts->GetDrawer(data, GetClusterColor(m_clusterDrawer.size())));
	m_charts->addPlot(m_clusterDrawer[m_clusterDrawer.size()-1]);
}

void iAClusterAttribChart::SetFilteredClusterData(QSharedPointer<iAParamHistogramData> data)
{
	SetAdditionalDrawer(m_filteredClusterDrawer, m_charts->GetDrawer(data, DefaultColors::FilteredClusterChartColor));
}

void iAClusterAttribChart::SetSpanValues(double minValue, double maxValue)
{
	m_charts->SetMinMaxSlider(minValue, maxValue);
}

void iAClusterAttribChart::SetMarker(double xPos)
{
	m_charts->SetMarker(xPos);
}

void iAClusterAttribChart::RemoveMarker()
{
	m_charts->RemoveMarker();
}

void iAClusterAttribChart::SelectionChanged()
{
	double minValue = m_charts->GetMinSliderPos();
	double maxValue = m_charts->GetMaxSliderPos();
	if (m_charts->GetRangeType() == Discrete || m_charts->GetRangeType() == Categorical)
	{
		minValue = static_cast<int>(minValue);
		maxValue = static_cast<int>(maxValue);
	}
	if (m_oldMin != minValue || m_oldMax != maxValue)
	{
		m_oldMin = minValue;
		m_oldMax = maxValue;
		emit FilterChanged(minValue, maxValue);
	}
}

int iAClusterAttribChart::GetID() const
{
	return m_ID;
}

iAValueType iAClusterAttribChart::GetRangeType() const
{
	return m_charts->GetRangeType();
}

double iAClusterAttribChart::GetMaxYValue() const
{
	return m_charts->getMaxYDataValue();
}

void iAClusterAttribChart::SetMaxYAxisValue(double val)
{
	m_charts->setYBounds(0, val);
}

void iAClusterAttribChart::ResetSpan()
{
	double dr0= m_charts->mapBinToValue(0);
	double dr1= m_charts->mapBinToValue(m_charts->plots()[0]->data()->GetNumBin());
	SetSpanValues(dr0, dr1);
}

size_t iAClusterAttribChart::GetNumBin() const
{
	return m_charts->plots()[0]->data()->GetNumBin();
}

double iAClusterAttribChart::mapValueToBin(double value) const
{
	return m_charts->mapValueToBin(value);
}

void iAClusterAttribChart::SetBinColor(int bin, QColor const & color)
{
	m_charts->SetBinColor(bin, color);
}

void iAClusterAttribChart::UpdateChart()
{
	m_charts->update();
}

void iAClusterAttribChart::ResetMaxYAxisValue()
{
	m_charts->resetYBounds();
}
