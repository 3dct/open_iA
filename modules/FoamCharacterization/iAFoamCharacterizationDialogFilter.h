/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
