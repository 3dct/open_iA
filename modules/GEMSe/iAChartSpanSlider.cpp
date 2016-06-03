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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAChartSpanSlider.h"

#include "iAFunctionDrawers.h"
#include "iAMathUtility.h"
#include "iAParamChart.h"
#include "iAParamHistogramData.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

#include <cmath>

iAChartSpanSlider::iAChartSpanSlider(QString const & caption, AttributeID id, QSharedPointer<iAParamHistogramData> data,
		QSharedPointer<iANameMapper> nameMapper):
	m_attribID(id),
	m_oldMin(-1),
	m_oldMax(-1)
{
	double dr0= data->GetDataRange(0);
	double dr1= data->GetDataRange(1);

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setMargin(0);
	mainLayout->setSpacing(5);

	m_checkbox = new QCheckBox(caption);
	QFont f(m_checkbox->font());
	f.setPointSize(FontSize);
	m_checkbox->setFont(f);
	mainLayout->addWidget(m_checkbox);

	m_charts = new iAParamChart(this, vtkSmartPointer<vtkPiecewiseFunction>(), vtkSmartPointer<vtkColorTransferFunction>(), caption, data, nameMapper);
	m_charts->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	mainLayout->addWidget(m_charts);

	setLayout(mainLayout);

	connect(m_checkbox, SIGNAL(toggled(bool)), this, SIGNAL(Toggled(bool)));
	connect(m_charts, SIGNAL(DblClicked()), this,  SIGNAL(ChartDblClicked()));
	connect(m_charts, SIGNAL(SelectionChanged()), this, SLOT(SelectionChanged()));
}


void iAChartSpanSlider::SetAdditionalDrawer(QSharedPointer<iAAbstractDrawableFunction>& drawer, QSharedPointer<iAAbstractDrawableFunction> newDrawer)
{
	if (drawer)
	{
		m_charts->RemoveDataset(drawer);
	}
	drawer = newDrawer;
	m_charts->AddDataset(drawer);
	m_charts->redraw();
}


void iAChartSpanSlider::SetFilteredData(QSharedPointer<iAParamHistogramData> data)
{
	SetAdditionalDrawer(m_filteredDrawer, m_charts->GetDrawer(data, DefaultColors::FilteredChartColor));
}


void iAChartSpanSlider::ClearClusterData()
{
	m_charts->RemoveMarker();
	foreach (QSharedPointer<iAAbstractDrawableFunction> drawer, m_clusterDrawer)
	{
		m_charts->RemoveDataset(drawer);
	}
	m_clusterDrawer.clear();
}

void iAChartSpanSlider::RemoveFilterData()
{
	bool redraw = (m_filteredDrawer || m_filteredClusterDrawer);
	if (m_filteredDrawer)
	{
		m_charts->RemoveDataset(m_filteredDrawer);
		m_filteredDrawer.clear();
	}
	if (m_filteredClusterDrawer)
	{
		m_charts->RemoveDataset(m_filteredClusterDrawer);
		m_filteredClusterDrawer.clear();
	}
	if (redraw)
	{
		m_charts->redraw();
	}
}

QColor iAChartSpanSlider::GetClusterColor(int nr) const
{
	assert(nr < MaxSelectedClusters);
	return DefaultColors::ClusterChartColor[nr];
}

void iAChartSpanSlider::AddClusterData(QSharedPointer<iAParamHistogramData> data)
{
	m_clusterDrawer.push_back(m_charts->GetDrawer(data, GetClusterColor(m_clusterDrawer.size())));
	m_charts->AddDataset(m_clusterDrawer[m_clusterDrawer.size()-1]);
}


void iAChartSpanSlider::SetFilteredClusterData(QSharedPointer<iAParamHistogramData> data)
{
	SetAdditionalDrawer(m_filteredClusterDrawer, m_charts->GetDrawer(data, DefaultColors::FilteredClusterChartColor));
}

void iAChartSpanSlider::SetSpanValues(double minValue, double maxValue)
{
	m_charts->SetMinMaxSlider(minValue, maxValue);
}

void iAChartSpanSlider::SetMarker(double xPos)
{
	m_charts->SetMarker(xPos);
}

void iAChartSpanSlider::SelectionChanged()
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
		double dataRange[2];
		m_charts->GetDataRange(dataRange);
		m_oldMin = minValue;
		m_oldMax = maxValue;
		emit FilterChanged(minValue, maxValue);
	}
}

AttributeID iAChartSpanSlider::GetAttribID() const
{
	return m_attribID;
}

iAValueType iAChartSpanSlider::GetRangeType() const
{
	return m_charts->GetRangeType();
}

void iAChartSpanSlider::ResetSpan()
{
	double dr0= m_charts->mapBinToValue(0);
	double dr1= m_charts->mapBinToValue(m_charts->GetData()->GetNumBin());
	SetSpanValues(dr0, dr1);
}

size_t iAChartSpanSlider::GetNumBin() const
{
	return m_charts->GetData()->GetNumBin();
}


double iAChartSpanSlider::mapValueToBin(double value) const
{
	return m_charts->mapValueToBin(value);
}


void iAChartSpanSlider::SetBinColor(int bin, QColor const & color)
{
	m_charts->SetBinColor(bin, color);
}

void iAChartSpanSlider::UpdateChart()
{
	m_charts->redraw();
}
