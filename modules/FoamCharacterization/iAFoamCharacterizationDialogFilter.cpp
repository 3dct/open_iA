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

#include "iAFoamCharacterizationDialogFilter.h"

#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include "iAFoamCharacterizationItemFilter.h"

iAFoamCharacterizationDialogFilter::iAFoamCharacterizationDialogFilter
                                                               (iAFoamCharacterizationItemFilter* _pItemFilter, QWidget* _pParent)
	                                                                        : iAFoamCharacterizationDialog(_pItemFilter, _pParent)
	                                                                        , m_pItemFilter(_pItemFilter)
{
	m_pGroupBox2 = new QGroupBox(this);

	QLabel* pLabel21(new QLabel("Type:", m_pGroupBox2));

	m_pComboBox2 = new QComboBox(m_pGroupBox2);
	m_pComboBox2->addItem("Gauss", 0);
	m_pComboBox2->addItem("Median", 1);
	m_pComboBox2->setCurrentIndex((int) m_pItemFilter->itemFilterType());

	QLabel* pLabel22(new QLabel("Box radius:", m_pGroupBox2));

	m_pSpinBox2 = new QSpinBox(m_pGroupBox2);
	m_pSpinBox2->setAlignment(Qt::AlignRight);
	m_pSpinBox2->setRange(1, 100);
	m_pSpinBox2->setValue(m_pItemFilter->boxRadius());

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(pLabel21, 0, 0);
	pGridLayout2->addWidget(m_pComboBox2, 0, 1);
	pGridLayout2->addWidget(pLabel22, 1, 0);
	pGridLayout2->addWidget(m_pSpinBox2, 1, 1);

	setLayout();
}

void iAFoamCharacterizationDialogFilter::slotPushButtonOk()
{
	m_pItemFilter->setItemFilterType((iAFoamCharacterizationItemFilter::EItemFilterType) m_pComboBox2->currentIndex());
	m_pItemFilter->setBoxRadius(m_pSpinBox2->value());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
