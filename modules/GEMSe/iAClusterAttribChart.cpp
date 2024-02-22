// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAClusterAttribChart.h"

#include "iAFilterChart.h"
#include "iAParamHistogramData.h"

#include <iAPlotTypes.h>
#include <iAMathUtility.h>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

#include <cmath>

iAClusterAttribChart::iAClusterAttribChart(
	QString const & caption,
	int id,
	std::shared_ptr<iAParamHistogramData> data,
	std::shared_ptr<iANameMapper> nameMapper):
	m_ID(id),
	m_oldMin(-1),
	m_oldMax(-1)
{
	//double dr0= data->xBounds()[0];
	//double dr1= data->xBounds()[1];

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(5);

	m_checkbox = new QCheckBox(caption);
	QFont f(m_checkbox->font());
	f.setPointSize(FontSize);
	m_checkbox->setFont(f);
	m_checkbox->setMinimumWidth(10);
	mainLayout->addWidget(m_checkbox);
	connect(m_checkbox, &QCheckBox::toggled, this, &iAClusterAttribChart::Toggled);

	m_charts = new iAFilterChart(this, caption, data, nameMapper);
	m_charts->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	mainLayout->addWidget(m_charts);

	setLayout(mainLayout);

	connect(m_charts, &iAFilterChart::dblClicked, this, &iAClusterAttribChart::ChartDblClicked);
	connect(m_charts, &iAFilterChart::selectionChanged, this, &iAClusterAttribChart::SelectionChanged);
}

void iAClusterAttribChart::SetAdditionalDrawer(std::shared_ptr<iAPlot>& drawer, std::shared_ptr<iAPlot> newDrawer)
{
	if (drawer)
	{
		m_charts->removePlot(drawer);
	}
	drawer = newDrawer;
	m_charts->addPlot(drawer);
	m_charts->update();
}

void iAClusterAttribChart::SetFilteredData(std::shared_ptr<iAParamHistogramData> newData)
{
	SetAdditionalDrawer(m_filteredDrawer, m_charts->GetDrawer(newData, DefaultColors::FilteredChartColor));
}

void iAClusterAttribChart::ClearClusterData()
{
	m_charts->RemoveMarker();
	for (auto drawer: m_clusterDrawer)
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
		m_filteredDrawer.reset();
	}
	if (m_filteredClusterDrawer)
	{
		m_charts->removePlot(m_filteredClusterDrawer);
		m_filteredClusterDrawer.reset();
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

void iAClusterAttribChart::AddClusterData(std::shared_ptr<iAParamHistogramData> newData)
{
	m_clusterDrawer.push_back(m_charts->GetDrawer(newData, GetClusterColor(m_clusterDrawer.size())));
	m_charts->addPlot(m_clusterDrawer[m_clusterDrawer.size()-1]);
}

void iAClusterAttribChart::SetFilteredClusterData(std::shared_ptr<iAParamHistogramData> filteredData)
{
	SetAdditionalDrawer(m_filteredClusterDrawer, m_charts->GetDrawer(filteredData, DefaultColors::FilteredClusterChartColor));
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
	if (m_charts->GetRangeType() == iAValueType::Discrete || m_charts->GetRangeType() == iAValueType::Categorical)
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
	return m_charts->maxYDataValue();
}

void iAClusterAttribChart::SetMaxYAxisValue(double val)
{
	m_charts->setYBounds(0, val);
}

void iAClusterAttribChart::ResetSpan()
{
	double dr0= m_charts->mapBinToValue(0);
	double dr1= m_charts->mapBinToValue(m_charts->plots()[0]->data()->valueCount());
	SetSpanValues(dr0, dr1);
}

size_t iAClusterAttribChart::GetNumBin() const
{
	return m_charts->plots()[0]->data()->valueCount();
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
