/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAIntersectionDefinition.h"

#include "iAThresholdDefinitions.h"

#include <iAConsole.h>

namespace intersection
{

	iAXYLine::iAXYLine()
	{}

	QLineF::IntersectType iAXYLine::calulateILineInterSection(const iAXYLine& other, QPointF* pt) const
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		return this->intersects(other, pt);
#else
		return this->intersect(other, pt);
#endif
	}


	void iAXYLine::intersectWithLines(const QVector<iAXYLine>& allLines)
	{

		for (const iAXYLine& line : allLines)
		{
			QPointF pt_Intersect;

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			auto interSectFlag = this->intersects(line, &pt_Intersect);
#else
			auto interSectFlag = this->intersect(line, &pt_Intersect);
#endif
			if ((!pt_Intersect.isNull()) && ( !(interSectFlag == QLineF::UnboundedIntersection)))
			{
				m_intersectPoints.push_back(pt_Intersect);
			}
		}
	}

	void createLineSegments(const threshold_defs::iAParametersRanges& lineRange, QVector<iAXYLine>& xyLines)
	{
		std::vector<double> x_vals = lineRange.getXRange();
		std::vector<double> y_vals = lineRange.getYRange();

		if (x_vals.empty() || y_vals.empty()) throw std::invalid_argument("xrange for intersection empty") ;
		if (x_vals.size() != y_vals.size()) throw std::invalid_argument("y_range for intersection empty") ;
		float x1, x2, y1, y2;

		for (size_t start = 0; start < x_vals.size()-1; start++)
		{
			x1 =(float) x_vals[start];
			x2 = (float) x_vals[start + 1];
			y1 = (float) y_vals[start];
			y2 = (float) y_vals[start + 1];

			DEBUG_LOG(QString("segment %1 %2 %3 %4").arg(x1).arg(y1).arg(x2).arg(y2));

			iAXYLine line(x1, y1, x2, y2);
			xyLines.push_back(line);
		}
	}

	const QVector<QPointF>& iAXYLine::intersectionFromRange(const threshold_defs::iAParametersRanges& aRange)
	{
		QVector<iAXYLine> Lines;
		createLineSegments(aRange, Lines);
		DEBUG_LOG(QString("Line size %1").arg(Lines.size()));
		iAXYLine aLine = Lines.takeFirst();
		aLine.intersectWithLines(Lines);
		return this->m_intersectPoints;
	}

	const QVector<QPointF>& iAXYLine::intersectionLineWithRange(const threshold_defs::iAParametersRanges& aRange)
	{
		QVector<iAXYLine> Lines;
		createLineSegments(aRange, Lines);
		this->intersectWithLines(Lines);
		return this->m_intersectPoints;
	}

}
