// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QComboBox>

class iAFoamCharacterizationTable;

class iAFoamCharacterizationComboBoxMask : public QComboBox
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationComboBoxMask(iAFoamCharacterizationTable* _pTable, QWidget* _pParent = nullptr);
	int itemMask() const;
	void setItemMask(const int& _iItemMask);

private:
	iAFoamCharacterizationTable* m_pTable = nullptr;
	QVector<int> m_vItemMask;
};
