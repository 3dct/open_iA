/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFilterSelectionDlg.h"

#include "iAFilterRegistry.h"
#include "iAFilter.h"
#include "ui_FilterSelection.h"

#include <QPushButton>

iAFilterSelectionDlg::iAFilterSelectionDlg(QWidget * parent, QString const & preselectedFilter):
	QDialog(parent),
	m_curMatches(0),
	m_ui(new Ui_FilterSelectionDlg())
{
	m_ui->setupUi(this);
	connect(m_ui->leFilterSearch, &QLineEdit::textEdited, this, &iAFilterSelectionDlg::filterChanged);
	connect(m_ui->lwFilterList, &QListWidget::currentItemChanged, this, &iAFilterSelectionDlg::listSelectionChanged);
	connect(m_ui->lwFilterList, &QListWidget::itemDoubleClicked, this, &iAFilterSelectionDlg::accept);
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (auto filterFactory : iAFilterRegistry::filterFactories())
	{
		m_ui->lwFilterList->addItem(filterFactory->create()->name());
	}
	if (!preselectedFilter.isEmpty())
	{
		auto matching = m_ui->lwFilterList->findItems(preselectedFilter, Qt::MatchExactly);
		if (matching.size() > 0)
		{
			m_ui->lwFilterList->setCurrentItem(matching[0]);
		}
	}
	m_ui->splitter->setCollapsible(1, true);
}

void iAFilterSelectionDlg::filterChanged(QString const & filter)
{
	for (int row = 0; row < m_ui->lwFilterList->count(); ++row)
	{
		m_ui->lwFilterList->item(row)->setHidden(true);
	}
	QList<QListWidgetItem*> matches(m_ui->lwFilterList->findItems(filter, Qt::MatchFlag::MatchContains));
	m_curMatches = matches.size();
	for (QListWidgetItem* item : matches)
	{
		item->setHidden(false);
		if (matches.size() == 1)
		{
			m_ui->lwFilterList->setCurrentItem(item);
		}
	}
	updateOKAndDescription();
}

void iAFilterSelectionDlg::updateOKAndDescription()
{
	bool enable = m_curMatches == 1 ||
		(m_ui->lwFilterList->currentItem() != nullptr && !m_ui->lwFilterList->currentItem()->isHidden());
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
	QString description;
	if (enable)
	{
		QString filterName = m_ui->lwFilterList->currentItem()->text();
		auto filter = iAFilterRegistry::filter(filterName);
		assert(filter);
		description = filter->description();
	}
	m_ui->teDescription->setText(description);
}

void iAFilterSelectionDlg::listSelectionChanged(QListWidgetItem * /*current*/, QListWidgetItem * /*previous*/)
{
	updateOKAndDescription();
}

QString iAFilterSelectionDlg::selectedFilterName() const
{
	return m_ui->lwFilterList->currentItem() == nullptr ? QString() : m_ui->lwFilterList->currentItem()->text();
}
