// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iANonLinearAxisTicker.h"

iANonLinearAxisTicker::iANonLinearAxisTicker() :
	m_tickVector(QVector<double>(0)),
	m_tickStep(0)
{
}

void iANonLinearAxisTicker::setTickData(const QVector<double> &tickVector)
{
	m_tickVector = tickVector;
	// TODO: check getMantissa?
	m_tickVector.size() < 10 ? m_tickStep = 1 :
		m_tickStep = std::pow(10, (std::floor(std::log10(m_tickVector.size())) - 1));
}

void iANonLinearAxisTicker::setAxis(QCPAxis *xAxis)
{
	m_xAxis = xAxis;
}

QVector<double> iANonLinearAxisTicker::createTickVector(double tickStep,
	const QCPRange &range)
{
	Q_UNUSED(tickStep)  Q_UNUSED(range)
	QVector<double> result;
	for (int i = 0; i < m_tickVector.size(); i += m_tickStep)
	{
		result.append(m_tickVector[i]);
	}
	return result;
}

QVector<double> iANonLinearAxisTicker::createSubTickVector(int subTickCount,
	const QVector<double> &ticks)
{
	Q_UNUSED(subTickCount)
	QVector<double> result;
	auto start = std::lower_bound(m_tickVector.begin(), m_tickVector.end(), ticks.first());
	int startIdx = start - m_tickVector.begin();
	auto end = std::lower_bound(m_tickVector.begin(), m_tickVector.end(), ticks.last());
	int endIdx = end - m_tickVector.begin();
	int indicesAfterLastMajorTick = 0;

	if ((endIdx + m_tickStep) > m_tickVector.size() - 1)
	{
		indicesAfterLastMajorTick = m_tickVector.size() - 1 - endIdx;
	}

	for (int i = startIdx; i <= endIdx + indicesAfterLastMajorTick; ++i)
	{
		if ((i % m_tickStep) != 0)
		{
			result.append(m_tickVector[i]);
		}
	}
	return result;
}

QVector<QString> iANonLinearAxisTicker::createLabelVector(const QVector<double> &ticks,
	const QLocale &locale, QChar formatChar, int precision)
{
	//TODO: set dist automatically
	QVector<QString> result;
	if (ticks.size() == 0)
	{
		return result;
	}

	int prev = 1;
	for (int i = 0; i < ticks.size(); ++i)
	{
		if (i > 0)
		{
			double dist = m_xAxis->coordToPixel(ticks[i]) -
				m_xAxis->coordToPixel(ticks[i - prev]);
			if (dist < 20.0)
			{
				prev++;
				result.append(QString(""));
				continue;
			}
		}
		prev = 1;
		auto start = std::lower_bound(m_tickVector.begin(), m_tickVector.end(), ticks[i]);
		int startIdx = start - m_tickVector.begin();
		result.append(QCPAxisTicker::getTickLabel(startIdx,	locale, formatChar, precision));
	}
	return result;
}
