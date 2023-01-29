// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationComboBoxMask.h"

#include "iAFoamCharacterizationTable.h"

iAFoamCharacterizationComboBoxMask::iAFoamCharacterizationComboBoxMask(iAFoamCharacterizationTable* _pTable, QWidget* _pParent) :
																						   QComboBox(_pParent), m_pTable (_pTable)
{
	const int n (m_pTable->rowCount());

	int iItem(0);

	for (int i(0); i < n; ++i)
	{
		iAFoamCharacterizationItem* pItem ((iAFoamCharacterizationItem*) m_pTable->item(i, 0));

		if (pItem->itemType() == iAFoamCharacterizationItem::itBinarization)
		{
			addItem(pItem->name(), iItem++);

			m_vItemMask.push_back(iItem);
		}
	}
}

int iAFoamCharacterizationComboBoxMask::itemMask() const
{
	const int i(currentIndex());

	if (i > -1)
	{
		return m_vItemMask.at(i);
	}
	else
	{
		return -1;
	}
}

void iAFoamCharacterizationComboBoxMask::setItemMask(const int& _iItemMask)
{
	setCurrentIndex(m_vItemMask.indexOf(_iItemMask));
}
