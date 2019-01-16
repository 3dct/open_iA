/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAFoamCharacterizationDialogDistanceTransform.h"

#include "iAFoamCharacterizationComboBoxMask.h"
#include "iAFoamCharacterizationItemDistanceTransform.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

iAFoamCharacterizationDialogDistanceTransform::iAFoamCharacterizationDialogDistanceTransform
									     (iAFoamCharacterizationItemDistanceTransform* _pItemDistanceTransform, QWidget* _pParent)
	                                                             : iAFoamCharacterizationDialog(_pItemDistanceTransform, _pParent)
															     , m_pItemDistanceTransform (_pItemDistanceTransform)
{
	m_pGroupBox2 = new QGroupBox(this);

	m_pCheckBoxImageSpacing = new QCheckBox("Use image spacing", m_pGroupBox2);
	m_pCheckBoxImageSpacing->setChecked(m_pItemDistanceTransform->useImageSpacing());
	m_pCheckBoxImageSpacing->setWhatsThis("Set if image spacing should be used in computing distances.");

	QLabel* pLabelMask (new QLabel("Mask with:", m_pGroupBox2));

	//m_pComboBoxMask = new iAFoamCharacterizationComboBoxMask(m_pItemDistanceTransform->table(), m_pGroupBox2);
	//m_pComboBoxMask->setItemMask(m_pItemDistanceTransform->itemMask());

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(m_pCheckBoxImageSpacing);
	pGridLayout2->addWidget(pLabelMask, 1, 0);
	//pGridLayout2->addWidget(m_pComboBoxMask, 1, 1);

	setLayout();
}

void iAFoamCharacterizationDialogDistanceTransform::slotPushButtonOk()
{
	m_pItemDistanceTransform->setUseImageSpacing(m_pCheckBoxImageSpacing->isChecked());

	//m_pItemDistanceTransform->setItemMask(m_pComboBoxMask->itemMask());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
