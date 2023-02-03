// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationDialog.h"

class QDoubleSpinBox;

class iAFoamCharacterizationItemWatershed;

class iAFoamCharacterizationDialogWatershed : public iAFoamCharacterizationDialog
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationDialogWatershed(iAFoamCharacterizationItemWatershed* _pItem, QWidget* _pParent = nullptr);

private:
	iAFoamCharacterizationItemWatershed* m_pItemWatershed = nullptr;

	QDoubleSpinBox* m_pDoubleSpinBoxLevel = nullptr;
	QDoubleSpinBox* m_pDoubleSpinBoxTreshold = nullptr;

protected slots:
	virtual void slotPushButtonOk() override;
};
