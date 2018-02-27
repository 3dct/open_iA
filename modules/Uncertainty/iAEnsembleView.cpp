/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAEnsembleView.h"

#include <QHBoxLayout>
#include <QListWidget>

iAEnsembleView::iAEnsembleView():
	m_list(new QListWidget())
{
	setLayout(new QHBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(4, 4, 4, 4);
	layout()->addWidget(m_list);
	connect(m_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(EnsembleDblClicked(QListWidgetItem*)));
	setMaximumWidth(200);
}


void iAEnsembleView::AddEnsemble(QString const & caption, QSharedPointer<iAEnsemble> ensemble)
{
	m_ensembles.push_back(ensemble);
	QListWidgetItem* item = new QListWidgetItem(caption);
	item->setData(Qt::UserRole, m_ensembles.size() - 1);
	m_list->addItem(item);
}


void iAEnsembleView::EnsembleDblClicked(QListWidgetItem* item)
{
	emit EnsembleSelected(
		m_ensembles[
			item->data(Qt::UserRole).toInt()
		]
	);
}

QVector<QSharedPointer<iAEnsemble> > & iAEnsembleView::Ensembles()
{
	return m_ensembles;
}
