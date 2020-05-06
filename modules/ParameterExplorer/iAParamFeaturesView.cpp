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
#include "iAParamFeaturesView.h"

#include "iAParamTableView.h"

#include "iAStringHelper.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
	const int ShowColumn = 0;
	//const int NameColumn = 1;
	const int InvertColumn = 2;
}

iAParamFeaturesView::iAParamFeaturesView(QTableWidget* dataTable):
	m_featureTable(new QTableWidget())
{
	m_featureTable->setRowCount(dataTable->columnCount());
	m_featureTable->setColumnCount(3);
	m_featureTable->verticalHeader()->setVisible(false);
	QStringList horizontalHeaders; horizontalHeaders << "Show" << "Name" << "Invert";
	m_featureTable->setHorizontalHeaderLabels(horizontalHeaders);
	for (int row = 0; row < dataTable->columnCount(); ++row)
	{
		QCheckBox* visibleCheckbox = new QCheckBox();
		QWidget * visibleW = new QWidget();
		QHBoxLayout *visibleL = new QHBoxLayout();
		visibleL->setAlignment(Qt::AlignCenter);
		visibleL->addWidget(visibleCheckbox);
		visibleL->setContentsMargins(0, 0, 0, 0);
		visibleL->setSpacing(0);
		visibleW->setLayout(visibleL);
		visibleCheckbox->setProperty("featureID", row);
		visibleCheckbox->setChecked(row != 0);
		connect(visibleCheckbox, &QCheckBox::stateChanged, this, &iAParamFeaturesView::VisibleCheckChanged);
		m_featureTable->setCellWidget(row, ShowColumn, visibleW);
		auto titleItem = new QTableWidgetItem(dataTable->item(0, row)->text());
		titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsEditable);
		m_featureTable->setItem(row, 1, titleItem);
		QCheckBox* invertCheckbox = new QCheckBox();
		QWidget * invertW = new QWidget();
		QHBoxLayout *invertL = new QHBoxLayout();
		invertL->setAlignment(Qt::AlignCenter);
		invertL->setContentsMargins(0, 0, 0, 0);
		invertL->setSpacing(0);
		invertL->addWidget(invertCheckbox);
		invertW->setLayout(invertL);
		invertCheckbox->setProperty("featureID", row);
		m_featureTable->setCellWidget(row, InvertColumn, invertW);
		connect(invertCheckbox, &QCheckBox::stateChanged, this, &iAParamFeaturesView::InvertCheckChanged);
	}
	m_featureTable->resizeColumnsToContents();
	setLayout(new QVBoxLayout);
	layout()->addWidget(m_featureTable);
}

void iAParamFeaturesView::InvertCheckChanged(int state)
{
	QCheckBox* checkbox = qobject_cast<QCheckBox*>(sender());
	int featureID = checkbox->property("featureID").toInt();
	emit InvertFeature(featureID, state);
}

void iAParamFeaturesView::VisibleCheckChanged(int state)
{
	QCheckBox* checkbox = qobject_cast<QCheckBox*>(sender());
	int featureID = checkbox->property("featureID").toInt();
	emit ShowFeature(featureID, state);
}


void iAParamFeaturesView::SaveSettings(QSettings & settings)
{
	QVector<bool> shownList, invertedList;
	for (int row = 0; row < m_featureTable->rowCount(); ++row)
	{
		shownList << m_featureTable->cellWidget(row, ShowColumn)->findChildren<QCheckBox*>()[0]->isChecked();
		invertedList << m_featureTable->cellWidget(row, InvertColumn)->findChildren<QCheckBox*>()[0]->isChecked();
	}
	settings.setValue("Shown", joinAsString(shownList, ","));
	settings.setValue("Inverted", joinAsString(invertedList, ","));
}

void iAParamFeaturesView::LoadSettings(QSettings const & settings)
{
	QStringList shownList = settings.value("Shown").toString().split(",");
	QStringList invertedList = settings.value("Inverted").toString().split(",");
	int rowCount = m_featureTable->rowCount();
	for (int row = 0; row < rowCount; ++row)
	{
		m_featureTable->cellWidget(row, ShowColumn)->findChildren<QCheckBox*>()[0]->setChecked(shownList[row] == "1");
		m_featureTable->cellWidget(row, InvertColumn)->findChildren<QCheckBox*>()[0]->setChecked(invertedList[row] == "1");
	}
}
