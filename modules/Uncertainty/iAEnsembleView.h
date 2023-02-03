// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVector>
#include <QWidget>

class iAEnsemble;

class QListWidget;
class QListWidgetItem;

class iAEnsembleView : public QWidget
{
	Q_OBJECT
public:
	iAEnsembleView();
	void AddEnsemble(QString const & caption, QSharedPointer<iAEnsemble> ensemble);
	QVector<QSharedPointer<iAEnsemble> > & Ensembles();
signals:
	void EnsembleSelected(QSharedPointer<iAEnsemble> ensemble);
private slots:
	void EnsembleDblClicked(QListWidgetItem*);
private:
	QListWidget* m_list;
	QVector<QSharedPointer<iAEnsemble> > m_ensembles;
};
