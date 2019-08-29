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
			DEBUG_LOG(QString("fmin: %1 is greater than fair/2: %2").arg(fmin).arg(results.fAirPeakHalf()));
			return this->getLokalMininum(results);
		}
		else
		{
			//take the next crossing of fair/2 -> intersection point or 50 %
			double Iso50GreyValue = results.Iso50ValueThr();
			QPointF intersectionPoint = getIntersectionPoint(results);
			QPointF resultingPoint(0, 0);
			if (Iso50GreyValue < intersectionPoint.x()) {
				DEBUG_LOG(QString("iso 50 is lowest iso50: %1 intersection: %2").arg(Iso50GreyValue).arg(intersectionPoint.x()))
				DEBUG_LOG(QString("resultingPoint is the iso 50 value- fmin: %1 fair/2 %2").arg(fmin).arg(results.fAirPeakHalf()));
				resultingPoint.setX(Iso50GreyValue);
			}
			else {
				DEBUG_LOG("iso 50 greater than the intersection-"); 
				DEBUG_LOG(QString("resulting point will be intersection point with curve"));
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