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
#include <QDoubleSpinBox>
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

	QLabel* pLabel2(new QLabel("Type:", m_pGroupBox2));

	m_pComboBox2 = new QComboBox(m_pGroupBox2);
	m_pComboBox2->addItem("Gauss", 0);
	m_pComboBox2->addItem("Median", 1);

	m_pWidgetGauss = new QWidget(m_pGroupBox2);

	QLabel* pLabelGauss(new QLabel("Variance:", m_pWidgetGauss));

	m_pDoubleSpinBoxGauss = new QDoubleSpinBox(m_pWidgetMedian);
	m_pDoubleSpinBoxGauss->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxGauss->setRange(0.0, 100.0);
	m_pDoubleSpinBoxGauss->setValue(m_pItemFilter->variance());

	QGridLayout* pGridLayoutGauss(new QGridLayout(m_pWidgetGauss));
	pGridLayoutGauss->addWidget(pLabelGauss, 0, 0);
	pGridLayoutGauss->addWidget(m_pDoubleSpinBoxGauss, 0, 1);

	m_pWidgetMedian = new QWidget(m_pGroupBox2);
	m_pWidgetMedian->setVisible(false);

	QLabel* pLabelMedian(new QLabel("Box radius:", m_pWidgetMedian));

	m_pSpinBoxMedian = new QSpinBox(m_pWidgetMedian);
	m_pSpinBoxMedian->setAlignment(Qt::AlignRight);
	m_pSpinBoxMedian->setRange(1, 100);
	m_pSpinBoxMedian->setValue(m_pItemFilter->boxRadius());

	QGridLayout* pGridLayoutMedian(new QGridLayout(m_pWidgetMedian));
	pGridLayoutMedian->addWidget(pLabelMedian, 0, 0);
	pGridLayoutMedian->addWidget(m_pSpinBoxMedian, 0, 1);

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(pLabel2, 0, 0);
	pGridLayout2->addWidget(m_pComboBox2, 0, 1);
	pGridLayout2->addWidget(m_pWidgetGauss, 1, 0, 1, 2);
	pGridLayout2->addWidget(m_pWidgetMedian, 1, 0, 1, 2);

	setLayout();

	connect(m_pComboBox2, SIGNAL(currentIndexChanged(const int&)), this, SLOT(slotComboBox2(const int&)));

	m_pComboBox2->setCurrentIndex((int)m_pItemFilter->itemFilterType());
}

void iAFoamCharacterizationDialogFilter::slotComboBox2(const int& _iIndex)
{
	if (_iIndex == 0)
	{
		m_pWidgetGauss->setVisible(true);
		m_pWidgetMedian->setVisible(false);
	}
	else
	{
		m_pWidgetGauss->setVisible(false);
		m_pWidgetMedian->setVisible(true);
	}
}

void iAFoamCharacterizationDialogFilter::slotPushButtonOk()
{
	m_pItemFilter->setItemFilterType((iAFoamCharacterizationItemFilter::EItemFilterType) m_pComboBox2->currentIndex());
	m_pItemFilter->setVariance(m_pDoubleSpinBoxGauss->value());
	m_pItemFilter->setBoxRadius(m_pSpinBoxMedian->value());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
