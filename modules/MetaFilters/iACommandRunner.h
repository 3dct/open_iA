// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASampleOperation.h"

#include <QProcess>

class iACommandRunner : public iASampleOperation
{
	Q_OBJECT
public:
	iACommandRunner(QString const & executable, QStringList const & arguments);
	QString output() const override;
private slots:
	void errorOccured(QProcess::ProcessError);
private:
	void performWork() override;
	QString m_executable;
	QStringList m_arguments;
	QString m_output;
};
