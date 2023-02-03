// Copyright 2016-2023, the open_iA contributors
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
	typedef QSharedPointer<iASamplingResults> SamplingResultPtr;
	dlg_samplings();
	void Add(SamplingResultPtr samplingResults);
	SamplingResultPtr GetSampling(int idx);
	int SamplingCount() const;
	QSharedPointer<QVector<SamplingResultPtr> > GetSamplings();
public slots:
	void Remove();
signals:
	void AddSampling();
private:
	QStandardItemModel* m_itemModel;
	QSharedPointer<QVector<SamplingResultPtr> > m_samplings;
};
