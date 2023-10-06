// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASeedType.h"

#include "iALog.h"
#include "iAImageCoordinate.h"

#include <QRegularExpression>

#include <utility>     // for std::make_pair

std::shared_ptr<iASeedVector> ExtractSeedVector(QString const & seedString, int width, int height, int depth)
{
	QStringList lines = seedString.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
	auto result = std::make_shared<iASeedVector>();
	QString parseErrors;
	bool numberOK;
	// convert seed string to vector:
	QRegularExpression rx("(\\ |\\,|\\.|\\:|\\;|\\t)");  //RegEx for ' ' or ',' or '.' or ':' or '\t'
	for (int lineNumber = 0; lineNumber < lines.size(); ++lineNumber)
	{
		QString line = lines[lineNumber];
		QStringList query = line.split(rx);
		int x=0,
			y=0,
			z=0;
		if (query.size() < 2)
		{
			parseErrors.append(QString("Line %1: Invalid coordinate number %2.\n").arg(lineNumber).arg(query.size()));
			continue;
		}
		x = query[0].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid x-coordinate: %2.\n").arg(lineNumber).arg(query[0]));
			continue;
		}
		y = query[1].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid y-coordinate: %2.\n").arg(lineNumber).arg(query[1]));
			continue;
		}
		z = query[2].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid z-coordinate: %2.\n").arg(lineNumber).arg(query[2]));
			continue;
		}
		int label = query[3].toInt(&numberOK);
		if (!numberOK)
		{
			parseErrors.append(QString("Line %1: Invalid label: %2.\n").arg(lineNumber).arg(query[3]));
			continue;
		}
		if (x < 0 || x > width ||
		    y < 0 || y > height ||
			z < 0 || z > depth)
		{
			parseErrors.append(QString("Line %1: Coordinate outside of image: %2, %3, %4.\n").arg(lineNumber).arg(x).arg(y).arg(z));
		}
		else
		{
			result->push_back(std::make_pair(iAImageCoordinate(x, y, z), label));
		}
	}
	if (parseErrors.size() > 0)
	{
		LOG(lvlError, QString("Error(s) in seed file: %1").arg(parseErrors));
	}
	return result;
}
