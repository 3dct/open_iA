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
#include <QMessageBox>

#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationItemFilter.h"
#include "iAFoamCharacterizationItemWatershed.h"

iAFoamCharacterizationTable::iAFoamCharacterizationTable(QWidget* _pParent) : QTableWidget(_pParent)
{
	setCursor(Qt::PointingHandCursor);

	setDragDropMode(QAbstractItemView::InternalMove);
	setDragDropOverwriteMode(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionMode(QAbstractItemView::SingleSelection);

	horizontalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionsClickable(false);
	
	setColumnCount(1);

	const QStringList slLabels("Foam characterization protocol");
	setHorizontalHeaderLabels(slLabels);
}

void iAFoamCharacterizationTable::addBinarization()
{
	const int n(rowCount());

	setRowCount(n + 1);

	++m_iCountBinarization;

	iAFoamCharacterizationItemBinarization* pItem(new iAFoamCharacterizationItemBinarization());
	pItem->setText(pItem->text() + QString(" %1").arg(m_iCountBinarization));
	setItem(n, 0, pItem);
}

void iAFoamCharacterizationTable::addFilter()
{
	const int n(rowCount());

	setRowCount(n + 1);

	++m_iCountFilter;

	iAFoamCharacterizationItemFilter* pItem(new iAFoamCharacterizationItemFilter());
	pItem->setText(pItem->text() + QString(" %1").arg(m_iCountFilter));
	setItem(n, 0, pItem);
}

void iAFoamCharacterizationTable::addWatershed()
{
	const int n(rowCount());

	setRowCount(n + 1);

	++m_iCountWatershed;

	iAFoamCharacterizationItemWatershed* pItem(new iAFoamCharacterizationItemWatershed());
	pItem->setText(pItem->text() + QString(" %1").arg(m_iCountWatershed));
	setItem(n, 0, pItem);
}

void iAFoamCharacterizationTable::dropEvent(QDropEvent* e)
{
	if ((e->source() == this) && (m_iRowDrag > -1))
	{
		m_iRowDrop = indexAt(e->pos()).row();

		if ((m_iRowDrop > -1) && (m_iRowDrag != m_iRowDrop))
		{
			iAFoamCharacterizationItem* pItemDrag((iAFoamCharacterizationItem*) item(m_iRowDrag, 0));

			if (pItemDrag->itemType() == iAFoamCharacterizationItem::itBinarization)
			{
				pItemDrag =
					new iAFoamCharacterizationItemBinarization((iAFoamCharacterizationItemBinarization*) takeItem(m_iRowDrag, 0));
			}
			else if (pItemDrag->itemType() == iAFoamCharacterizationItem::itFilter)
			{
				pItemDrag = new iAFoamCharacterizationItemFilter((iAFoamCharacterizationItemFilter*) takeItem(m_iRowDrag, 0));
			}
			else
			{
				pItemDrag =
					      new iAFoamCharacterizationItemWatershed((iAFoamCharacterizationItemWatershed*) takeItem(m_iRowDrag, 0));
			}

			if (m_iRowDrag < m_iRowDrop)
			{
				int ii(m_iRowDrag + 1);

				for (int i(m_iRowDrag); i < m_iRowDrop; ++i, ++ii)
				{
					setItem(i, 0, (iAFoamCharacterizationItem*) takeItem(ii, 0));
				}
			}
			else
			{
				int ii(m_iRowDrag - 1);

				for (int i(m_iRowDrag); i > m_iRowDrop; --i, --ii)
				{
					setItem(i, 0, (iAFoamCharacterizationItem*) takeItem(ii, 0));
				}
			}

			setItem(m_iRowDrop, 0, pItemDrag);
		}
	}
}

void iAFoamCharacterizationTable::execute()
{
	const int n(rowCount());

	for (int i (0) ; i < n ; ++i)
	{
		((iAFoamCharacterizationItem*) item(i, 0))->execute();
	}
}

void iAFoamCharacterizationTable::keyPressEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_Delete)
	{
		QModelIndexList mlIndex(selectedIndexes());

		if (mlIndex.size())
		{
			const int iRowSelected(mlIndex.at(0).row());

			if (QMessageBox::information(this, "Information", "Delete " + item(iRowSelected, 0)->text() + "?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
			{
				removeRow(iRowSelected);
			}
		}

		e->accept();
	}
	else
	{
		QTableWidget::keyPressEvent(e);
	}
}

void iAFoamCharacterizationTable::mouseDoubleClickEvent(QMouseEvent*)
{
	QModelIndexList mlIndex(selectedIndexes());

	if (mlIndex.size())
	{
		iAFoamCharacterizationItem* pItem ((iAFoamCharacterizationItem*) item(mlIndex.at(0).row(), 0));

		pItem->dialog();
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
