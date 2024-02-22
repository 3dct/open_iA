// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationDialog.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

class iAFoamCharacterizationItemFilter;

class iAFoamCharacterizationDialogFilter : public iAFoamCharacterizationDialog
{
	Q_OBJECT

public:
	explicit iAFoamCharacterizationDialogFilter(iAFoamCharacterizationItemFilter* _pItemFilter, QWidget* _pParent = nullptr);

private:
	iAFoamCharacterizationItemFilter* m_pItemFilter = nullptr;

	QComboBox* m_pComboBox2 = nullptr;

	QWidget* m_pWidgetAnisotropic = nullptr;
	QWidget* m_pWidgetGauss = nullptr;
	QWidget* m_pWidgetMedian = nullptr;
	QWidget* m_pWidgetNonLocalMeans= nullptr;

	QCheckBox* m_pCheckBoxImageSpacing = nullptr;

	QDoubleSpinBox* m_pDoubleSpinBoxAnisotropicConductance = nullptr;
	QDoubleSpinBox* m_pDoubleSpinBoxAnisotropicTimeStep = nullptr;
	QDoubleSpinBox* m_pDoubleSpinBoxGaussVariance = nullptr;

	QSpinBox* m_pSpinBoxAnisotropicIteration = nullptr;
	QSpinBox* m_pSpinBoxMedianBoxRadius = nullptr;
	QSpinBox* m_pSpinBoxNonLocalMeansIteration = nullptr;
	QSpinBox* m_pSpinBoxNonLocalMeansRadius = nullptr;

private slots:
	void slotComboBox2(const int& _iIndex);

protected slots:
	virtual void slotPushButtonOk() override;
};
