// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// TODO: Replace by more generic way of executing filter-based derived output computations!

#include <QThread>

#include <memory>

class iASingleResult;

class QString;

class iADerivedOutputCalculator : public QThread
{
public:
	iADerivedOutputCalculator(
		std::shared_ptr<iASingleResult> result,
		int objCountIdx,
		int avgUncIdx,
		int labelCount);
	bool success();

private:
	std::shared_ptr<iASingleResult> m_result;
	int m_objCountIdx;
	int m_avgUncIdx;
	bool m_success;
	int m_labelCount;

	void run() override;
};
