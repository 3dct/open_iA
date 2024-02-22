// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationDialog.h"

class QCheckBox;
class QSpinBox;

class iAFoamCharacterizationItemBinarization;

class iAFoamCharacterizationDialogBinarization : public iAFoamCharacterizationDialog
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationDialogBinarization
		                            (iAFoamCharacterizationItemBinarization* _pItemBinarization, QWidget* _pParent = nullptr);

private:
	iAFoamCharacterizationItemBinarization* m_pItemBinarization = nullptr;

	QCheckBox* m_pCheckBoxMask = nullptr;
	QCheckBox* m_pCheckBoxOtzu = nullptr;

	QSpinBox* m_pSpinBoxBinarizationLower = nullptr;
	QSpinBox* m_pSpinBoxBinarizationUpper = nullptr;

	QSpinBox* m_pSpinBoxOtzuHistogramBins = nullptr;

protected slots:
	virtual void slotPushButtonOk() override;
};
