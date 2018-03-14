/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAParamSPLOMView.h"

#include "iAParamColors.h"
#include "iAParamSpatialView.h"
#include "iAParamTableView.h"

#include "charts/iAQSplom.h"
#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iAPerceptuallyUniformLUT.h"
#include "iAQFlowLayout.h"

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QTableWidget>

namespace
{
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
	connect(m_splom, SIGNAL(selectionModified(QVector<unsigned int> *)), this, SLOT(SplomSelection(QVector<unsigned int> *)));
	connect(m_splom, SIGNAL(currentPointModified(int)), this, SLOT(PointHovered(int)));
	m_splom->setData(m_tableView->Table());
	SetLUTColumn("None");
	m_splom->setParameterVisibility("filename", false);
	m_splom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// set up settings:
	m_settings->setLayout(new QVBoxLayout());
	QSpinBox* separationSpinBox = new QSpinBox();
	separationSpinBox->setMinimum(0);
	separationSpinBox->setMaximum(m_tableView->Table()->columnCount()-1);
	separationSpinBox->setValue(0);
	connect(separationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(SeparationChanged(int)));
	QComboBox* separationColors = new QComboBox();
	for (QString themeName : iAColorThemeManager::GetInstance().GetAvailableThemes())
	{
		separationColors->addItem(themeName);
		if (themeName == m_splom->GetBackgroundColorTheme()->GetName())
		{
			separationColors->setCurrentText(themeName);
		}
	}
	connect(separationColors, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(SetColorTheme(const QString &)));
	QComboBox* lutSourceChoice = new QComboBox();
	lutSourceChoice->addItem("None");
	for (int c = 1; c < m_tableView->Table()->columnCount(); ++c) // first col is assumed to be ID/filename
		lutSourceChoice->addItem(m_tableView->Table()->item(0, c)->text());
	connect(lutSourceChoice, SIGNAL(currentTextChanged(const QString &)), this, SLOT(SetLUTColumn(const QString &)));
	QWidget* lutSourceLine = new QWidget();
	lutSourceLine->setLayout(new QHBoxLayout());
	lutSourceLine->layout()->addWidget(new QLabel("Input Parameter #: "));
	lutSourceLine->layout()->addWidget(separationSpinBox);
	lutSourceLine->layout()->addWidget(new QLabel("Separation color scheme: "));
	lutSourceLine->layout()->addWidget(separationColors);
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

void iAParamSPLOMView::SplomSelection(QVector<unsigned int> * selInds)
{
	// set 1 for selection:
	for (int i = 0; i<selInds->size(); ++i)
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
			for (int i = 0; i < 4; ++i)
				rgba[i] = SPLOMDotColor[i];
			m_lut->SetTableValue(i, rgba);
		}
		m_lut->Build();
	}
	else
	{
		iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT(m_lut, lutRange, FullTableValues);
	}
	m_splom->setLookupTable(m_lut, (colName == "None") ? m_tableView->Table()->item(0, 1)->text() : colName );
}


void iAParamSPLOMView::UpdateFeatVisibilty(int)
{
	for (int i = 0; i < m_featCB.size(); ++i)
		m_splom->setParameterVisibility(m_tableView->Table()->item(0, i+1)->text(), m_featCB[i]->isChecked());
}

void iAParamSPLOMView::PointHovered(int id)
{
	m_spatialView->SetImage(id+1);
}

void iAParamSPLOMView::SeparationChanged(int idx)
{
	m_splom->SetSeparation(idx-1);
}

void iAParamSPLOMView::SetColorTheme(const QString &name)
{
	m_splom->SetBackgroundColorTheme(iAColorThemeManager::GetInstance().GetTheme(name));
}
