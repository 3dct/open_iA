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
#pragma once

#include "iAThresholdDefinitions.h"

#include <QLineF>
#include <QVector>

class QPointF;
class QString;

namespace intersection
{
	class iAXYLine : protected QLineF
	{
	public:
		iAXYLine();
		iAXYLine(const QPointF& pt1, const QPointF& pt2):QLineF(pt1, pt2) {};
		iAXYLine(float x1, float y1, float x2, float y2):QLineF(x1, y1, x2, y2) {};
		iAXYLine(const QLineF &line):QLineF(line) {};

		//can be also null if no intersection
		QLineF::IntersectType calulateILineInterSection(const iAXYLine& other, QPointF* pt) const;

		//calculates the intersection with of line segments with a current line;
		void intersectWithLines(const QVector<iAXYLine> &QVector);


		//the first line is taken for intersection calculation
		const QVector<QPointF>& intersectionFromRange(const threshold_defs::iAParametersRanges& aRange);
		//output vector with intersection points

		//intersects a this line with some segments
		const QVector<QPointF>& intersectionLineWithRange(const threshold_defs::iAParametersRanges& aRange);

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

}
