// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	connect(m_list, &QListWidget::itemDoubleClicked, this, &iAEnsembleView::EnsembleDblClicked);
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
