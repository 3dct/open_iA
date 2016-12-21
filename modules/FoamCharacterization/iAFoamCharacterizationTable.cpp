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

#include "iAFoamCharacterizationItemBinarization.h"
#include "iAFoamCharacterizationItemFilter.h"
#include "iAFoamCharacterizationItemWatershed.h"

iAFoamCharacterizationTable::iAFoamCharacterizationTable(QWidget* _pParent) : QTableWidget(_pParent)
{
	setCursor(Qt::PointingHandCursor);

	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionMode(QAbstractItemView::SingleSelection);

	horizontalHeader()->setSectionsClickable(false);
	verticalHeader()->setSectionsClickable(false);

	setColumnCount(1);

	const QStringList slLabels("Foam characterization protocol");
	setHorizontalHeaderLabels(slLabels);

	setRowCount(3);

	iAFoamCharacterizationItemFilter* pItem1(new iAFoamCharacterizationItemFilter());
	setItem(0, 0, pItem1);

	iAFoamCharacterizationItemBinarization* pItem2(new iAFoamCharacterizationItemBinarization());
	setItem(1, 0, pItem2);

	iAFoamCharacterizationItemWatershed* pItem3(new iAFoamCharacterizationItemWatershed());
	setItem(2, 0, pItem3);
}

void iAFoamCharacterizationTable::resizeEvent(QResizeEvent* e)
{
	QTableWidget::resizeEvent(e);

	setColumnWidth(0, viewport()->width());
}
