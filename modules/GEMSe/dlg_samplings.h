// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_samplings.h"

#include <qthelper/iAQTtoUIConnector.h>

class iASamplingResults;

class QStandardItemModel;

typedef iAQTtoUIConnector<QDockWidget, Ui_samplings> dlgSamplingsUI;

class dlg_samplings : public dlgSamplingsUI
{
	Q_OBJECT
public:
	typedef std::shared_ptr<iASamplingResults> SamplingResultPtr;
	dlg_samplings();
	void Add(SamplingResultPtr samplingResults);
	SamplingResultPtr GetSampling(qsizetype idx);
	qsizetype SamplingCount() const;
	std::shared_ptr<QVector<SamplingResultPtr> > GetSamplings();
public slots:
	void Remove();
signals:
	void AddSampling();
private:
	QStandardItemModel* m_itemModel;
	std::shared_ptr<QVector<SamplingResultPtr> > m_samplings;
};
