// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <qcustomplot.h>

class iANonLinearAxisTicker : public QCPAxisTicker
{
public:
	iANonLinearAxisTicker();

	void setTickData(const QVector<double> &tickVector);
	void setAxis(QCPAxis* axis);

protected:
	QVector<double> m_tickVector;
	QCPAxis *m_xAxis;
	int m_tickStep;

	QVector<double> createTickVector(double tickStep,
		const QCPRange &range) override;

	QVector<double> createSubTickVector(int subTickCount,
		const QVector<double> &ticks) override;

	QVector<QString> createLabelVector(const QVector<double> &ticks,
		const QLocale &locale, QChar formatChar, int precision) override;
};
