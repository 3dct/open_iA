#pragma once
class QPointF;
class QString;


#include "ThresholdDefinitions.h"
#include <QLineF>
#include <QVector>

namespace intersection {

	class XYLine : protected QLineF
	{
	public: 
		XYLine(): QLineF() {}
		XYLine(const QPointF& pt1, const QPointF& pt2):QLineF(pt1, pt2) {};
		XYLine(float x1, float y1, float x2, float y2):QLineF(x1, y1, x2, y2) {};
		XYLine(const QLineF &line):QLineF(line) {};

		//can be also null if no intersection
		QLineF::IntersectType calulateILineInterSection(const XYLine& other, QPointF* pt) const;
		
		//calculates the intersection with of line segments with a current line; 
		void intersectWithLines(const QVector<XYLine> &QVector);


		//the first line is taken for intersection calculation
		const QVector<QPointF>& intersectionFromRange(const threshold_defs::ParametersRanges& aRange);
		//output vector with intersection points

		//intersects a this line with some segments
		const QVector<QPointF>& intersectionLineWithRange(const threshold_defs::ParametersRanges& aRange);

		inline const QString toString() const {
			QString lineStr = "line ";
			lineStr += QString("%1 %2").arg(this->p1().x()).arg(this->p1().y());
			lineStr += QString("%1 %2").arg(this->p2().x()).arg(this->p2().y());
			return lineStr;
		}	

		inline const QVector<QPointF>& intersectionPoints() const {
			return m_intersectPoints;
		};

	private:
		QVector<QPointF> m_intersectPoints; 
	};


	class LineCreator {
	public:
		
	  	static void createLineSegments(const threshold_defs::ParametersRanges& lineRange, QVector<XYLine> &xyLines);
	private:
		LineCreator() = delete;
	};


}
