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

#include "iAFoamCharacterizationTable.h"

#include <QHeaderView>
#include <QDropEvent>

#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationItemFilter.h"
#include "iAFoamCharacterizationItemWatershed.h"

iAFoamCharacterizationTable::iAFoamCharacterizationTable(QWidget* _pParent) : QTableWidget(_pParent)
{
	setCursor(Qt::PointingHandCursor);

	setDragDropMode(QAbstractItemView::InternalMove);
	setDragDropOverwriteMode(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setFocusPolicy(Qt::NoFocus);
	setSelectionMode(QAbstractItemView::SingleSelection);

	horizontalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionsClickable(false);
	
	setColumnCount(1);

	const QStringList slLabels("Foam characterization protocol");
	setHorizontalHeaderLabels(slLabels);

	setRowCount(5);

	iAFoamCharacterizationItemFilter* pItem1(new iAFoamCharacterizationItemFilter());
	setItem(0, 0, pItem1);

	iAFoamCharacterizationItemFilter* pItem2(new iAFoamCharacterizationItemFilter());
	setItem(1, 0, pItem2);

	iAFoamCharacterizationItemFilter* pItem3(new iAFoamCharacterizationItemFilter());
	setItem(2, 0, pItem3);

	iAFoamCharacterizationItemBinarization* pItem4(new iAFoamCharacterizationItemBinarization());
	setItem(3, 0, pItem4);

	iAFoamCharacterizationItemWatershed* pItem5(new iAFoamCharacterizationItemWatershed());
	setItem(4, 0, pItem5);
}

void iAFoamCharacterizationTable::dragDropSort(QTableWidgetItem** _pItem, const int& _iCount)
{
	if (m_iRowDrag < m_iRowDrop)
	{
		QTableWidgetItem* pItemDrag(_pItem[m_iRowDrag]);

		for (int i(m_iRowDrag); i < m_iRowDrop; ++i)
		{
			_pItem[i] = _pItem[i + 1];
		}

		_pItem[m_iRowDrop] = pItemDrag;
	}
	else
	{
		QTableWidgetItem* pItemDrag(_pItem[m_iRowDrag]);

		for (int i(m_iRowDrag); i > m_iRowDrop; --i)
		{
			_pItem[i] = _pItem[i - 1];
		}

		_pItem[m_iRowDrop] = pItemDrag;
	}
}

void iAFoamCharacterizationTable::dropEvent(QDropEvent* e)
{
	if ((e->source() == this) && (m_iRowDrag > -1))
	{
		m_iRowDrop = indexAt(e->pos()).row();

		if ((m_iRowDrop > -1) && (m_iRowDrag != m_iRowDrop))
		{
			const int n (rowCount());

			QTableWidgetItem** pItem;
			pItem = new QTableWidgetItem*[n];

			for (int i(0); i < n; ++i)
			{
				pItem[i] = new QTableWidgetItem(*item(i, 0));
				takeItem(i, 0);
			}

			dragDropSort(pItem, n);

			for (int i(0); i < n; ++i)
			{
				setItem(i, 0, pItem[i]);
			}
		}
	}
}

void iAFoamCharacterizationTable::mousePressEvent(QMouseEvent* e)
{
	m_iRowDrag = indexAt(e->pos()).row();

	QTableWidget::mousePressEvent(e);
}

void iAFoamCharacterizationTable::resizeEvent(QResizeEvent* e)
{
	setColumnWidth(0, viewport()->width());

	QTableWidget::resizeEvent(e);
}
