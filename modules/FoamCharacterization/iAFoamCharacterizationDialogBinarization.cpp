// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFoamCharacterizationDialogBinarization.h"

#include "iAFoamCharacterizationItemBinarization.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>

iAFoamCharacterizationDialogBinarization::iAFoamCharacterizationDialogBinarization
                                                   (iAFoamCharacterizationItemBinarization* _pItemBinarization, QWidget* _pParent)
	                                                                  : iAFoamCharacterizationDialog(_pItemBinarization, _pParent)
																      , m_pItemBinarization (_pItemBinarization)
{
	m_pGroupBox2 = new QGroupBox(this);

	QLabel* pLabelBinarizationLower(new QLabel("Lower threshold:", m_pGroupBox2));
	m_pSpinBoxBinarizationLower = new QSpinBox(m_pGroupBox2);
	m_pSpinBoxBinarizationLower->setAlignment(Qt::AlignRight);
	m_pSpinBoxBinarizationLower->setRange(0, 65535);
	m_pSpinBoxBinarizationLower->setWhatsThis( "Set lower threshold. An exception is thrown if the lower threshold is greater "
		                                       " than the upper threshold."
	                                         );
	m_pSpinBoxBinarizationLower->setValue(m_pItemBinarization->lowerThreshold());

	QLabel* pLabelBinarizationUpper(new QLabel("Upper threshold:", m_pGroupBox2));
	m_pSpinBoxBinarizationUpper = new QSpinBox(m_pGroupBox2);
	m_pSpinBoxBinarizationUpper->setAlignment(Qt::AlignRight);
	m_pSpinBoxBinarizationUpper->setRange(0, 65535);
	m_pSpinBoxBinarizationUpper->setWhatsThis( "Set upper threshold. An exception is thrown if the lower threshold is greater "
					                           " than the upper threshold."
                                             );
	m_pSpinBoxBinarizationUpper->setValue(m_pItemBinarization->upperThreshold());

	m_pCheckBoxOtzu = new QCheckBox("Use Otsu thresholding", m_pGroupBox2);
	m_pCheckBoxOtzu->setChecked(m_pItemBinarization->itemFilterType() == iAFoamCharacterizationItemBinarization::iftOtzu);

	QLabel* pLabelOtzuHistogramBins(new QLabel("Otzu's histogram bins:", m_pGroupBox2));
	m_pSpinBoxOtzuHistogramBins = new QSpinBox(m_pGroupBox2);
	m_pSpinBoxOtzuHistogramBins->setAlignment(Qt::AlignRight);
	m_pSpinBoxOtzuHistogramBins->setRange(0, INT_MAX);
	m_pSpinBoxOtzuHistogramBins->setWhatsThis("Set the number of histogram bins.");
	m_pSpinBoxOtzuHistogramBins->setValue(m_pItemBinarization->otzuHistogramBins());

	m_pCheckBoxMask = new QCheckBox("Use as mask", m_pGroupBox2);
	m_pCheckBoxMask->setChecked(m_pItemBinarization->isMask());
	m_pCheckBoxMask->setWhatsThis("The result of the binarization will be used as mask.");

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(pLabelBinarizationLower, 0, 0);
	pGridLayout2->addWidget(m_pSpinBoxBinarizationLower, 0, 1);
	pGridLayout2->addWidget(pLabelBinarizationUpper, 1, 0);
	pGridLayout2->addWidget(m_pSpinBoxBinarizationUpper, 1, 1);
	pGridLayout2->addWidget(m_pCheckBoxOtzu, 2, 0);
	pGridLayout2->addWidget(pLabelOtzuHistogramBins, 3, 0);
	pGridLayout2->addWidget(m_pSpinBoxOtzuHistogramBins, 3, 1);
	pGridLayout2->addWidget(m_pCheckBoxMask, 4, 0);

	setLayout();
}

void iAFoamCharacterizationDialogBinarization::slotPushButtonOk()
{
	m_pItemBinarization->setLowerThreshold(m_pSpinBoxBinarizationLower->value());
	m_pItemBinarization->setUpperThreshold(m_pSpinBoxBinarizationUpper->value());

	m_pItemBinarization->setItemFilterType ( (m_pCheckBoxOtzu->isChecked())
		                                                                 ? iAFoamCharacterizationItemBinarization::iftOtzu
		                                                                 : iAFoamCharacterizationItemBinarization::iftBinarization
	                                       );

	m_pItemBinarization->setOtzuHistogramBins(m_pSpinBoxOtzuHistogramBins->value());

	m_pItemBinarization->setIsMask(m_pCheckBoxMask->isChecked());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
