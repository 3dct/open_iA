/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAFoamCharacterizationDialog.h"

#include "iAFoamCharacterizationItem.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>

iAFoamCharacterizationDialog::iAFoamCharacterizationDialog(iAFoamCharacterizationItem* _pItem, QWidget* _pParent)
																							 : QDialog(_pParent), m_pItem (_pItem)
{
	setWindowTitle(m_pItem->itemTypeStr());

	m_pGroupBox1 = new QGroupBox(this);

	QLabel* pLabel1(new QLabel("Name:", m_pGroupBox1));
	m_pLineEdit1 = new QLineEdit(m_pItem->name(), m_pGroupBox1);
	m_pLineEdit1->setWhatsThis("Set the name of the item.");

	QGridLayout* pGridLayout1(new QGridLayout(m_pGroupBox1));
	pGridLayout1->addWidget(pLabel1, 0, 0);
	pGridLayout1->addWidget(m_pLineEdit1, 0, 1);

	m_pCheckBoxEnabled = new QCheckBox("Enabled", this);
	m_pCheckBoxEnabled->setChecked(m_pItem->itemEnabled());
	m_pCheckBoxEnabled->setWhatsThis("Enable / disable the item.");

	m_pDialogButtonBox = new QDialogButtonBox(this);

	QPushButton* pPushButtonCancel(new QPushButton("Cancel", m_pDialogButtonBox));
	pPushButtonCancel->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogCancelButton));
	connect(pPushButtonCancel, &QPushButton::clicked, this, &iAFoamCharacterizationDialog::slotPushButtonCancel);

	QPushButton* pPushButtonOk(new QPushButton("Ok", m_pDialogButtonBox));
	pPushButtonOk->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOkButton));
	connect(pPushButtonOk, &QPushButton::clicked, this, &iAFoamCharacterizationDialog::slotPushButtonOk);

	m_pDialogButtonBox->addButton(pPushButtonCancel, QDialogButtonBox::RejectRole);
	m_pDialogButtonBox->addButton(pPushButtonOk, QDialogButtonBox::AcceptRole);
}

iAFoamCharacterizationDialog::~iAFoamCharacterizationDialog()
{

}

void iAFoamCharacterizationDialog::setLayout()
{
	QGridLayout* pGridLayout(new QGridLayout(this));
	pGridLayout->addWidget(m_pGroupBox1);
	if (m_pGroupBox2)
	{
		pGridLayout->addWidget(m_pGroupBox2);
	}
	pGridLayout->addWidget(m_pCheckBoxEnabled);
	pGridLayout->addWidget(m_pDialogButtonBox);
}

void iAFoamCharacterizationDialog::slotPushButtonCancel()
{
	reject();
}

void iAFoamCharacterizationDialog::slotPushButtonOk()
{
	m_pItem->setName(m_pLineEdit1->text());
	m_pItem->setItemEnabled(m_pCheckBoxEnabled->isChecked());

	m_pItem->setModified(true);

	accept();
}
