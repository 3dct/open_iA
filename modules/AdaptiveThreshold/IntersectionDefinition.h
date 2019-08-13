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
		XYLine(const QPointF& pt1, const QPointF& pt2) :QLineF(pt1, pt2) {};
		XYLine(double x1, double y1, double x2, double y2) :QLineF(x1, y1, x2, y2) {};
		

		//can be also null if no intersection
		void calulateInterSection(const XYLine &other, QPointF *pt) const;
		
		//calculates the intersection with of line segments with a current line; 
		void intersectWithLines(const QVector<XYLine> &QVector);

		void intersectionFromRange(const threshold_defs::ParametersRanges& aRange);

		inline const QString &toString() const {
			QString lineStr = "line ";
			lineStr += QString("%1 %2").arg(this->p1().x()).arg(this->p1().y());
			lineStr += QString("%1 %2").arg(this->p2().x()).arg(this->p2().y());
			return lineStr;
		}	

		inline const QVector<QPointF>& intersectionPoins() const {
			return m_intersectPoints; 
		};

	private:
	
		//XYLine(const XYLine& other);
		XYLine() = delete;
		QVector<QPointF> m_intersectPoints; 
	};

	class LineCreator {
	public:
		//const QVector<XYLine>& createLineSegments(const ParametersRanges& lineRange);
	  	///*static*/ const QVector<intersection::XYLine> createLineSegments(const threshold_defs::ParametersRanges& lineRange);
		static void createLineSegments(const threshold_defs::ParametersRanges& lineRange, QVector<XYLine> &xyLines);
	private:
		LineCreator() = delete;
	};


}