#include <QPointF>
#include "ThresMinMaxHelper.h"
#include "iAConsole.h"

namespace threshold_defs {

	QPointF threshold_defs::ThresMinMaxHelper::getLokalMininum(const ThresMinMax& results)
	{
		return QPointF(results.PeakMinXThreshold(), results.FreqPeakMinY());
	}

	QPointF ThresMinMaxHelper::determineThresholdResultsPointXY(const ThresMinMax& results)
	{

		double fmin = results.FreqPeakMinY();
		//if fmin > fair/2 return first minimum gmin / fmin 
		if (compareFminWithAirPeak(fmin, results)) {
			DEBUG_LOG(QString("fmin: %1 is greater than fair/2").arg(fmin));
			return this->getLokalMininum(results);
		}
		else
		{
			//take the next crossing of fair/2 -> intersection point or 50 %
			double lokalGreyThr = results.Iso50ValueThr();
			QPointF intersectionPoint = getIntersectionPoint(results);
			QPointF resultingPoint(0, 0);
			if (lokalGreyThr < intersectionPoint.x()) {
				DEBUG_LOG("resultingPoint is the iso 50 value");
				resultingPoint.setX(lokalGreyThr);
			}
			else {
				DEBUG_LOG(QString("resulting point will be intersection point"));
				resultingPoint = intersectionPoint;
			}

			return resultingPoint;

		}
	}

	bool ThresMinMaxHelper::compareFminWithAirPeak(const double fmin, const ThresMinMax& results)
	{
		return (results.FreqPeakMinY() > results.fAirPeakHalf());
	}

	QPointF ThresMinMaxHelper::getIntersectionPoint(const ThresMinMax& results)
	{
		return results.getIntersectionPoint();
	}

};