// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABoneThicknessTable.h"

#include "iABoneThickness.h"
#include "iABoneThicknessChartBar.h"

#include <QHeaderView>
#include <QStandardItemModel>

iABoneThicknessTable::iABoneThicknessTable(QWidget* _pParent) : QTableView (_pParent)
{
	setCursor(Qt::PointingHandCursor);

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);

	QStandardItemModel* pItemModel (new QStandardItemModel(0, 5, this));
	pItemModel->setHorizontalHeaderItem(0, new QStandardItem("X"));
	pItemModel->setHorizontalHeaderItem(1, new QStandardItem("Y"));
	pItemModel->setHorizontalHeaderItem(2, new QStandardItem("Z"));
	pItemModel->setHorizontalHeaderItem(3, new QStandardItem("Surface distance"));
	pItemModel->setHorizontalHeaderItem(4, new QStandardItem("Thickness"));

	horizontalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionsClickable(false);

	setModel(pItemModel);
}

QSize iABoneThicknessTable::minimumSizeHint() const
{
	return QSize(2 * logicalDpiX(), 2 * logicalDpiY());
}

void iABoneThicknessTable::mousePressEvent(QMouseEvent* e)
{
	const int iSelected(selected());

	m_pBoneThickness->setSelected(iSelected);
	m_pBoneThicknessChartBar->setSelected(iSelected);

	QTableView::mousePressEvent(e);
}

void iABoneThicknessTable::selectionChanged(const QItemSelection& _Selected, const QItemSelection& _Deselected)
{
	const int iSelected(selected());

	if (m_pBoneThickness) m_pBoneThickness->setSelected(iSelected);
	if (m_pBoneThicknessChartBar) m_pBoneThicknessChartBar->setSelected(iSelected);

	QTableView::selectionChanged(_Selected, _Deselected);
}

int iABoneThicknessTable::selected() const
{
	const QModelIndexList listRows(selectionModel()->selectedRows());

	if (listRows.size())
	{
		return listRows.at(0).row();
	}
	else
	{
		return -1;
	}
}

void iABoneThicknessTable::set(iABoneThickness* _pBoneThickness, iABoneThicknessChartBar* _pBoneThicknessChartBar)
{
	m_pBoneThickness = _pBoneThickness;
	m_pBoneThicknessChartBar = _pBoneThicknessChartBar;
}

void iABoneThicknessTable::setSelected(const vtkIdType& _idSelected)
{
	selectRow(_idSelected);
}
