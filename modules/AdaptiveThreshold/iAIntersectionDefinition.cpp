// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAIntersectionDefinition.h"

#include "iAThresholdDefinitions.h"

#include <QLineF>

#include <iALog.h>

namespace
{
	void createLineSegments(const threshold_defs::iAParametersRanges& lineRange, QVector<QLineF>& xyLines)
	{
		std::vector<double> x_vals = lineRange.getXRange();
		std::vector<double> y_vals = lineRange.getYRange();

		if (x_vals.empty() || y_vals.empty())
		{
			throw std::invalid_argument("xrange for intersection empty");
		}
		if (x_vals.size() != y_vals.size())
		{
			throw std::invalid_argument("y_range for intersection empty");
		}
		float x1, x2, y1, y2;

		for (size_t start = 0; start < x_vals.size() - 1; start++)
		{
			x1 = static_cast<float>(x_vals[start]);
			x2 = static_cast<float>(x_vals[start + 1]);
			y1 = static_cast<float>(y_vals[start]);
			y2 = static_cast<float>(y_vals[start + 1]);

			QLineF line(x1, y1, x2, y2);
			xyLines.push_back(line);
		}
	}
}

QVector<QPointF> intersectLineWithRange(QLineF const& line, const threshold_defs::iAParametersRanges& aRange)
{
	QVector<QLineF> allLines;
	createLineSegments(aRange, allLines);
	QVector<QPointF> intersectPoints;
	for (const QLineF& otherLine : allLines)
	{
		QPointF pt_Intersect;

		auto interSectFlag = line.intersects(otherLine, &pt_Intersect);
		if ((!pt_Intersect.isNull()) && (!(interSectFlag == QLineF::UnboundedIntersection)))
		{
			intersectPoints.push_back(pt_Intersect);
		}
	}
	return intersectPoints;
}
