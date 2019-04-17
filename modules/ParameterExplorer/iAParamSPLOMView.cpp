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
#include "iAParamSPLOMView.h"

#include "iAParamTableView.h"
#include "iAParamSpatialView.h"

#include <charts/iAQSplom.h>
#include <iAConsole.h>
#include <iALUT.h>
#include <qthelper/iAQFlowLayout.h>

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QTableWidget>

namespace
{
	const double DotAlpha = 0.5;
	const double DefaultColor[4] = {0.0, 0.0, 1.0, DotAlpha};
	const int EmptyTableValues = 2;
	const int FullTableValues = 256;
	const int DefaultColorColumn = 1;
}

iAParamSPLOMView::iAParamSPLOMView(iAParamTableView* tableView, iAParamSpatialView * spatialView) :
	m_spatialView(spatialView),
	m_tableView(tableView),
	m_splom(new iAQSplom(this)),
	m_selection_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_selection_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_settings(new QWidget)
{
	// set up scatter plot matrix:
	m_selection_ctf->AddRGBPoint(0, 0, 0, 0);
	m_selection_ctf->AddRGBPoint(1, 1.0, 0.0, 0.0);
	m_selection_otf->AddPoint(0, 0);
	m_selection_otf->AddPoint(1, 1);
	connect(m_splom, &iAQSplom::selectionModified, this, &iAParamSPLOMView::SplomSelection);
	connect(m_splom, &iAQSplom::currentPointModified, this, &iAParamSPLOMView::PointHovered);
	m_splom->setData(m_tableView->Table());
	SetLUTColumn("None");
	m_splom->setParameterVisibility("filename", false);
	m_splom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// set up settings:
	m_settings->setLayout(new QVBoxLayout());
	QComboBox* lutSourceChoice = new QComboBox();
	lutSourceChoice->addItem("None");
	for (int c = 1; c < m_tableView->Table()->columnCount(); ++c) // first col is assumed to be ID/filename
		lutSourceChoice->addItem(m_tableView->Table()->item(0, c)->text());
	connect(lutSourceChoice, SIGNAL(currentTextChanged(const QString &)), this, SLOT(SetLUTColumn(const QString &)));
	QWidget* lutSourceLine = new QWidget();
	lutSourceLine->setLayout(new QHBoxLayout());
	lutSourceLine->layout()->addWidget(new QLabel("LUT Source:"));
	lutSourceLine->layout()->addWidget(lutSourceChoice);
	lutSourceLine->setFixedHeight(24);
	lutSourceLine->layout()->setMargin(0);
	lutSourceLine->layout()->setSpacing(2);

	QWidget* featSelectLine = new QWidget();
	featSelectLine->setLayout(new iAQFlowLayout());
	for (int c = 1; c < m_tableView->Table()->columnCount(); ++c) // first col is assumed to be ID/filename
	{
		QCheckBox* cb = new QCheckBox(m_tableView->Table()->item(0, c)->text());
		cb->setChecked(true);
		featSelectLine->layout()->addWidget(cb);
		m_featCB.push_back(cb);
		connect(cb, SIGNAL(stateChanged(int)), this, SLOT(UpdateFeatVisibilty(int)));
	}
	featSelectLine->layout()->setMargin(0);
	featSelectLine->layout()->setSpacing(2);
	featSelectLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_settings->layout()->setMargin(0);
	m_settings->layout()->setSpacing(0);
	m_settings->layout()->addWidget(lutSourceLine);
	m_settings->layout()->addWidget(featSelectLine);

	setLayout(new QVBoxLayout());
	layout()->addWidget(m_splom);
	layout()->addWidget(m_settings);
}

void iAParamSPLOMView::SplomSelection(std::vector<size_t> const & selInds)
{
	// set 1 for selection:
	for (int i = 0; i<selInds.size(); ++i)
	{
		// show in spatial view?
	}
}

void iAParamSPLOMView::SetLUTColumn(QString const & colName)
{
	int col = -1;
	if (colName == "None")
	{
		col = DefaultColorColumn;
	}
	else
	{
		for (int c = 0; c < m_tableView->Table()->columnCount(); ++c)
		{
			if (m_tableView->Table()->item(0, c)->text() == colName)
			{
				col = c;
				break;
			}
		}
		if (col == -1)
		{
			DEBUG_LOG(QString("Unknown column: %1").arg(colName));
			return;
		}
	}
	double lutRange[2] = { m_tableView->ColumnMin(col), m_tableView->ColumnMax(col) };
	if (colName == "None")
	{
		m_lut->SetRange(lutRange);
		m_lut->SetTableRange(lutRange);
		m_lut->SetNumberOfTableValues(2);
		double rgba[4];
		for (vtkIdType i = 0; i < 2; i++)
		{
			for (int j = 0; j < 4; ++j)
				rgba[j] = DefaultColor[j];
			m_lut->SetTableValue(i, rgba);
		}
		m_lut->Build();
	}
	else
	{
		iALUT::BuildLUT(m_lut, lutRange, "Diverging blue-gray-red", FullTableValues);
	}
	m_splom->setLookupTable(m_lut, (colName == "None") ? m_tableView->Table()->item(0, 1)->text() : colName );
}


void iAParamSPLOMView::UpdateFeatVisibilty(int)
{
	for (size_t i = 0; i < m_featCB.size(); ++i)
		m_splom->setParameterVisibility(m_tableView->Table()->item(0, i+1)->text(), m_featCB[i]->isChecked());
}


void iAParamSPLOMView::PointHovered(size_t id)
{
	m_spatialView->SetImage(id+1);
}