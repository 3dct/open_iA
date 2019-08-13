#include "IntersectionDefinition.h"
#include "ThresholdDefinitions.h"
#include "iAConsole.h"

namespace intersection{

	void intersection::XYLine::calulateInterSection(const XYLine& other, QPointF* pt) const
	{
		this->intersect(other, pt);
	}

	void intersection::XYLine::intersectWithLines(const QVector<XYLine>& allLines)
	{		for (const XYLine& line : allLines) {
			QPointF pt_Intersect;

			this->intersect(line, &pt_Intersect);
			if (!pt_Intersect.isNull())
				m_intersectPoints.push_back(pt_Intersect);
			}
	}

	void LineCreator::createLineSegments(const threshold_defs::ParametersRanges& lineRange, QVector<XYLine>& xyLines)
	{
		std::vector<double> x_vals = lineRange.getXRange();
		std::vector<double> y_vals = lineRange.getYRange();
		 
		if (x_vals.empty() || y_vals.empty()) return;
		if (x_vals.size() != y_vals.size()) return; 
		double x1, x2, y1, y2; 

		for (size_t start = 0; start < x_vals.size() - 1; start){
			x1 = x_vals[start]; x2 = x_vals[start + 1];
			y1 = y_vals[start]; y2 = y_vals[start + 1];
			XYLine line(x1, y1, x2, y2);
			xyLines.push_back(line);
		}

	}

	void XYLine::intersectionFromRange(const threshold_defs::ParametersRanges& aRange)
	{
		QVector<XYLine> Lines;		
		LineCreator::createLineSegments(aRange, Lines);
		DEBUG_LOG(QString("Line size %1").arg(Lines.size())); 
		XYLine aLine = Lines.takeFirst();
		aLine.intersectWithLines(Lines); 
	}

}