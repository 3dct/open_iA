/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iASeedType.h"

#include "iALog.h"
#include "iAImageCoordinate.h"

#include <QRegularExpression>

#include <utility>     // for std::make_pair

QSharedPointer<iASeedVector> ExtractSeedVector(QString const & seedString, int width, int height, int depth)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	QStringList lines = seedString.split(QRegularExpression("[\r\n]"),QString::SkipEmptyParts);
#else
	QStringList lines = seedString.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
#endif
	auto result = QSharedPointer<iASeedVector>::create();
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
