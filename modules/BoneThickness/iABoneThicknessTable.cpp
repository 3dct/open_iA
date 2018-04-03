/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iABoneThicknessTable.h"

#include <QHeaderView>
#include <QStandardItemModel>

#include "iABoneThickness.h"
#include "iABoneThicknessChartBar.h"

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
