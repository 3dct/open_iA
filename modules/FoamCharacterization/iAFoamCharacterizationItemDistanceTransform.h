// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationItem.h"

class QFile;

class iAFoamCharacterizationItemBinarization;

class iAFoamCharacterizationItemDistanceTransform : public iAFoamCharacterizationItem
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationItemDistanceTransform(iAFoamCharacterizationTable* _pTable);
	explicit iAFoamCharacterizationItemDistanceTransform(iAFoamCharacterizationItemDistanceTransform* _pDistanceTransform);

	int itemMask() const;

	void setItemMask(const int& _iItemMask);
	void setUseImageSpacing(const bool& _bImageSpacing);

	bool useImageSpacing() const;

	void dialog() override;
	std::shared_ptr<iADataSet> execute(std::shared_ptr<iADataSet>) override;
	void open(QFile* _pFileOpen) override;
	void save(QFile* _pFileSave) override;

private:
	bool m_bImageSpacing = true;

	int m_iItemMask = -1;
};
