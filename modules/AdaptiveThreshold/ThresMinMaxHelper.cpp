#include <QPointF>
#include "ThresMinMaxHelper.h"
#include "iAConsole.h"
#include <QTextEdit>


namespace threshold_defs {

	QPointF threshold_defs::ThresMinMaxHelper::getLokalMininum(const ThresMinMax& results)
	{
		return QPointF(results.PeakMinXThreshold(), results.FreqPeakMinY());
	}

	QPointF ThresMinMaxHelper::determineThresholdResultsPointXY(const ThresMinMax& results, QTextEdit *elem)
	{

		double fmin = results.FreqPeakMinY();
		//if fmin > fair/2 return first minimum gmin / fmin 
		if (compareFminWithAirPeak(fmin, results)) {
			
			elem->append(QString("fmin: %1 is greater than fair/2: %2").arg(fmin).arg(results.fAirPeakHalf()));
			
			return this->getLokalMininum(results);
		}
		else
		{
			elem->append(QString("fmin < fair/2")); 
			//take the next crossing of fair/2 -> intersection point or 50 %
			double Iso50GreyValue = results.Iso50ValueThr();
			QPointF intersectionPoint = getIntersectionPoint(results);
			QPointF resultingPoint(0, 0);
			if (Iso50GreyValue < intersectionPoint.x()) {
				elem->append(QString("iso 50 is lowest iso50: %1 intersection: %2").arg(Iso50GreyValue).arg(intersectionPoint.x()));
				elem->append(QString("resultingPoint is the iso 50 value- fmin: %1 fair/2 %2").arg(fmin).arg(results.fAirPeakHalf()));
				resultingPoint.setX(Iso50GreyValue);
			}
			else {
				elem->append(QString("iso 50 greater than the intersection-")); 
				elem->append(QString("resulting threshold will be intersection point with curve:"));
				//elem->append(QString("%1").arg(resultingPoint.x);
				resultingPoint = intersectionPoint;
				elem->append(QString("%1").arg(resultingPoint.x()));
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