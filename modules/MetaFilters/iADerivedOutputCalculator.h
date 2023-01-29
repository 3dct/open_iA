// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// TODO: Replace by more generic way of executing filter-based derived output computations!

#include <QSharedPointer>
#include <QThread>

class iASingleResult;

class QString;

class iADerivedOutputCalculator : public QThread
{
public:
	iADerivedOutputCalculator(
		QSharedPointer<iASingleResult> result,
		int objCountIdx,
		int avgUncIdx,
		int labelCount);
	bool success();

private:
	QSharedPointer<iASingleResult> m_result;
	int m_objCountIdx;
	int m_avgUncIdx;
	bool m_success;
	int m_labelCount;

	void run() override;
};
