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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iABoneThicknessTable.h"

#include <QHeaderView>
#include <QStandardItemModel>

#include "iABoneThickness.h"

iABoneThicknessTable::iABoneThicknessTable(iABoneThickness* _pBoneThickness, QWidget* _pParent) : QTableView (_pParent), m_pBoneThickness (_pBoneThickness)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setFocusPolicy(Qt::NoFocus);
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

void iABoneThicknessTable::mousePressEvent(QMouseEvent* e)
{
	setSphereSelected();
	QTableView::mousePressEvent(e);
}

int iABoneThicknessTable::selectedRow() const
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

void iABoneThicknessTable::selectionChanged(const QItemSelection& _itemSelected, const QItemSelection& _itemDeselected)
{
	setSphereSelected();
	QTableView::QTableView::selectionChanged(_itemSelected, _itemDeselected);
}

void iABoneThicknessTable::setSphereSelected()
{
	m_pBoneThickness->setSphereSelected(selectedRow());
}
