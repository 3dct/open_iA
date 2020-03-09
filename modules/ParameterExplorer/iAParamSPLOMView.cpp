/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAParamColors.h"
#include "iAParamSpatialView.h"
#include "iAParamTableView.h"

#include <charts/iAQSplom.h>
#include <charts/iASPLOMData.h>
#include <iAColorTheme.h>
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
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QTableWidget>

namespace
{
	//const int EmptyTableValues = 2;
	const int FullTableValues = 256;
	const int DefaultColorColumn = 1;

	//!< Create data from a QTableWidget.
	QSharedPointer<iASPLOMData> splomDataFromQTable(const QTableWidget* tw)
	{
		QSharedPointer<iASPLOMData> result(new iASPLOMData);
		for (int c = 0; c < tw->columnCount(); ++c)
		{
			result->paramNames().push_back(tw->item(0, c)->text());
			std::vector<double> paramData;
			for (int r = 1; r < tw->rowCount() + 1; ++r)
			{
				paramData.push_back(tw->item(r, c)->text().toDouble());
			}
			result->data().push_back(paramData);
		}
		result->updateRanges();
		return result;
	}
}

iAParamSPLOMView::iAParamSPLOMView(iAParamTableView* tableView, iAParamSpatialView* spatialView) :
	m_spatialView(spatialView),
	m_tableView(tableView),
	m_splom(new iAQSplom(this)),
	m_lut(vtkSmartPointer<vtkLookupTable>::New()),
	m_selection_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_selection_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_settings(new QWidget),
	m_separationColors(new QComboBox()),
	m_separationSpinBox(new QSpinBox())
{
	// set up scatter plot matrix:
	m_selection_ctf->AddRGBPoint(0, 0, 0, 0);
	m_selection_ctf->AddRGBPoint(1, 1.0, 0.0, 0.0);
	m_selection_otf->AddPoint(0, 0);
	m_selection_otf->AddPoint(1, 1);
	connect(m_splom, &iAQSplom::selectionModified, this, &iAParamSPLOMView::SplomSelection);
	connect(m_splom, &iAQSplom::currentPointModified, this, &iAParamSPLOMView::PointHovered);
	QSharedPointer<iASPLOMData> spmData = splomDataFromQTable(m_tableView->Table());
	std::vector<char> visibility(spmData->numParams(), true);
	visibility[spmData->paramIndex("filename")] = false;
	m_splom->setData(spmData, visibility);
	SetLUTColumn("None");
	m_splom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// set up settings:
	m_settings->setLayout(new QVBoxLayout());
	m_separationSpinBox->setMinimum(0);
	m_separationSpinBox->setMaximum(m_tableView->Table()->columnCount()-1);
	m_separationSpinBox->setValue(0);
	connect(m_separationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(SeparationChanged(int)));
	for (QString themeName : iAColorThemeManager::instance().availableThemes())
	{
		m_separationColors->addItem(themeName);
		if (themeName == m_splom->getBackgroundColorTheme()->name())
		{
			m_separationColors->setCurrentText(themeName);
		}
	}
	connect(m_separationColors, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(setColorTheme(const QString &)));
	QComboBox* lutSourceChoice = new QComboBox();
	lutSourceChoice->addItem("None");
	for (int c = 1; c < m_tableView->Table()->columnCount(); ++c) // first col is assumed to be ID/filename
		lutSourceChoice->addItem(m_tableView->Table()->item(0, c)->text());
	connect(lutSourceChoice, SIGNAL(currentTextChanged(const QString &)), this, SLOT(SetLUTColumn(const QString &)));
	QWidget* lutSourceLine = new QWidget();
	lutSourceLine->setLayout(new QHBoxLayout());
	lutSourceLine->layout()->addWidget(new QLabel("Input Parameter #: "));
	lutSourceLine->layout()->addWidget(m_separationSpinBox);
	lutSourceLine->layout()->addWidget(new QLabel("Separation color scheme: "));
	lutSourceLine->layout()->addWidget(m_separationColors);
	lutSourceLine->layout()->addWidget(new QLabel("LUT Source:"));
	lutSourceLine->layout()->addWidget(lutSourceChoice);
	lutSourceLine->setFixedHeight(24);
	lutSourceLine->layout()->setMargin(0);
	lutSourceLine->layout()->setSpacing(2);

	m_settings->layout()->setMargin(0);
	m_settings->layout()->setSpacing(0);
	m_settings->layout()->addWidget(lutSourceLine);

	setLayout(new QVBoxLayout());
	layout()->addWidget(m_splom);
	layout()->addWidget(m_settings);
}

void iAParamSPLOMView::SplomSelection(std::vector<size_t> const & /*selInds*/)
{
	// set 1 for selection:
	//for (int i = 0; i<selInds.size(); ++i)
	//{
		// show in spatial view?
	//}
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
				rgba[j] = SPLOMDotColor[j];
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

void iAParamSPLOMView::PointHovered(size_t id)
{
	m_spatialView->setImage(id+1);
}

void iAParamSPLOMView::SeparationChanged(int idx)
{
	m_splom->setSeparation(idx-1);
}

void iAParamSPLOMView::setColorTheme(const QString &name)
{
	m_splom->setBackgroundColorTheme(iAColorThemeManager::instance().theme(name));
}

void iAParamSPLOMView::ToggleSettings(bool visible)
{
	m_settings->setVisible(visible);
}

void iAParamSPLOMView::ShowFeature(int featureID, bool show)
{
	m_splom->setParameterVisibility(featureID, show);
}

void iAParamSPLOMView::InvertFeature(int featureID, bool show)
{
	m_splom->setParameterInverted(featureID, show);
}

void iAParamSPLOMView::SaveSettings(QSettings & settings)
{
	settings.setValue("SPLOMBackgroundColorTheme", m_separationColors->currentText());
	settings.setValue("SPLOMSeparationIndex", m_separationSpinBox->value());
}

void iAParamSPLOMView::LoadSettings(QSettings const & settings)
{
	m_separationColors->setCurrentText(settings.value("SPLOMBackgroundColorTheme", "White").toString());
	m_separationSpinBox->setValue(settings.value("SPLOMSeparationIndex", 0).toInt());
	//m_splom->SetBackgroundColorTheme(iAColorThemeManager::GetInstance().GetTheme(settings.value("SPLOMBackgroundColorTheme", "White").toString()));
	//m_splom->SetSeparation(settings.value("SPLOMSeparationIndex", -1).toInt());
}
