// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACharacteristicEnergy.h"

#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QRegularExpression>
#include <QStringList>

#include <cassert>

const double iACharacteristicEnergy::NotAvailable = -1;


iACharacteristicEnergy EnergyLoader::ConstructElement(QString line)
{
	iACharacteristicEnergy element;

	QRegularExpression splitter("\\s+");
	QStringList stringList = line.split(splitter);
	assert(stringList.size() > 2);
	if (stringList.size() < 2)
		return element;

	element.symbol = stringList[1];
	for (int i=2; i<stringList.size(); ++i)
	{
		element.energies.push_back(stringList[i].toDouble());
	}
	return element;
}

void EnergyLoader::Load(QString const & fileName, QVector<iACharacteristicEnergy>& output)
{
	if(!QFileInfo(fileName).exists())
		return;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	while (!file.atEnd())
	{
		QString line = file.readLine().trimmed();
		if (line.length() > 0 && line[0].isDigit())
		{
			iACharacteristicEnergy element = EnergyLoader::ConstructElement(line);
			output.push_back(element);
		}
	}
	file.close(); //cleanup
}
