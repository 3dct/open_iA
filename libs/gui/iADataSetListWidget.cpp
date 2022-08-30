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
	// possible more compact, easier to extend solution to below, separated constants & switch/case cascade in itemClicked:
	//struct CheckableColumn
	//{
	//    QString name;
	//    bool defaultCheckState;
	//    QString iconName;
	//    std::function<void(int, bool)> handler;
	//};
	// std::vector<CheckableColumn> checkableColumns = {
	//	CheckableColumn{"3D", true, "eye", [this](int row, bool checked) {emit set3DRendererVisibility(row, checked); } },
	//	CheckableColumn{"Box", true, "eye", [this](int row, bool checked) {emit setBoundsVisibility(row, checked); } },
	//	CheckableColumn{"2D", true, "eye", [this](int row, bool checked) {emit setPickable(row, checked); } },
	//	CheckableColumn{"Pick", true, "transform-move", [this](int row, bool checked) {emit set2DVisibility(row, checked); } }
	//};
	// but such a list of items would need to be in iADataSetListWidget (unnecessarily exposing it to users)
	enum ViewCheckBoxes
	{
		ViewFirst = 1,  // index of first column with "checkbox" behavior
		View3D = ViewFirst,
		View3DBox = 2,
		View2D = 3,
		ViewLens3D = 4,
		Pickable = 5,
		ViewLast = Pickable  // index of last column with "checkbox" behavior
	};
	QStringList columnNames = QStringList() << "Name"
		// ---------------------------
		<< "3D"
		<< "Box"
		<< "2D"
		<< "Lens3D"
		<< "Pick"
		//			<< "Histo"
		;
	std::vector<bool> DefaultChecked = {
		true, false, true, false, true
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
	m_dataList->setColumnCount(columnNames.size());
	m_dataList->setHorizontalHeaderLabels(columnNames);
	m_dataList->verticalHeader()->hide();
	m_dataList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_dataList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_dataList->resizeColumnsToContents();
	enablePicking(false);

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
			auto dataSetIdx = m_dataList->item(row, 0)->data(Qt::UserRole).toULongLong();
			emit editDataSet(dataSetIdx);
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
			auto row = rows[0].row();
			auto dataSetIdx = m_dataList->item(row, 0)->data(Qt::UserRole).toULongLong();
			m_dataList->removeRow(row);
			emit removeDataSet(dataSetIdx);
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
			if (!item->data(Qt::UserRole + 1).toBool())
			{   // not checkable
				return;
			}
			auto checked = !item->data(Qt::UserRole).toBool();
			setChecked(item, checked);
			auto dataSetIdx = m_dataList->item(row, 0)->data(Qt::UserRole).toULongLong();
			switch (col)
			{
			case View3D:
				emit set3DRendererVisibility(dataSetIdx, checked);
				break;
			case View3DBox:
				emit setBoundsVisibility(dataSetIdx, checked);
				break;
			case Pickable:
				emit setPickable(dataSetIdx, checked);
				break;
			case View2D:
				emit set2DVisibility(dataSetIdx, checked);
				break;
			case ViewLens3D:
				emit set3DMagicLensVisibility(dataSetIdx, checked);
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

void iADataSetListWidget::addDataSet(iADataSet* dataset, size_t dataSetID, bool render3DChecked, bool render3DCheckable, bool render2D)
{
	QSignalBlocker blockList(m_dataList);
	auto nameItem = new QTableWidgetItem(dataset->name());
	nameItem->setToolTip(dataset->info());
	nameItem->setData(Qt::UserRole, static_cast<qulonglong>(dataSetID));
	int row = m_dataList->rowCount();
	m_dataList->insertRow(row);
	m_dataList->setItem(row, 0, nameItem);
	auto checkState(DefaultChecked);
	checkState[View3D - ViewFirst] = render3DChecked;
	for (int i = ViewFirst; i <= ViewLast; ++i)
	{
		auto checked = checkState[i - ViewFirst];
		auto checkable = true;
		if ((i == View3D && !render3DCheckable) || (i == View2D && !render2D))
		{
			checked = false;
			checkable = false;
		}
		auto viewItem = new QTableWidgetItem(iconForCol(i, checked), "");
		viewItem->setData(Qt::UserRole, checked ? 1 : 0);
		viewItem->setData(Qt::UserRole+1, checkable);
		viewItem->setToolTip(checkable ? QString("Enable/Disable rendering/picking") : QString("Disabled because no renderer available"));
		m_dataList->setItem(row, i, viewItem);
	}
	m_dataList->resizeColumnsToContents();
}

void iADataSetListWidget::setName(size_t dataSetIdx, QString newName)
{
	int row = findDataSetIdx(dataSetIdx);
	assert(row != -1);
	m_dataList->item(row, 0)->setText(newName);
}

void iADataSetListWidget::setPickableState(size_t dataSetIdx, bool pickable)
{
	QSignalBlocker blockList(m_dataList);
	int row = findDataSetIdx(dataSetIdx);
	assert(row != -1);
	auto item = m_dataList->item(row, Pickable);
	setChecked(item, pickable);
}

void iADataSetListWidget::enablePicking(bool enable)
{
	m_dataList->setColumnHidden(Pickable, !enable);
}

void iADataSetListWidget::setChecked(QTableWidgetItem * item, int checked)
{
	item->setData(Qt::UserRole, checked);
	item->setIcon(iconForCol(item->column(), checked));
}

int iADataSetListWidget::findDataSetIdx(size_t dataSetIdx)
{
	for (int row=0; row < m_dataList->rowCount(); ++row)
	{
		if (m_dataList->item(row, 0)->data(Qt::UserRole).toULongLong() == dataSetIdx)
		{
			return row;
		}
	}
	return -1;
}
