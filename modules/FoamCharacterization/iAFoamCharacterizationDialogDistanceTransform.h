// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationDialog.h"

class QCheckBox;
class iAFoamCharacterizationComboBoxMask;

class iAFoamCharacterizationItemDistanceTransform;

class iAFoamCharacterizationDialogDistanceTransform : public iAFoamCharacterizationDialog
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationDialogDistanceTransform
		                                    (iAFoamCharacterizationItemDistanceTransform* _pItem, QWidget* _pParent = nullptr);

private:
	iAFoamCharacterizationItemDistanceTransform* m_pItemDistanceTransform = nullptr;

	QCheckBox* m_pCheckBoxImageSpacing = nullptr;
	iAFoamCharacterizationComboBoxMask* m_pComboBoxMask = nullptr;

protected slots:
	virtual void slotPushButtonOk() override;
};
