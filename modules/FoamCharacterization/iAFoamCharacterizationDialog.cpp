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

#include "iAFoamCharacterizationDialog.h"

#include<QApplication>
#include<QGridLayout>
#include<QLabel>
#include<QStyle>
#include<QPushButton>

iAFoamCharacterizationDialog::iAFoamCharacterizationDialog(iAFoamCharacterizationItem* _pItem, QWidget* _pParent)
	                                                           : QDialog(_pParent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
															   , m_pItem (_pItem)
{
	setWindowTitle(m_pItem->itemTypeStr());

	m_pGroupBox1 = new QGroupBox(this);

	QLabel* pLabel1(new QLabel("Name:", m_pGroupBox1));
	m_pLineEdit1 = new QLineEdit(m_pItem->name(), m_pGroupBox1);

	QGridLayout* pGridLayout1(new QGridLayout(m_pGroupBox1));
	pGridLayout1->addWidget(pLabel1, 0, 0);
	pGridLayout1->addWidget(m_pLineEdit1, 0, 1);

	m_pCheckBoxEnabled = new QCheckBox("Enabled", this);
	m_pCheckBoxEnabled->setChecked(m_pItem->itemEnabled());
	
	m_pDialogButtonBox = new QDialogButtonBox(this);

	QPushButton* pPushButtonCancel(new QPushButton("Cancel", m_pDialogButtonBox));
	pPushButtonCancel->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogCancelButton));
	connect(pPushButtonCancel, SIGNAL(clicked()), this, SLOT(slotPushButtonCancel()));

	QPushButton* pPushButtonOk(new QPushButton("Ok", m_pDialogButtonBox));
	pPushButtonOk->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOkButton));
	connect(pPushButtonOk, SIGNAL(clicked()), this, SLOT(slotPushButtonOk()));

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
	m_pItem->reset();

	m_pItem->setItemEnabled(m_pCheckBoxEnabled->isChecked());

	accept();
}
