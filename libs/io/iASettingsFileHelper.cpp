/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASettingsFileHelper.h"

#include <QFile>
#include <QTextStream>

QMap<QString, QString> readSettingsFile(QString const& fileName)
{
	QFile file(fileName);
	QString const& KeyValueSeparator = ":";
	QString const& CommentSeparator = "%";
	if (!file.open(QIODevice::ReadOnly))
	{
		throw std::runtime_error(QString("Could not open file %1").arg(fileName).toStdString());
	}
	QTextStream textStream(&file);
	QMap<QString, QString> result;
	int lineNr = 0;
	while (!textStream.atEnd())
	{
		++lineNr;
		QString currentLine = textStream.readLine();
		if (currentLine.trimmed().isEmpty())
		{
			continue;
		}
		auto tokensComment = currentLine.split(CommentSeparator); // throw away everything after first comment separator character
		auto firstSep = tokensComment[0].indexOf(KeyValueSeparator);
		if (firstSep == -1)
		{
			throw std::runtime_error(QString("Invalid key/value line (#%1: %2) - could not split at separator %3")
				.arg(lineNr).arg(currentLine).arg(KeyValueSeparator).toStdString());
		}
		auto key = tokensComment[0].first(firstSep).trimmed();     // everything up until the separator
		auto value = tokensComment[0].sliced(firstSep+1).trimmed();  // everything after key value separator
		result[key] = value;
	}
	// file closed automatically by ~QFile
	return result;
}
