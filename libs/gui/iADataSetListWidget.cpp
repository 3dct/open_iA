/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iADataSetListWidget.h"

#include "iADataSet.h"
#include "iALog.h"
#include "iAMainWindow.h"
#include "iAParameterDlg.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QToolButton>

namespace
{
	enum ViewCheckBoxes
	{
		ViewFirst = 1,  // index of first column with "checkbox" behavior
		View3D = ViewFirst,
		View3DBox = 2,
		Pickable = 3,
		ViewLast = Pickable  // index of last column with "checkbox" behavior
	};

	QIcon iconForCol(int col, bool checked)
	{
		return QIcon(QString(":/images/%1%2.svg")
			.arg(col == Pickable ? "transform-move" : "eye")
			.arg((checked ^ !iAMainWindow::get()->brightMode()) ? "" : "_light"));
	}
}

iADataSetListWidget::iADataSetListWidget()
{
	m_dataList = new QTableWidget;
	QStringList columnNames;
	columnNames << "Name"
				<< "3D"
				<< "Box"
				<< "Pick"
		//			<< "2D" (Slicers)
		//			<< "Histo"
		//          << "Move"  (manual 3D registration)
		;

	m_dataList->setColumnCount(columnNames.size());
	m_dataList->setHorizontalHeaderLabels(columnNames);
	m_dataList->verticalHeader()->hide();
	m_dataList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_dataList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_dataList->resizeColumnsToContents();

	auto buttons = new QWidget();
	buttons->setLayout(new QVBoxLayout);
	buttons->layout()->setContentsMargins(0, 0, 0, 0);
	buttons->layout()->setSpacing(4);
	auto editButton = new QToolButton();
	editButton->setObjectName("tbEdit");
	editButton->setToolTip(tr("Edit dataset and display properties"));
	buttons->layout()->addWidget(editButton);
	auto minusButton = new QToolButton();
	minusButton->setObjectName("tbRemove");
	minusButton->setToolTip(tr("Remove dataset from display, unload from memory"));
	buttons->layout()->addWidget(minusButton);
	auto spacer = new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
	buttons->layout()->addItem(spacer);

	connect(editButton, &QToolButton::clicked, this,
		[this]()
		{
			auto rows = m_dataList->selectionModel()->selectedRows();
			if (rows.size() != 1)
			{
				LOG(lvlWarn, "Please select exactly one row for editing!");
				return;
			}
			int row = rows[0].row();
			emit editDataSet(row);
		});
	connect(minusButton, &QToolButton::clicked, this,
		[this]()
		{
			auto rows = m_dataList->selectionModel()->selectedRows();
			if (rows.size() != 1)
			{
				LOG(lvlWarn, "Please select exactly one row for deleting!");
				return;
			}
			auto idx = rows[0].row();
			m_dataList->removeRow(idx);
			emit removeDataSet(idx);
		});
	connect(m_dataList, &QTableWidget::itemClicked, this,
		[this](QTableWidgetItem* item)
		{
			//		connect(m_dataList, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item)
			//			{
			auto col = m_dataList->column(item);
			if (col < ViewFirst || col > ViewLast)
			{
				return;
			}
			auto row = m_dataList->row(item);
			auto checked = !item->data(Qt::UserRole).toBool();
			item->setData(Qt::UserRole, checked);
			item->setIcon(iconForCol(col, checked));
			switch (col)
			{
			case View3D:
				emit set3DRendererVisibility(row, checked);
				break;
			case View3DBox:
				emit setBoundsVisibility(row, checked);
				break;
			case Pickable:
				emit setPickable(row, checked);
				break;
			default:
				LOG(lvlWarn, QString("Unhandled itemChanged(colum = %1)").arg(col));
				break;
			}
		});
	connect(iAMainWindow::get(), &iAMainWindow::styleChanged, this,
		[this]()
		{
			for (auto row = 0; row < m_dataList->rowCount(); ++row)
			{
				for (int col = ViewFirst; col <= ViewLast; ++col)
				{
					auto item = m_dataList->item(row, col);
					auto checked = item->data(Qt::UserRole).toBool();
					item->setIcon(iconForCol(col, checked));
				}
			}
		});

	setLayout(new QHBoxLayout);
	layout()->addWidget(m_dataList);
	layout()->addWidget(buttons);
	layout()->setContentsMargins(1, 0, 0, 0);
	layout()->setSpacing(4);
}

void iADataSetListWidget::addDataSet(iADataSet* dataset)
{
	QSignalBlocker blockList(m_dataList);
	auto nameItem = new QTableWidgetItem(dataset->name());
	nameItem->setToolTip(dataset->info());
	int row = m_dataList->rowCount();
	m_dataList->insertRow(row);
	m_dataList->setItem(row, 0, nameItem);

	// TODO: avoid duplication, extract method
	auto view3DItem = new QTableWidgetItem(iconForCol(View3D, true), "");
	view3DItem->setData(Qt::UserRole, 1);
	m_dataList->setItem(row, View3D, view3DItem);

	auto view3DBoxItem = new QTableWidgetItem(iconForCol(View3DBox, false), "");
	view3DBoxItem->setData(Qt::UserRole, 0);
	m_dataList->setItem(row, View3DBox, view3DBoxItem);

	auto pickableItem = new QTableWidgetItem(iconForCol(Pickable, true), "");
	pickableItem->setData(Qt::UserRole, 1);
	m_dataList->setItem(row, Pickable, pickableItem);

	m_dataList->resizeColumnsToContents();
}