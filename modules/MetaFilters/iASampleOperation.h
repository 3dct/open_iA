// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAPerformanceHelper.h>

#include <QString>
#include <QThread>

class iASampleOperation: public QThread
{
public:
	iASampleOperation();
	virtual ~iASampleOperation();
	virtual QString output() const = 0;
	bool success() const;
	iAPerformanceTimer::DurationType duration() const;
protected:
	virtual void performWork() =0;
	void setSuccess(bool success);
private:
	void run() override;
	iAPerformanceTimer m_timer;
	iAPerformanceTimer::DurationType m_duration;
	bool m_success;
};
