/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "iANonLinearAxisTicker.h"

iANonLinearAxisTicker::iANonLinearAxisTicker() :
	m_tickVector(QVector<double>(0)),
	m_tickStep(0)
{
}

void iANonLinearAxisTicker::setTickData(QVector<double> tickVector)
{
	m_tickVector = tickVector;
	// TODO: check getMantissa?
	m_tickVector.size() < 10 ? m_tickStep = 1 :
		m_tickStep = pow(10, (floor(log10(m_tickVector.size())) - 1));
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
	for (int i = 0; i < m_tickVector.size(); i+=m_tickStep)
		result.append(m_tickVector[i]);
	return result;
}

QVector<double> iANonLinearAxisTicker::createSubTickVector(int subTickCount,
	const QVector<double> &ticks)
{
	Q_UNUSED(subTickCount)
	QVector<double> result;
	int startIdx = m_tickVector.indexOf(ticks.first());
	int endIdx = m_tickVector.indexOf(ticks.last());
	int indicesAfterLastMajorTick = 0;
	
	if ((endIdx + m_tickStep) > m_tickVector.size() - 1)
		indicesAfterLastMajorTick = m_tickVector.size() - 1 - endIdx;
	
	for (int i = startIdx; i <= endIdx + indicesAfterLastMajorTick; ++i)
		if ((i % m_tickStep) != 0)
			result.append(m_tickVector[i]);
	return result;
}

QVector<QString> iANonLinearAxisTicker::createLabelVector(const QVector<double> &ticks,
	const QLocale &locale, QChar formatChar, int precision)
{
	//TODO: set dist automatically
	QVector<QString> result;
	if (ticks.size() == 0)
		return result;

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
		result.append(QCPAxisTicker::getTickLabel(m_tickVector.indexOf(ticks[i]),
			locale, formatChar, precision));
	}
	return result;
}