// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAThreshMinMaxHelper.h"

#include <iALog.h>

#include <QPointF>
#include <QTextEdit>

namespace threshold_defs {

	QPointF iAThreshMinMaxHelper::getLokalMininum(const iAThresMinMax& results)
	{
		return QPointF(results.PeakMinXThreshold(), results.FreqPeakMinY());
	}

	QPointF iAThreshMinMaxHelper::determineThresholdResultsPointXY(const iAThresMinMax& results, QTextEdit *elem)
	{

		double fmin = results.FreqPeakMinY();
		elem->append(QString("fmin: %1").arg(fmin));
		//if fmin > fair/2 return first minimum gmin / fmin
		if (results.FreqPeakMinY() > results.fAirPeakHalf()) {

			elem->append(QString("fmin: %1 is greater than fair/2: %2").arg(fmin).arg(results.fAirPeakHalf()));

			return this->getLokalMininum(results);
		}
		else
		{
			elem->append(QString("fmin is smaller than fair/2"));

			//take the next crossing of fair/2 -> intersection point or 50 %
			double Iso50GreyValue = results.Iso50ValueThr();
			elem->append(QString("Iso 50 is: %1").arg(Iso50GreyValue));


			QPointF intersectionPoint = getIntersectionPoint(results);
			elem->append(QString("comparing iso 50 (%1) with intersection : (%2)").arg(Iso50GreyValue).arg(intersectionPoint.x()));
			QPointF resultingPoint(0, 0);
			if (Iso50GreyValue < intersectionPoint.x()) {
				elem->append(QString("iso 50 is lowest"));
				elem->append(QString("resultingPoint is the iso 50 value").arg(Iso50GreyValue));
				resultingPoint.setX(Iso50GreyValue);
			}
			else {
				elem->append(QString("iso 50 greater than the intersection-"));
				resultingPoint = intersectionPoint;
				elem->append(QString("resulting threshold will be intersection (%1) with curve:").arg(resultingPoint.x()));

			}

			return resultingPoint;

		}
	}

	QPointF iAThreshMinMaxHelper::getIntersectionPoint(const iAThresMinMax& results)
	{
		return results.getIntersectionPoint();
	}

};
