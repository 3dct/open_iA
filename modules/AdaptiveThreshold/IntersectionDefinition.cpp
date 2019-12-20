#include "IntersectionDefinition.h"
#include "ThresholdDefinitions.h"
#include "iAConsole.h"

namespace intersection{

	QLineF::IntersectType intersection::XYLine::calulateILineInterSection(const XYLine& other, QPointF* pt) const
	{
		return this->intersect(other, pt);
	}


	void intersection::XYLine::intersectWithLines(const QVector<XYLine>& allLines)
	{
		
		for (const XYLine& line : allLines) {
			
			QPointF pt_Intersect;

			auto interSectFlag = this->intersect(line, &pt_Intersect);
			if ((!pt_Intersect.isNull()) && ( !(interSectFlag == QLineF::UnboundedIntersection)))
				m_intersectPoints.push_back(pt_Intersect);

		}
		
	}

	void LineCreator::createLineSegments(const threshold_defs::ParametersRanges& lineRange, QVector<XYLine>& xyLines)
	{
		std::vector<double> x_vals = lineRange.getXRange();
		std::vector<double> y_vals = lineRange.getYRange();
		 
		if (x_vals.empty() || y_vals.empty()) throw std::invalid_argument("xrange for intersection empty") ;
		if (x_vals.size() != y_vals.size()) throw std::invalid_argument("y_range for intersection empty") ; 
		float x1, x2, y1, y2; 

		for (size_t start = 0; start < x_vals.size()-1; start++){
						
			x1 =(float) x_vals[start];
			x2 = (float) x_vals[start + 1];
			y1 = (float) y_vals[start]; 
			y2 = (float) y_vals[start + 1];

			DEBUG_LOG(QString("segment %1 %2 %3 %4").arg(x1).arg(y1).arg(x2).arg(y2)); 

			XYLine line(x1, y1, x2, y2);
			xyLines.push_back(line);
		}

	}

	const QVector<QPointF>& XYLine::intersectionFromRange(const threshold_defs::ParametersRanges& aRange)
	{
		QVector<XYLine> Lines;		
		LineCreator::createLineSegments(aRange, Lines);
		DEBUG_LOG(QString("Line size %1").arg(Lines.size())); 
		XYLine aLine = Lines.takeFirst();
		aLine.intersectWithLines(Lines); 
		return this->m_intersectPoints;
	}

	const QVector<QPointF>& XYLine::intersectionLineWithRange(const threshold_defs::ParametersRanges& aRange)
	{

		try {
			QVector<XYLine> Lines;
			LineCreator::createLineSegments(aRange, Lines);
			this->intersectWithLines(Lines);
			return this->m_intersectPoints;
		}
		catch (std::invalid_argument& /*iar*/) {
		
			throw; 
		}
	}

}