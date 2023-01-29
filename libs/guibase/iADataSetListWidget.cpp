// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	QStringList columnNames = QStringList() << "Name" << "Actions";
	enum ColumIdx {
		NameColumn = 0,
		ActionColumn = 1
	};
}

iADataSetListWidget::iADataSetListWidget()
{
	m_dataList = new QTableWidget;
	m_dataList->setColumnCount(columnNames.size());
	m_dataList->setHorizontalHeaderLabels(columnNames);
	m_dataList->verticalHeader()->hide();
	m_dataList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_dataList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_dataList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_dataList->resizeColumnsToContents();
	connect(m_dataList, &QTableWidget::itemSelectionChanged, this,
		[this]()
		{
			bool itemSelected = !m_dataList->selectedItems().isEmpty();
			size_t dataSetIdx = -1;
			if (m_dataList->selectedItems().size() > 0)
			{
				dataSetIdx = m_dataList->item(m_dataList->selectedItems()[0]->row(), 0)->data(Qt::UserRole).toULongLong();
			}
			emit dataSetSelected(dataSetIdx);
		});
	setLayout(new QHBoxLayout);
	layout()->addWidget(m_dataList);
	layout()->setContentsMargins(1, 0, 0, 0);
	layout()->setSpacing(4);
}

void iADataSetListWidget::addDataSet(iADataSet const* dataset, size_t dataSetIdx, QVector<QAction*> const & actions)
{
	QSignalBlocker blockList(m_dataList);
	auto nameItem = new QTableWidgetItem(dataset->name());
	nameItem->setToolTip(dataset->info());
	nameItem->setData(Qt::UserRole, static_cast<qulonglong>(dataSetIdx));
	int row = m_dataList->rowCount();
	m_dataList->insertRow(row);
	m_dataList->setItem(row, NameColumn, nameItem);
	auto actionWidget = new QWidget();
	actionWidget->setLayout(new QHBoxLayout);
	actionWidget->layout()->setContentsMargins(0, 0, 0, 0);
	actionWidget->layout()->setSpacing(1);
	for (auto& a : actions)
	{
		auto tb = new QToolButton(this);
		tb->setDefaultAction(a);
		actionWidget->layout()->addWidget(tb);
	}
	m_dataList->setCellWidget(row, ActionColumn, actionWidget);
	m_dataList->resizeColumnsToContents();
}

void iADataSetListWidget::setName(size_t dataSetIdx, QString newName)
{
	int row = findDataSetIdx(dataSetIdx);
	assert(row != -1);
	m_dataList->item(row, 0)->setText(newName);
}

void iADataSetListWidget::removeDataSet(size_t dataSetIdx)
{
	int row = findDataSetIdx(dataSetIdx);
	assert(row != -1);
	m_dataList->removeRow(row);
}

int iADataSetListWidget::findDataSetIdx(size_t dataSetIdx)
{
	for (int row=0; row < m_dataList->rowCount(); ++row)
	{
		auto listDSIdx = m_dataList->item(row, 0)->data(Qt::UserRole).toULongLong();
		if (listDSIdx == dataSetIdx)
		{
			return row;
		}
	}
	return -1;
}
