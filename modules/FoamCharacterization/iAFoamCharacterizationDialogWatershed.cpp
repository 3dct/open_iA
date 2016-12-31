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

#include "iAFoamCharacterizationDialogWatershed.h"

#include <QGridLayout>
#include <QLabel>
#include <QDoubleSpinBox>

#include "iAFoamCharacterizationItemWatershed.h"


iAFoamCharacterizationDialogWatershed::iAFoamCharacterizationDialogWatershed
                                                         (iAFoamCharacterizationItemWatershed* _pItemWatershed, QWidget* _pParent)
	                                                                     : iAFoamCharacterizationDialog(_pItemWatershed, _pParent)
																		 , m_pItemWatershed(_pItemWatershed)
{
	m_pGroupBox2 = new QGroupBox(this);

	QLabel* pLabelLevel (new QLabel("Level:", m_pGroupBox2));
	m_pDoubleSpinBoxLevel = new QDoubleSpinBox(m_pGroupBox2);
	m_pDoubleSpinBoxLevel->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxLevel->setValue(m_pItemWatershed->level());

	QLabel* pLabelThreshold(new QLabel("Threshold [%]:", m_pGroupBox2));
	m_pDoubleSpinBoxTreshold = new QDoubleSpinBox(m_pGroupBox2);
	m_pDoubleSpinBoxTreshold->setAlignment(Qt::AlignRight);
	m_pDoubleSpinBoxTreshold->setRange(0.0, 1.0);
	m_pDoubleSpinBoxTreshold->setSingleStep(0.1);
	m_pDoubleSpinBoxTreshold->setValue(m_pItemWatershed->threshold());

	QGridLayout* pGridLayout2(new QGridLayout(m_pGroupBox2));
	pGridLayout2->addWidget(pLabelLevel, 0, 0);
	pGridLayout2->addWidget(m_pDoubleSpinBoxLevel, 0, 1);
	pGridLayout2->addWidget(pLabelThreshold, 1, 0);
	pGridLayout2->addWidget(m_pDoubleSpinBoxTreshold, 1, 1);

	setLayout();
}

void iAFoamCharacterizationDialogWatershed::slotPushButtonOk()
{
	m_pItemWatershed->setLevel(m_pDoubleSpinBoxLevel->value());
	m_pItemWatershed->setThreshold(m_pDoubleSpinBoxTreshold->value());

	iAFoamCharacterizationDialog::slotPushButtonOk();
}
