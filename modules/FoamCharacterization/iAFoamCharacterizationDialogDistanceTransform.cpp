// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
