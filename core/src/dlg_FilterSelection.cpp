/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "dlg_FilterSelection.h"

#include "iAFilterRegistry.h"
#include "iAFilter.h"

#include <QPushButton>

dlg_FilterSelection::dlg_FilterSelection(QWidget * parent, QString const & preselectedFilter):
	dlg_FilterSelectionConnector(parent),
	m_curMatches(0)
{
	connect(leFilterSearch, &QLineEdit::textEdited, this, &dlg_FilterSelection::filterChanged);
	connect(lwFilterList, &QListWidget::currentItemChanged, this, &dlg_FilterSelection::listSelectionChanged);
	connect(lwFilterList, &QListWidget::itemDoubleClicked, this, &dlg_FilterSelection::accept);
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (auto filterFactory : iAFilterRegistry::filterFactories())
	{
		lwFilterList->addItem(filterFactory->create()->name());
	}
	if (!preselectedFilter.isEmpty())
	{
		auto matching = lwFilterList->findItems(preselectedFilter, Qt::MatchExactly);
		if (matching.size() > 0)
		{
			lwFilterList->setCurrentItem(matching[0]);
		}
	}
}

void dlg_FilterSelection::filterChanged(QString const & filter)
{
	for (int row = 0; row < lwFilterList->count(); ++row)
	{
		lwFilterList->item(row)->setHidden(true);
	}
	QList<QListWidgetItem*> matches(lwFilterList->findItems(filter, Qt::MatchFlag::MatchContains));
	m_curMatches = matches.size();
	for (QListWidgetItem* item : matches)
	{
		item->setHidden(false);
		if (matches.size() == 1)
		{
			lwFilterList->setCurrentItem(item);
		}
	}
	updateOKAndDescription();
}

void dlg_FilterSelection::updateOKAndDescription()
{
	bool enable = m_curMatches == 1 ||
		(lwFilterList->currentItem() != nullptr && !lwFilterList->currentItem()->isHidden());
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
	QString description;
	if (enable)
	{
		QString filterName = lwFilterList->currentItem()->text();
		auto filter = iAFilterRegistry::filter(filterName);
		assert(filter);
		description = filter->description();
	}
	teDescription->setText(description);
}

void dlg_FilterSelection::listSelectionChanged(QListWidgetItem * /*current*/, QListWidgetItem * /*previous*/)
{
	updateOKAndDescription();
}

QString dlg_FilterSelection::selectedFilterName() const
{
	return lwFilterList->currentItem() == nullptr ? QString() : lwFilterList->currentItem()->text();
}
