/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAFoamCharacterizationDialogBinarization.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include "iAFoamCharacterizationItemBinarization.h"

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
