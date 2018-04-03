/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
	m_curMatches(0)
{
	connect(leFilterSearch, SIGNAL(textEdited(QString const &)), this, SLOT(FilterChanged(QString const &)));
	connect(lwFilterList, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(ListSelectionChanged(QListWidgetItem *, QListWidgetItem *)));
	connect(lwFilterList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (auto filterFactory : iAFilterRegistry::FilterFactories())
		lwFilterList->addItem(filterFactory->Create()->Name());
	if (!preselectedFilter.isEmpty())
	{
		auto matching = lwFilterList->findItems(preselectedFilter, Qt::MatchExactly);
		if (matching.size() > 0)
			lwFilterList->setCurrentItem(matching[0]);
	}
}

void dlg_FilterSelection::FilterChanged(QString const & filter)
{
	for (int row=0; row < lwFilterList->count(); ++row)
		lwFilterList->item(row)->setHidden(true);
	QList<QListWidgetItem*> matches(lwFilterList->findItems(filter, Qt::MatchFlag::MatchContains));
	m_curMatches = matches.size();
	for (QListWidgetItem* item : matches)
	{
		item->setHidden(false);
		if (matches.size() == 1)
			lwFilterList->setCurrentItem(item);
	}
	EnableOKButton();
}

void dlg_FilterSelection::EnableOKButton()
{
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_curMatches == 1 ||
		(lwFilterList->currentItem() != nullptr && !lwFilterList->currentItem()->isHidden()));
}

void dlg_FilterSelection::ListSelectionChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
	EnableOKButton();
}

QString dlg_FilterSelection::SelectedFilterName() const
{
	return lwFilterList->currentItem() == nullptr ? QString() : lwFilterList->currentItem()->text();
}
