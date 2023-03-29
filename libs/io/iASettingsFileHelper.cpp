// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASettingsFileHelper.h"

#include <QFile>
#include <QTextStream>

#include <stdexcept>

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
		auto key = tokensComment[0].left(firstSep).trimmed();     // everything up until the first separator
		auto value = tokensComment[0].right(tokensComment[0].length() - (firstSep+1)).trimmed();  // everything after first separator
		result[key] = value;
	}
	// file closed automatically by ~QFile
	return result;
}
