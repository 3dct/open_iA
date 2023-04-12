// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <QWidget>

class iADataSet;

class QTableWidget;
class QTableWidgetItem;

//! A widget containing a list of datasets.
class iAguibase_API iADataSetListWidget : public QWidget
{
	Q_OBJECT
public:
	iADataSetListWidget();
	void addDataSet(iADataSet const* dataset, size_t dataSetIdx, QVector<QAction*> const & actions, QVector<QAction*> const& editActions);
	void setName(size_t dataSetIdx, QString newName);
	void removeDataSet(size_t dataSetIdx);

signals:
	void dataSetSelected(size_t dataSetIdx);

private:
	int findDataSetIdx(size_t idx);

	QTableWidget* m_dataList;  //!< the actual table displaying datasets and controls for their visibility
};
