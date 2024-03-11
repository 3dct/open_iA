// Copyright (c) open_iA contributors
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
	void AddEnsemble(QString const & caption, std::shared_ptr<iAEnsemble> ensemble);
	QVector<std::shared_ptr<iAEnsemble> > & Ensembles();
signals:
	void EnsembleSelected(std::shared_ptr<iAEnsemble> ensemble);
private slots:
	void EnsembleDblClicked(QListWidgetItem*);
private:
	QListWidget* m_list;
	QVector<std::shared_ptr<iAEnsemble> > m_ensembles;
};
