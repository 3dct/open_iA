// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQMenuHelper.h"

#include <QMenu>

void addToMenuSorted(QMenu* menu, QAction* action)
{
	for (QAction* curAct : menu->actions())
	{
		if (curAct->text() > action->text())
		{
			menu->insertAction(curAct, action);
			return;
		}
	}
	menu->addAction(action);
}

QMenu* getOrAddSubMenu(QMenu* parentMenu, QString const& title, bool addSeparator)
{
	QList<QMenu*> submenus = parentMenu->findChildren<QMenu*>();
	for (int i = 0; i < submenus.size(); ++i)
	{
		if (submenus.at(i)->title() == title)
		{
			if (addSeparator && !submenus.at(i)->isEmpty())
			{
				submenus.at(i)->addSeparator();
			}
			return submenus.at(i);
		}
	}
	QMenu* result = new QMenu(parentMenu);
	result->setTitle(title);
	addToMenuSorted(parentMenu, result->menuAction());
	return result;
}
